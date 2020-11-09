// -*- C++ -*-
// Copyright (c) 2012-2015 Jakob Progsch
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//
// Modified for log4cplus, copyright (c) 2014-2015 Václav Zeman.

#ifndef THREAD_POOL_H_7ea1ee6b_4f17_4c09_b76b_3d44e102400c
#define THREAD_POOL_H_7ea1ee6b_4f17_4c09_b76b_3d44e102400c

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <cassert>


namespace progschj {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads
        = (std::max)(2u, std::thread::hardware_concurrency()));
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    void wait_until_empty();
    void wait_until_nothing_in_flight();
    void set_queue_size_limit(std::size_t limit);
    void set_pool_size(std::size_t limit);
    ~ThreadPool();

private:
    void emplace_back_worker (std::size_t worker_number);

    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // target pool size
    std::size_t pool_size;
    // the task queue
    std::queue< std::function<void()> > tasks;
    // queue length limit
    std::size_t max_queue_size = 100000;
    // stop signal
    bool stop = false;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition_producers;
    std::condition_variable condition_consumers;

    std::mutex in_flight_mutex;
    std::condition_variable in_flight_condition;
    std::atomic<std::size_t> in_flight;

    struct handle_in_flight_decrement
    {
        ThreadPool & tp;

        handle_in_flight_decrement(ThreadPool & tp_)
            : tp(tp_)
        { }

        ~handle_in_flight_decrement()
        {
            std::size_t prev
                = std::atomic_fetch_sub_explicit(&tp.in_flight,
                    std::size_t(1),
                    std::memory_order_acq_rel);
            if (prev == 1)
            {
                std::unique_lock<std::mutex> guard(tp.in_flight_mutex);
                tp.in_flight_condition.notify_all();
            }
        }
    };
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(std::size_t threads)
    : pool_size(threads)
    , in_flight(0)
{
    for (std::size_t i = 0; i != threads; ++i)
        emplace_back_worker(i);
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();

    std::unique_lock<std::mutex> lock(queue_mutex);
    if (tasks.size () >= max_queue_size)
        // wait for the queue to empty or be stopped
        condition_producers.wait(lock,
            [this]
            {
                return tasks.size () < max_queue_size
                    || stop;
            });

    // don't allow enqueueing after stopping the pool
    if (stop)
        throw std::runtime_error("enqueue on stopped ThreadPool");

    tasks.emplace([task](){ (*task)(); });
    std::atomic_fetch_add_explicit(&in_flight,
        std::size_t(1),
        std::memory_order_relaxed);
    condition_consumers.notify_one();

    return res;
}


// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
    condition_consumers.notify_all();
    condition_producers.notify_all();
    pool_size = 0;
    condition_consumers.wait(lock, [this]{ return this->workers.empty(); });
    assert(in_flight == 0);
}

inline void ThreadPool::wait_until_empty()
{
    std::unique_lock<std::mutex> lock(this->queue_mutex);
    this->condition_producers.wait(lock,
        [this]{ return this->tasks.empty(); });
}

inline void ThreadPool::wait_until_nothing_in_flight()
{
    std::unique_lock<std::mutex> lock(this->in_flight_mutex);
    this->in_flight_condition.wait(lock,
        [this]{ return this->in_flight == 0; });
}

inline void ThreadPool::set_queue_size_limit(std::size_t limit)
{
    std::unique_lock<std::mutex> lock(this->queue_mutex);

    if (stop)
        return;

    std::size_t const old_limit = max_queue_size;
    max_queue_size = (std::max)(limit, std::size_t(1));
    if (old_limit < max_queue_size)
        condition_producers.notify_all();
}

inline void ThreadPool::set_pool_size(std::size_t limit)
{
    if (limit < 1)
        limit = 1;

    std::unique_lock<std::mutex> lock(this->queue_mutex);

    if (stop)
        return;

    pool_size = limit;
    std::size_t const old_size = this->workers.size();
    if (pool_size > old_size)
    {
        // create new worker threads
        for (std::size_t i = old_size; i != pool_size; ++i)
            emplace_back_worker(i);
    }
    else if (pool_size < old_size)
        // notify all worker threads to start downsizing
        this->condition_consumers.notify_all();
}

inline void ThreadPool::emplace_back_worker (std::size_t worker_number)
{
    workers.emplace_back(
        [this, worker_number]
        {
            for(;;)
            {
                std::function<void()> task;
                bool notify;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition_consumers.wait(lock,
                        [this, worker_number]{
                            return this->stop || !this->tasks.empty()
                                || pool_size < worker_number + 1; });

                    // deal with downsizing of thread pool or shutdown
                    if ((this->stop && this->tasks.empty())
                        || (!this->stop && pool_size < worker_number + 1))
                    {
                        std::thread & last_thread = this->workers.back();
                        std::thread::id this_id = std::this_thread::get_id();
                        if (this_id == last_thread.get_id())
                        {
                            // highest number thread exits, resizes the workers
                            // vector, and notifies others
                            last_thread.detach();
                            this->workers.pop_back();
                            this->condition_consumers.notify_all();
                            return;
                        }
                        else
                            continue;
                    }
                    else if (!this->tasks.empty())
                    {
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        notify = this->tasks.size() + 1 ==  max_queue_size
                            || this->tasks.empty();
                    }
                    else
                        continue;
                }

                handle_in_flight_decrement guard(*this);

                if (notify)
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    condition_producers.notify_all();
                }

                task();
            }
        }
        );
}

} // namespace progschj

#endif // THREAD_POOL_H_7ea1ee6b_4f17_4c09_b76b_3d44e102400c

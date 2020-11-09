#ifndef ABSTRACTTHREADEDSERVER_H
#define ABSTRACTTHREADEDSERVER_H

#include "abstractserverconnector.h"
#include "threadpool.h"
#include <memory>
#include <thread>

namespace jsonrpc {
class AbstractThreadedServer : public AbstractServerConnector {
public:
  AbstractThreadedServer(size_t threads);
  virtual ~AbstractThreadedServer();

  virtual bool StartListening();
  virtual bool StopListening();

protected:
  /**
   * @brief InitializeListener should initialize sockets, file descriptors etc.
   * @return
   */
  virtual bool InitializeListener() = 0;

  /**
   * @brief CheckForConnection should poll for a new connection. This must be
   * a non-blocking call.
   * @return a handle which is passed on to HandleConnection()
   */
  virtual int CheckForConnection() = 0;

  /**
   * @brief HandleConnection must handle connection information for a given
   * handle that has been returned by CheckForConnection()
   * @param connection
   */
  virtual void HandleConnection(int connection) = 0;

private:
  bool running;
  std::unique_ptr<std::thread> listenerThread;
  ThreadPool threadPool;
  size_t threads;

  void ListenLoop();
};
}

#endif // ABSTRACTTHREADEDSERVER_H

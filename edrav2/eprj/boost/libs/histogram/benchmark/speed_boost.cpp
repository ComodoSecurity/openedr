// Copyright 2015-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/histogram.hpp>
#include "../test/utility_histogram.hpp"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <memory>
#include <random>

using namespace boost::histogram;
using reg = axis::regular<>;

std::unique_ptr<double[]> random_array(unsigned n, int type) {
  std::unique_ptr<double[]> r(new double[n]);
  std::default_random_engine gen(1);
  if (type) { // type == 1
    std::normal_distribution<> d(0.5, 0.3);
    for (unsigned i = 0; i < n; ++i) r[i] = d(gen);
  } else { // type == 0
    std::uniform_real_distribution<> d(0.0, 1.0);
    for (unsigned i = 0; i < n; ++i) r[i] = d(gen);
  }
  return r;
}

double baseline(unsigned n) {
  auto r = random_array(n, 0);
  auto t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    volatile auto x = *it++;
    (void)(x);
  }
  return (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
}

template <typename Tag, typename Storage>
double compare_1d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  auto h = make_s(Tag(), Storage(), reg(100, 0, 1));
  auto t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) h(*it++);
  return (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
}

template <typename Tag, typename Storage>
double compare_2d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  auto h = make_s(Tag(), Storage(), reg(100, 0, 1), reg(100, 0, 1));
  auto t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    const auto x = *it++;
    const auto y = *it++;
    h(x, y);
  }
  return (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
}

template <typename Tag, typename Storage>
double compare_3d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  auto h = make_s(Tag(), Storage(), reg(100, 0, 1), reg(100, 0, 1), reg(100, 0, 1));
  auto t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    const auto x = *it++;
    const auto y = *it++;
    const auto z = *it++;
    h(x, y, z);
  }
  return (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
}

template <typename Tag, typename Storage>
double compare_6d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  auto h = make_s(Tag(), Storage(), reg(10, 0, 1), reg(10, 0, 1), reg(10, 0, 1),
                  reg(10, 0, 1), reg(10, 0, 1), reg(10, 0, 1));
  auto t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    const auto x1 = *it++;
    const auto x2 = *it++;
    const auto x3 = *it++;
    const auto x4 = *it++;
    const auto x5 = *it++;
    const auto x6 = *it++;
    h(x1, x2, x3, x4, x5, x6);
  }
  return (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
}

int main() {
  const unsigned nfill = 6000000;
  using SStore = std::vector<int>;
  using DStore = unlimited_storage<>;

  printf("baseline %.1f\n", baseline(nfill));

  for (int itype = 0; itype < 2; ++itype) {
    const char* d = itype == 0 ? "uniform" : "normal ";
    printf("1D-boost:SS-%s %5.1f\n", d, compare_1d<static_tag, SStore>(nfill, itype));
    printf("1D-boost:SD-%s %5.1f\n", d, compare_1d<static_tag, DStore>(nfill, itype));
    printf("1D-boost:DS-%s %5.1f\n", d, compare_1d<dynamic_tag, SStore>(nfill, itype));
    printf("1D-boost:DD-%s %5.1f\n", d, compare_1d<dynamic_tag, DStore>(nfill, itype));
    printf("2D-boost:SS-%s %5.1f\n", d, compare_2d<static_tag, SStore>(nfill, itype));
    printf("2D-boost:SD-%s %5.1f\n", d, compare_2d<static_tag, DStore>(nfill, itype));
    printf("2D-boost:DS-%s %5.1f\n", d, compare_2d<dynamic_tag, SStore>(nfill, itype));
    printf("2D-boost:DD-%s %5.1f\n", d, compare_2d<dynamic_tag, DStore>(nfill, itype));
    printf("3D-boost:SS-%s %5.1f\n", d, compare_3d<static_tag, SStore>(nfill, itype));
    printf("3D-boost:SD-%s %5.1f\n", d, compare_3d<static_tag, DStore>(nfill, itype));
    printf("3D-boost:DS-%s %5.1f\n", d, compare_3d<dynamic_tag, SStore>(nfill, itype));
    printf("3D-boost:DD-%s %5.1f\n", d, compare_3d<dynamic_tag, DStore>(nfill, itype));
    printf("6D-boost:SS-%s %5.1f\n", d, compare_6d<static_tag, SStore>(nfill, itype));
    printf("6D-boost:SD-%s %5.1f\n", d, compare_6d<static_tag, DStore>(nfill, itype));
    printf("6D-boost:DS-%s %5.1f\n", d, compare_6d<dynamic_tag, SStore>(nfill, itype));
    printf("6D-boost:DD-%s %5.1f\n", d, compare_6d<dynamic_tag, DStore>(nfill, itype));
  }
}

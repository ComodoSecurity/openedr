// Copyright 2015-2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <gsl/gsl_histogram.h>
#include <gsl/gsl_histogram2d.h>

#include <cassert>
#include <cstdio>
#include <ctime>
#include <memory>
#include <random>

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

double compare_1d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  gsl_histogram* h = gsl_histogram_alloc(100);
  gsl_histogram_set_ranges_uniform(h, 0, 1);
  double t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;)
    gsl_histogram_increment(h, *it++);
  t = (clock() - t) / CLOCKS_PER_SEC / n * 1e9;
  assert(distrib != 0 || gsl_histogram_sum(h) == n);
  gsl_histogram_free(h);
  return t;
}

double compare_2d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  gsl_histogram2d* h = gsl_histogram2d_alloc(100, 100);
  gsl_histogram2d_set_ranges_uniform(h, 0, 1, 0, 1);
  double t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    const auto x = *it++;
    const auto y = *it++;
    gsl_histogram2d_increment(h, x, y);
  }
  t = (clock() - t) / CLOCKS_PER_SEC / n * 1e9;
  assert(distrib != 0 || gsl_histogram2d_sum(h) == n / 2);
  gsl_histogram2d_free(h);
  return t;
}

int main(int argc, char** argv) {
  constexpr unsigned nfill = 6000000;
  for (int itype = 0; itype < 2; ++itype) {
    auto d = itype == 0 ? "uniform" : "normal ";
    printf("1D-gsl-%s %5.1f\n", d, compare_1d(nfill, itype));
    printf("2D-gsl-%s %5.1f\n", d, compare_2d(nfill, itype));
  }
}

// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <TH1I.h>
#include <TH2I.h>
#include <TH3I.h>
#include <THn.h>

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
  TH1I h("", "", 100, 0, 1);
  double t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) h.Fill(*it++);
  t = (clock() - t) / CLOCKS_PER_SEC / n * 1e9;
  assert(distrib != 0 || h.GetSumOfWeights() == n);
  return t;
}

double compare_2d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  TH2I h("", "", 100, 0, 1, 100, 0, 1);
  double t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    const auto x = *it++;
    const auto y = *it++;
    h.Fill(x, y);
  }
  t = (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
  assert(distrib != 0 || h.GetSumOfWeights() == n / 2);
  return t;
}

double compare_3d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  TH3I h("", "", 100, 0, 1, 100, 0, 1, 100, 0, 1);
  double t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end;) {
    const auto x = *it++;
    const auto y = *it++;
    const auto z = *it++;
    h.Fill(x, y, z);
  }
  t = (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
  assert(distrib != 0 || h.GetSumOfWeights() == n / 3);
  return t;
}

double compare_6d(unsigned n, int distrib) {
  auto r = random_array(n, distrib);
  int bin[] = {10, 10, 10, 10, 10, 10};
  double min[] = {0, 0, 0, 0, 0, 0};
  double max[] = {1, 1, 1, 1, 1, 1};
  THnI h("", "", 6, bin, min, max);
  double t = clock();
  for (auto it = r.get(), end = r.get() + n; it != end; it += 6) h.Fill(it);
  t = (double(clock()) - t) / CLOCKS_PER_SEC / n * 1e9;
  TH1D* h1 = h.Projection(0);
  assert(distrib != 0 || h1->GetSumOfWeights() == n / 6);
  delete h1;
  return t;
}

int main(int argc, char** argv) {
  constexpr unsigned nfill = 6000000;
  for (int itype = 0; itype < 2; ++itype) {
    auto d = itype == 0 ? "uniform" : "normal ";
    printf("1D-root-%s %5.1f\n", d, compare_1d(nfill, itype));
    printf("2D-root-%s %5.1f\n", d, compare_2d(nfill, itype));
    printf("3D-root-%s %5.1f\n", d, compare_3d(nfill, itype));
    printf("6D-root-%s %5.1f\n", d, compare_6d(nfill, itype));
  }
}

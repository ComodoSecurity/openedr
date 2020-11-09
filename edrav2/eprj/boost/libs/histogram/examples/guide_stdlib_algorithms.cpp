// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

//[ guide_stdlib_algorithms

#include <boost/histogram.hpp>
#include <cassert>

#include <algorithm> // fill, any_of, min_element, max_element
#include <cmath>     // sqrt
#include <numeric>   // partial_sum, inner_product

int main() {
  using namespace boost::histogram;

  // make histogram that represents a probability density function (PDF)
  auto h1 = make_histogram(axis::regular<>(4, 1.0, 3.0));

  // use std::fill to set all counters to 0.25, including *flow cells
  std::fill(h1.begin(), h1.end(), 0.25);
  // reset *flow cells to zero
  h1.at(-1) = h1.at(4) = 0;

  // compute the cumulative density function (CDF), overriding cell values
  std::partial_sum(h1.begin(), h1.end(), h1.begin());

  assert(h1.at(-1) == 0.0);
  assert(h1.at(0) == 0.25);
  assert(h1.at(1) == 0.50);
  assert(h1.at(2) == 0.75);
  assert(h1.at(3) == 1.00);
  assert(h1.at(4) == 1.00);

  // use any_of to check if any cell values are smaller than 0.1,
  // and use indexed() to skip underflow and overflow cells
  auto h1_ind = indexed(h1);
  const auto any_small =
      std::any_of(h1_ind.begin(), h1_ind.end(), [](const auto& x) { return *x < 0.1; });
  assert(any_small == false); // underflow and overflow are zero, but skipped

  // find maximum element
  const auto max_it = std::max_element(h1.begin(), h1.end());
  assert(max_it == h1.end() - 2);

  // find minimum element
  const auto min_it = std::min_element(h1.begin(), h1.end());
  assert(min_it == h1.begin());

  // make second PDF
  auto h2 = make_histogram(axis::regular<>(4, 1.0, 4.0));
  h2.at(0) = 0.1;
  h2.at(1) = 0.3;
  h2.at(2) = 0.2;
  h2.at(3) = 0.4;

  // computing cosine similiarity: cos(theta) = A dot B / sqrt((A dot A) * (B dot B))
  const auto aa = std::inner_product(h1.begin(), h1.end(), h1.begin(), 0.0);
  const auto bb = std::inner_product(h2.begin(), h2.end(), h2.begin(), 0.0);
  const auto ab = std::inner_product(h1.begin(), h1.end(), h2.begin(), 0.0);
  const auto cos_sim = ab / std::sqrt(aa * bb);

  assert(std::abs(cos_sim - 0.78) < 1e-2);
}

//]

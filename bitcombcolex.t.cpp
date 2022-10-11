#include "bitcombcolex.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "util.h"

namespace t = testing;

MATCHER_P(SetBitCountIs, n, "") { return std::popcount(arg) == n; }

template<int n, int k>
void testBitCombColexNK()
{
    std::vector<uint32_t> sink;

    for (auto x: BitCombColex<n, k>{})
        sink.push_back(x);

    EXPECT_THAT(sink, t::SizeIs(combinations(n, k)));
    EXPECT_THAT(sink, t::Each(SetBitCountIs(k)));

    namespace r = std::ranges;
    EXPECT_TRUE(r::is_sorted(sink));
    EXPECT_EQ(r::adjacent_find(sink), sink.end()); // unique elements
}

template<int n, int... ints>
void testBitCombColexN(std::integer_sequence<int, ints...>)
{
    (testBitCombColexNK<n, ints + 1>(), ...);
}

template<int n>
void testBitCombColexN()
{
    testBitCombColexN<n>(std::make_integer_sequence<int, n>{});
}

// For given bitwidths test BitCombColex completely
TEST(BitCombColexTest, All)
{
    testBitCombColexN<1>();
    testBitCombColexN<2>();
    testBitCombColexN<3>();
    testBitCombColexN<4>();
    testBitCombColexN<7>();
    testBitCombColexN<10>();
}

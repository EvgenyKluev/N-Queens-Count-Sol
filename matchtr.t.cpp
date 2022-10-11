/* Check if MatchTr class behaves as expected for various inputs and
 * configuration parameters (but have to keep the space of parameters
 * small to limit compilation time).
 */
#include "matchtr.h"

#include <bit>
#include <cstdint>
#include <utility>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "cfg.h"
#include "util.h"

namespace t = testing;

enum class Case {
    belowMin,
    belowMax,
    belowMaxPlusMin,
    aboveMaxPlusMin,
};

template<int grpSize, int chunkSize, int sizeMod>
void testMatchTr(Case c)
{
    static constexpr int size = 24 + sizeMod;
    static constexpr int sizem1 = size - 1;
    uint64_t pat = 0xFEFEFEFEFEFEFEFE;

    const auto [patCnt, trSize] = [c]() -> std::pair<unsigned, unsigned> {
        switch (c)
        {
        case Case::belowMin: return {32u, 0u};
        case Case::belowMax: return  {48u, 1u};
        case Case::belowMaxPlusMin: return {grpSize * 64u + 32u, 1u};
        case Case::aboveMaxPlusMin: return {grpSize * 64u + 48u, 2u};
        }
        return {0u, 0u};
    }();
    uint64_t res = patCnt / 8;

    MatchTr<size, Cfg{0, chunkSize, grpSize, 40}> matchTr;
    EXPECT_EQ(matchTr.testPatternsSize(), 0u);
    EXPECT_EQ(matchTr.testTransposedSize(), 0u);

    for (unsigned i = 0; i != patCnt; ++i)
    {
        matchTr.appendPattern(pat & nLeastBits<uint64_t>(size));
        pat = std::rotl(pat, 1);
    }

    EXPECT_EQ(matchTr.testPatternsSize(), patCnt % (grpSize * 64u));
    EXPECT_EQ(matchTr.testTransposedSize(), patCnt / (grpSize * 64u));

    matchTr.closePatterns();
    EXPECT_EQ(matchTr.testPatternsSize(),
              (patCnt % (grpSize * 64u) == 32u)? 32u: 0u);
    EXPECT_EQ(matchTr.testTransposedSize(), trSize);
    EXPECT_EQ(matchTr.count(1u), res);
    EXPECT_EQ(matchTr.count(1u << sizem1), res);

    matchTr.clear();
    EXPECT_EQ(matchTr.testPatternsSize(), 0u);
    EXPECT_EQ(matchTr.testTransposedSize(), 0u);
    EXPECT_EQ(matchTr.count(1u), 0u);
    EXPECT_EQ(matchTr.count(1u << sizem1), 0u);
}

template<int grpSize, int chunkSize, int... ints>
void testMatchTrSize(std::integer_sequence<int, ints...>, Case c)
{
    (testMatchTr<grpSize, chunkSize, ints>(c), ...);
}

template<int grpSize, int... ints>
void testMatchTrChunk(std::integer_sequence<int, ints...>, Case c)
{
    (testMatchTrSize<grpSize, ints + 1>(
                std::make_integer_sequence<int, ints + 1>{}, c), ...);
}

template<int... ints>
void testMatchTrGrp(std::integer_sequence<int, ints...>, Case c)
{
    (testMatchTrChunk<(1 << (ints * 3))>(
                std::make_integer_sequence<int, 3>{}, c), ...);
}

TEST(MatchTrTest, BelowMin)
{
    testMatchTrGrp(std::make_integer_sequence<int, 2>{}, Case::belowMin);
}

TEST(MatchTrTest, BelowMax)
{
    testMatchTrGrp(std::make_integer_sequence<int, 2>{}, Case::belowMax);
}

TEST(MatchTrTest, BelowMaxPlusMin)
{
    testMatchTrGrp(std::make_integer_sequence<int, 2>{}, Case::belowMaxPlusMin);
}

TEST(MatchTrTest, AboveMaxPlusMin)
{
    testMatchTrGrp(std::make_integer_sequence<int, 2>{}, Case::aboveMaxPlusMin);
}

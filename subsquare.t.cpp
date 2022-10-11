#include "subsquare.h"

#include <algorithm>
#include <array>
#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "pack.h"
#include "qsymmetry.h"

namespace t = testing;
namespace r = std::ranges;

MATCHER_P(SetBitCountIs, n, "") { return std::popcount(arg) == n; }

TEST(SubsquareTest, ForCells)
{
    static constexpr int size = 5;
    const uint32_t rows = 0b10100;
    const int rPop = std::popcount(rows);
    Subsquare<size, QNoSymmetry, PackNothing> quarter;
    const auto factory = quarter.withRows(rows);
    std::vector<uint32_t> cList;

    quarter.forCells(factory, [&](const auto& cell) {
        cList.push_back(cell.columns);
    });

    EXPECT_THAT(cList, t::SizeIs(t::Le(combinations(size, rPop))));
    EXPECT_THAT(cList, t::Each(SetBitCountIs(rPop)));
    EXPECT_TRUE(r::is_sorted(cList));
    EXPECT_EQ(r::adjacent_find(cList), cList.end()); // unique elements
}

namespace {
using D2 = std::array<uint32_t, 2>;
MATCHER_P2(SBitIsZero, sBit, pos, "") { return (arg[pos] & sBit) == 0; }

auto hasZeroSBit(int whichDiag, uint32_t sBit)
{
    return [whichDiag, sBit](D2 d) {
        return (d[whichDiag] & sBit) == 0;
    };
}

void unfiltered(const std::vector<D2>& dList, const uint32_t sBit, bool)
{
    auto p0 = r::partition_point(dList, hasZeroSBit(0, sBit));
    EXPECT_TRUE(r::is_partitioned(dList.begin(), p0, hasZeroSBit(1, sBit)));
    EXPECT_TRUE(r::is_partitioned(p0, dList.end(),
                                  std::not_fn(hasZeroSBit(1, sBit))));
}

void filtered(const std::vector<D2>& dList, const uint32_t sBit, bool other)
{
    EXPECT_THAT(dList, t::Each(SBitIsZero(sBit, other)));
}

template<bool filtered, bool other = false>
void testSubsquare(auto diagTests)
{
    static constexpr int size = 8;
    const uint32_t sBit = 0b10000000;
    const uint32_t rowsN = 0b00000000;
    const uint32_t columnsN = 0b00000000;
    Subsquare<size, QNoSymmetry, PackNothing> quarter;
    const auto factoryN = quarter.withRows(rowsN);
    const auto ni = factoryN.makeCellInd(columnsN);
    const uint32_t rowsS = 0b10010101;
    const int rPop = std::popcount(rowsS);

    for (uint32_t columnsS = 0; columnsS != (uint32_t{1} << size); ++columnsS)
    {
        if (std::popcount(columnsS) != rPop)
            continue;

        const auto factoryS = quarter.withRows(rowsS);
        const auto si = factoryS.makeCellInd(columnsS);
        std::vector<D2> dList;

        quarter.forDiags<filtered, other>(ni, si, [&dList](const auto& d) {
            dList.push_back(d.second);
        });

        EXPECT_THAT(dList, t::Each(t::Each(SetBitCountIs(rPop))));
        EXPECT_TRUE(r::is_partitioned(dList, hasZeroSBit(0, sBit)));
        diagTests(dList, sBit, !other);
    }
}
}

TEST(SubsquareTest, ForDiagsUnfiltered)
{
    testSubsquare<false>(unfiltered);
}

TEST(SubsquareTest, ForDiagsFiltered0)
{
    testSubsquare<true, false>(filtered);
}

TEST(SubsquareTest, ForDiagsFiltered1)
{
    testSubsquare<true, true>(filtered);
}

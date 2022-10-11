#include "pack.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

namespace t = testing;
namespace r = std::ranges;

MATCHER_P(SetBitCountIs, n, "") { return std::popcount(arg) == n; }

void uniqueElements(const auto& container)
{
    EXPECT_EQ(r::adjacent_find(container), container.end());
}

enum class PackCase
{
    nothing,
    iter,
    columns,
};

template<class Pack, int bSize>
void testPack(PackCase pc)
{
    static constexpr uint32_t size = (1 << bSize);
    std::vector<uint32_t> indAll;

    for (uint32_t rows = 0; rows != size; ++rows)
    {
        const int rPop = std::popcount(rows);
        std::vector<uint32_t> cList;
        typename Pack::RowInfo rowInfo = Pack::getRowInfo(rows);

        Pack::forColumns(rowInfo, rows, [&](uint32_t index, uint32_t columns) {
            EXPECT_EQ(Pack::getColIndex(rowInfo, columns), index);
            cList.push_back(columns);
            indAll.push_back(index);
        });

        if (pc == PackCase::nothing)
        {
            EXPECT_THAT(cList, t::SizeIs(size));
        }
        else
        {
            EXPECT_THAT(cList, t::SizeIs(combinations(bSize, rPop)));
            EXPECT_THAT(cList, t::Each(SetBitCountIs(rPop)));
        }

        EXPECT_TRUE(r::is_sorted(cList));
        uniqueElements(cList);
    }

    if (pc != PackCase::iter)
    {
        EXPECT_THAT(indAll, t::SizeIs(Pack::getLastIndex()));
        EXPECT_THAT(indAll, t::SizeIs(indAll.back() + 1));
    }

    EXPECT_TRUE(r::is_sorted(indAll));
    uniqueElements(indAll);
    EXPECT_EQ(indAll.front(), 0u);
}

TEST(PackTest, PackNothingFor)
{
    static constexpr int bSize = 8;
    testPack<PackNothing<bSize>, bSize>(PackCase::nothing);
}

TEST(PackTest, PackIterFor)
{
    static constexpr int bSize = 8;
    testPack<PackIter<bSize>, bSize>(PackCase::iter);
}

TEST(PackTest, PackColumnsFor)
{
    static constexpr int bSize = 8;
    testPack<PackColumns<bSize>, bSize>(PackCase::columns);
}

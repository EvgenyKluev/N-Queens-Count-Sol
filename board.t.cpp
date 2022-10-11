// Check if Board class behaves as expected in several simple cases
#include "board.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "util.h"

namespace t = testing;

template<int size>
void testEmpty()
{
    Board<size> b;
    EXPECT_EQ(b.getFreeColumns(0), nLeastBits<uint32_t>(size));
    EXPECT_EQ(b.rows(), 0u);
    EXPECT_EQ(b.columns(), 0u);
    EXPECT_EQ(b.diags(0), 0u);
    EXPECT_EQ(b.diags(1), 0u);
}

TEST(BoardTest, Empty)
{
    testEmpty<1>();
    testEmpty<2>();
    testEmpty<9>();
    testEmpty<16>();
}

TEST(BoardTest, Queens1of1)
{
    Board<1> empty;
    auto b = empty.addQueen(0, 1);
    EXPECT_EQ(b.getFreeColumns(0), 0u);
    EXPECT_EQ(b.rows(), 1u);
    EXPECT_EQ(b.columns(), 1u);
    EXPECT_EQ(b.diags(0), 1u);
    EXPECT_EQ(b.diags(1), 1u);
}

TEST(BoardTest, Queens1of8)
{
    Board<8> empty;
    auto b = empty.addQueen(2, 0b100);
    EXPECT_EQ(b.getFreeColumns(6), 0b10111011u);
    EXPECT_EQ(b.rows(), 0b100u);
    EXPECT_EQ(b.columns(), 0b100u);
    EXPECT_EQ(b.diags(0), 1u << 7);
    EXPECT_EQ(b.diags(1), 0b10000u);
}

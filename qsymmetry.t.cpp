/* Check if qsymmetry classes behave as expected in several simple cases;
 * for QSymmetry only 2 of 8 symmetry cases checked.
 */
#include "qsymmetry.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

namespace t = testing;

#include "pack.h"

TEST(QSymmetryTest, QNoSymmetryUniq)
{
    EXPECT_TRUE((QNoSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x100}, uint32_t{0x40})));
}

TEST(QSymmetryTest, QRowSymmetryUniq)
{
    EXPECT_TRUE((QRowSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{4}, uint32_t{8})));
    EXPECT_TRUE((QRowSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x10}, uint32_t{8})));

    EXPECT_FALSE((QRowSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x40}, uint32_t{8})));
}

TEST(QSymmetryTest, QSymmetryUniq)
{
    EXPECT_TRUE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{4}, uint32_t{8})));
    EXPECT_TRUE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{4}, uint32_t{0x10})));

    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x40}, uint32_t{8})));
    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x40}, uint32_t{0x10})));
    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{4}, uint32_t{0x80})));
    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x10}, uint32_t{0x80})));
    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{8}, uint32_t{4})));
    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x10}, uint32_t{4})));
    EXPECT_FALSE((QSymmetry<9, PackNothing<9>>::isUniq(
                    uint32_t{0x40}, uint32_t{0x10})));
}

TEST(QSymmetryTest, QNoSymmetryCellInd)
{
    using Q = QNoSymmetry<9, PackNothing<9>>;
    const Q::CellFactory cf(0x100);
    const auto ci = cf.makeCellInd(0x40);

    EXPECT_TRUE(Q::filter<false>(ci, 1));
    EXPECT_TRUE(Q::filter<true>(ci, 1));

    EXPECT_FALSE(Q::reflect(ci));

    const Q::Diagonals d {1, 2};
    EXPECT_EQ(Q::fix(d, ci), d);
}

TEST(QSymmetryTest, QRowSymmetryCellInd)
{
    using Q = QRowSymmetry<9, PackNothing<9>>;
    const Q::CellFactory cf(0x100);
    const auto ci = cf.makeCellInd(0x40);

    EXPECT_TRUE(Q::filter<false>(ci, 1));
    EXPECT_TRUE(Q::filter<true>(ci, 1));

    EXPECT_TRUE(Q::reflect(ci));

    const Q::Diagonals d {1, 2};
    const Q::Diagonals dSwap {2, 1};
    EXPECT_EQ(Q::fix(d, ci), dSwap);
}

TEST(QSymmetryTest, QSymmetryHV)
{
    using Q = QSymmetry<9, PackNothing<9>>;
    const Q::CellFactory cf(0x100);
    const auto ci = cf.makeCellInd(0x40);

    EXPECT_FALSE(Q::filter<false>(ci, 1));
    EXPECT_FALSE(Q::filter<true>(ci, 1));
    EXPECT_TRUE(Q::filter<false>(ci, 0x100));
    EXPECT_TRUE(Q::filter<true>(ci, 0x100));

    EXPECT_FALSE(Q::reflect(ci));

    const Q::Diagonals d {1, 2};
    const Q::Diagonals dRev {0x10000, 0x8000};
    EXPECT_EQ(Q::fix(d, ci), dRev);
}

TEST(QSymmetryTest, QSymmetryHVD)
{
    using Q = QSymmetry<9, PackNothing<9>>;
    const Q::CellFactory cf(0x40);
    const auto ci = cf.makeCellInd(0x100);

    EXPECT_TRUE(Q::filter<false>(ci, 1));
    EXPECT_FALSE(Q::filter<true>(ci, 1));
    EXPECT_TRUE(Q::filter<false>(ci, 0x100));
    EXPECT_TRUE(Q::filter<true>(ci, 0x100));

    EXPECT_FALSE(Q::reflect(ci));

    const Q::Diagonals d {1, 2};
    const Q::Diagonals dRev {1, 0x8000};
    EXPECT_EQ(Q::fix(d, ci), dRev);
}

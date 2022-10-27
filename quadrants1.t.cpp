/* Test Quadrants1's symmetries and control flow (start with getting
 * stream of diagonals from FakeQuarter and follow execution path until
 * feeding diagonals to MockSieve). As Quadrants1's only function is counting
 * n-queens solutions (and this is already tested in main application),
 * the only reasonable way to test it is to surround it with mocks and fakes.
 */
#include "quadrants1.h"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "gmock/gmock.h"

#include "divider.h"

namespace t = testing;

using D2 = std::array<uint32_t, 2>;
using D4 = std::pair<D2, D2>;
using DP = std::pair<uint32_t, uint32_t>;

struct MockStart
{
    using BitComb = std::array<uint32_t, 1>;
    MOCK_METHOD(BitComb, getBitComb, (), (const));
    MOCK_METHOD(uint32_t, stretchRows, (uint32_t bits), (const));
    MOCK_METHOD(uint32_t, getFreeRows, (), (const));
    MOCK_METHOD(uint32_t, getColumns, (), (const));
    MOCK_METHOD(bool, matchDiagsE, (const D4& d), (const));
    MOCK_METHOD(bool, matchDiagsW, (const D4& d), (const));

    template <int offset>
    bool matchDiags(const D4& d) const
    {
        if (offset)
            return matchDiagsW(d);
        else
            return matchDiagsE(d);
    }

    static constexpr bool internalSymmetry()
    {
        return true;
    }

    static constexpr bool diagSymmetry()
    {
        return true;
    }

    static constexpr bool filterDiag()
    {
        return false;
    }
};

struct MockSieve
{
    MOCK_METHOD(void, appendPattern, (const DP& d));
    MOCK_METHOD(uint64_t, count, (const DP d), (const));
    MOCK_METHOD(void, clear, ());
};

struct FakeCell
{
    uint32_t columns;
};

struct FakeCellInd
{
    std::vector<D4>* eastD;
    D4* westD;
};

// Made as mock to pass two pointers (via FakeCellInd) to FakeQuarter
struct MockCellFactory
{
    MOCK_METHOD(FakeCellInd, makeCellInd, (FakeCell c), (const));
    MOCK_METHOD(FakeCellInd, makeCellInd, (uint32_t ci), (const));
};

struct FakeQuarter
{
    auto withRows(uint32_t) const
    {
        return t::NiceMock<MockCellFactory>{};
    }

    void forCells(const MockCellFactory&, const auto& action) const
    {
        action(FakeCell{0});
    }

    template<bool filterDiags, bool other>
    void forDiags(FakeCellInd first,
                  FakeCellInd,
                  const auto& action) const
    {
        if (other)
        {
            action(*first.westD);
        }
        else
        {
            for (const auto& x: *first.eastD)
                action(x);
        }
    }

    void setSBit(int)
    {}

    static bool handlesSpecialBit()
    {
        return false;
    }
};

struct FakeThread
{
    void sync() const
    {}

    [[nodiscard]] bool accepted()
    {
        return !rejected();
    }

    [[nodiscard]] bool rejected()
    {
        return false;
    }
};

struct FakeFreeze
{
    MockSieve* ptr;

    const MockSieve& getObj() const
    {
        return *ptr;
    }

    void freeze(auto*)
    {}

    void clear()
    {
        ptr->clear();
    }

    void shrink()
    {}
};

struct FakeContext
{
    const MockSieve& counter() const
    {
        return sink;
    }

    void sync() const
    {}

    t::NiceMock<MockStart> start;
    FakeThread* thread;
    t::NiceMock<MockSieve> sink;
    FakeFreeze* freeze;
    Divider divider;
};

class Quadrants1Test: public t::Test
{
protected:
    void SetUp() override
    {
        f.ptr = &env.sink;

        ON_CALL(env.start, getFreeRows)
                .WillByDefault(t::Return(0xFF));
        EXPECT_CALL(env.start, getBitComb())
                .WillOnce(t::Return(bitComb));
        EXPECT_CALL(env.start, matchDiagsE)
                .WillOnce(t::Return(false))
                .WillRepeatedly(t::Return(true));
        EXPECT_CALL(env.start, matchDiagsW)
                .WillOnce(t::Return(true));

        EXPECT_CALL(env.sink, count)
                .WillOnce(t::Return(1));
        EXPECT_CALL(env.sink, clear)
                .Times(1);

        t::DefaultValue<FakeCellInd>::Set({&eastD, &westD});
    }

    std::array<uint32_t, 1> bitComb{};
    FakeThread t;
    FakeFreeze f;
    FakeContext env {t::NiceMock<MockStart>{}, &t,
                t::NiceMock<MockSieve>{}, &f, Divider{}};
    Quadrants1<8, FakeQuarter> quadrants;

    D4 westD {{0, 8}, {8, 0}};
    std::vector<D4> eastD {
        {{1, 0}, {16, 0}},// rejected by matchDiags
        {{1, 0}, {16, 0}},// rejected by matchQuarters
        {{0, 0}, {0, 8}}, // rejected by bothDiagsEmpty
        {{8, 0}, {0, 0}}, // rejected by bothDiagsEmpty
        {{8, 0}, {0, 8}}, // rejected by bothDiagsEmpty
        {{0, 0}, {0, 0}}, // passed to sink
    };
};

TEST_F(Quadrants1Test, DiagsSymmetry4)
{
    EXPECT_CALL(env.sink, appendPattern)
            .Times(1);

    EXPECT_EQ(quadrants(env), 4u);
}

TEST_F(Quadrants1Test, DiagsSymmetry2A)
{
    EXPECT_CALL(env.sink, appendPattern)
            .Times(1);

    westD = {{0, 0}, {8, 0}};
    EXPECT_EQ(quadrants(env), 2u);
}

TEST_F(Quadrants1Test, DiagsSymmetry2B)
{
    EXPECT_CALL(env.sink, appendPattern)
            .Times(1);

    westD = {{0, 8}, {0, 0}};
    EXPECT_EQ(quadrants(env), 2u);
}

TEST_F(Quadrants1Test, DiagsSymmetry1ButEastWest2)
{
    EXPECT_CALL(env.sink, appendPattern)
            .Times(1);
    ON_CALL(env.start, stretchRows)
            .WillByDefault(t::Return(0xFF));

    westD = {{0, 0}, {0, 0}};
    EXPECT_EQ(quadrants(env), 2u);
}

TEST_F(Quadrants1Test, DiagsSymmetry1ButNorthSouth2)
{
    EXPECT_CALL(env.sink, appendPattern)
            .Times(1);
    ON_CALL(env.start, stretchRows)
            .WillByDefault(t::Return(0x0F));

    westD = {{0, 0}, {0, 0}};
    EXPECT_EQ(quadrants(env), 2u);
}

TEST_F(Quadrants1Test, DiagsSymmetry0)
{
    EXPECT_CALL(env.sink, appendPattern)
            .Times(0);

    eastD.pop_back();
    (void)quadrants(env);
}

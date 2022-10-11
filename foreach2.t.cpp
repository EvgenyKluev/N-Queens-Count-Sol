#include "foreach2.h"

#include <array>
#include <ranges>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

namespace t = testing;

class ForEach2Test: public t::Test
{
protected:
    void process(ssize_t count)
    {
        auto func = [i = 0](int& a) mutable { a += i++; };
        forEach2(r1, r2, count, func);
    }
    std::array<int, 2> r1 {1, 6};
    std::array<int, 2> r2 {3, 4};
};

TEST_F(ForEach2Test, IterateBoth)
{
    process(4);
    EXPECT_THAT(r1, t::ElementsAre(1, 7));
    EXPECT_THAT(r2, t::ElementsAre(5, 7));
}

TEST_F(ForEach2Test, IterateFirst)
{
    process(2);
    EXPECT_THAT(r1, t::ElementsAre(1, 7));
    EXPECT_THAT(r2, t::ElementsAre(3, 4));
}

TEST_F(ForEach2Test, IterateNeither)
{
    auto func = [](int&) { FAIL() << "this function should not be called"; };
    std::ranges::empty_view<int> empty;
    forEach2(empty, empty, 4, func);
}

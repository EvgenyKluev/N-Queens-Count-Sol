#include "sieve.h"

#include <bit>
#include <utility>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "bmintrin.h"

namespace t = testing;

template<int size, Cfg cfg>
struct FakeMatch
{
    void appendPattern(uint64_t) {}
    void closePatterns() {}
    uint64_t count(uint64_t) const { return 1u; }
    void clear() {}
    void shrink() {}
    void passTo(FakeMatch<size, cfg>&) {}
    void prefetch(uint64_t) const {}
};

template<bool bmi, int cut>
void testSieveCount()
{
    Sieve<FakeMatch, Cfg{cut, 5, 8, 40, bmi}, 17, 0> sieve;
    sieve.setHoles({0x1FFFF, 0x1FF00}); // force bmi/nonbmi versions be the same

    for (uint32_t i = 0; i != (uint32_t{1} << cut); ++i)
    {
        EXPECT_EQ(sieve.count(std::pair{0u, i << (8 - cut)}),
                  static_cast<uint64_t>(1 << (cut - std::popcount(i))))
                << "for cut = " << cut << ", i = " << i;
    }
}

template<bool bmi, int... ints>
void testSieveCount(std::integer_sequence<int, ints...>)
{
    (testSieveCount<bmi, ints>(), ...);
}

template<bool bmi>
void testSieveCount()
{
    testSieveCount<bmi>(std::make_integer_sequence<int, 3>{});
}

TEST(SieveTest, Count)
{
    testSieveCount<false>();

    if constexpr (isBMAvail)
        testSieveCount<true>();
}

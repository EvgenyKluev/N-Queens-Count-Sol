#include "util.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

namespace t = testing;

template <typename Bits>
void nLeastBitsTest()
{
    EXPECT_EQ(nLeastBits<Bits>(0), Bits{0});
    EXPECT_EQ(nLeastBits<Bits>(1), Bits{1});
    EXPECT_EQ(nLeastBits<Bits>(2), Bits{3});
    EXPECT_EQ(nLeastBits<Bits>(31), Bits{0x7FFFFFFF}); // max for uint32
}

TEST(UtilTest, NLeastBits)
{
    nLeastBitsTest<uint32_t>();
    nLeastBitsTest<uint64_t>();
}

TEST(UtilTest, RevBitsSlow)
{
    EXPECT_EQ(revBitsSlow<1>(0), uint32_t{0}); // min width
    EXPECT_EQ(revBitsSlow<1>(1), uint32_t{1}); // min width
    EXPECT_EQ(revBitsSlow<2>(0b10), uint32_t{0b01}); // close to min width
    EXPECT_EQ(revBitsSlow<7>(0b1010011), uint32_t{0b1100101});
    EXPECT_EQ(revBitsSlow<7>(0b1011101), uint32_t{0b1011101}); // symmetrical
    EXPECT_EQ((revBitsSlow<15, 16>(0x7316)), uint32_t{0x68CE} >> 1);
    EXPECT_EQ((revBitsSlow<16, 16>(0xF316)), uint32_t{0x68CF}); // max width
}

template <int srcBits, int tblBits>
void revBitsOne(const uint32_t src)
{
    const auto srcFixed = src & nLeastBits<uint32_t>(srcBits);
    EXPECT_EQ((revBits<srcBits, tblBits>(srcFixed)), revBitsSlow<srcBits>(src))
            << "srcBits = " << srcBits << " tblBits = " << tblBits;
}

template <int srcBits, int tblBits>
void revBitsCase()
{
    revBitsOne<srcBits, tblBits>(uint32_t{0});
    revBitsOne<srcBits, tblBits>(~uint32_t{0});
    revBitsOne<srcBits, tblBits>(uint32_t{0b10101001000100000101011011101111});
    revBitsOne<srcBits, tblBits>(uint32_t{0xC6A2F3B1});
}

template <int tblBits>
void revBitsTest()
{
    revBitsCase<(tblBits + 1) / 2, tblBits>();     // srcBits < tblBits
    revBitsCase<tblBits, tblBits>();               // srcBits = tblBits
    revBitsCase<tblBits + tblBits / 2, tblBits>(); // srcBits < 2 * tblBits
    revBitsCase<tblBits * 2, tblBits>();           // srcBits = 2 * tblBits
    revBitsCase<std::min(tblBits * 2 + 1, 32), tblBits>(); // = 2 * tblBits + 1
    revBitsCase<31, tblBits>();                    // srcBits > 2 * tblBits + 1
}

TEST(UtilTest, RevBits)
{
    revBitsTest<1>();
    revBitsTest<2>();
    revBitsTest<8>();
    revBitsTest<12>();
}

TEST(UtilTest, Factorial)
{
    EXPECT_EQ(factorial(0), 1u);
    EXPECT_EQ(factorial(1), 1u);
    EXPECT_EQ(factorial(2), 2u);
    EXPECT_EQ(factorial(3), 6u);
    EXPECT_EQ(factorial(4), 24u);
    EXPECT_EQ(factorial(5), 120u);
    EXPECT_EQ(factorial(10), 3628800u);
}

TEST(UtilTest, Combinations)
{
    EXPECT_EQ(combinations(1, 1), 1u);
    EXPECT_EQ(combinations(2, 1), 2u);
    EXPECT_EQ(combinations(3, 1), 3u);
    EXPECT_EQ(combinations(3, 2), 3u);
    EXPECT_EQ(combinations(4, 2), 6u);
    EXPECT_EQ(combinations(10, 5), 252u);
}

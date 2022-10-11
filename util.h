#ifndef UTIL_H
#define UTIL_H

#include <array>
#include <bit>
#include <cstdint>
#include <utility>

// Assume n is strictly less than the number of bits in Bits
template <typename Bits>
requires (std::same_as<Bits, uint32_t> || std::same_as<Bits, uint64_t>)
inline constexpr Bits nLeastBits(int n)
{
    return (Bits{1} << n) - 1;
}

// See "Matters Computational" by Jörg Arndt, section 1.14.2
// x may exceed range defined by srcBits
template<int srcBits, int bitWidth = 32>
inline constexpr uint32_t revBitsSlow(uint32_t x)
{
    static_assert(0 < srcBits && srcBits <= bitWidth);
    static_assert(std::has_single_bit(unsigned{bitWidth}));

    int s = bitWidth / 2;
    auto m = nLeastBits<uint32_t>(s);

    while (s)
    {
        x = ((x & m) << s) ^ ((x & (~m)) >> s);
        s >>= 1;
        m ^= m << s;
    }

    return x >> (bitWidth - srcBits);
}

/* Second template parameter (tblBits) is assumed to be the same for different
 * function invokations. src must not exceed range defined by srcBits */
template<int srcBits, int tblBits>
inline uint32_t revBits(uint32_t src)
{
    static_assert(0 < srcBits && srcBits <= 32);
    static_assert(0 < tblBits && tblBits <= 16);

    using Table = std::array<uint16_t, (1 << tblBits)>;
    static constexpr Table table = [] {
        Table t;
        for (uint32_t x = 0; x != t.size(); ++x)
            t[x] = static_cast<uint16_t>(revBitsSlow<tblBits, 16>(x));
        return t;
    }();

    static constexpr uint32_t mask = nLeastBits<uint32_t>(tblBits);

    if (srcBits <= tblBits)
    {
        return table[src] >> (tblBits - srcBits);
    }
    else if (srcBits <= 2 * tblBits)
    {
        static constexpr int off = srcBits - tblBits;
        return (uint32_t{table[src & mask]} << off)
                       | table[(src >> off) & mask];
    }
    else if (srcBits == 2 * tblBits + 1)
    {
        static constexpr int off = srcBits - tblBits;
        return (uint32_t{table[src & mask]} << off)
                       | (src & (1 << tblBits))
                       | table[(src >> off) & mask];
    }
    else
    {
        return revBitsSlow<srcBits>(src);
    }
}

constexpr uint64_t factorial(const int n)
{
    uint64_t fct = 1;
    for (int i = 2; i <= n; ++i)
        fct *= i;
    return fct;
}

constexpr uint32_t combinations(const int n, const int k)
{
    return static_cast<uint32_t>(
        factorial(n) / factorial(k) / factorial(n - k));
}

#endif // UTIL_H

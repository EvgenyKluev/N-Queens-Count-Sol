#ifndef BITCOMBCOLEX_H
#define BITCOMBCOLEX_H

#include <cstdint>

#include "util.h"

/* Bit combinations generator, n bits total, k bits nonzero.
 * Co-lexicographic (colex) order.
 * See "Matters Computational" by Jörg Arndt, section 1.24.1
 * Implemented to be used in range-based for loop, no proper iterators/ranges.
 */
template<int n, int k>
requires(0 < k && k <= n && n < 32)
class BitCombColexIt
{
public:
    using value_type = uint32_t;
    struct Sentinel {};

    constexpr value_type operator*() const
    {
        return value_;
    }

    constexpr BitCombColexIt& operator++()
    {
        next();
        return *this;
    }

    constexpr bool operator!=(Sentinel) const
    {
        return notAtEnd();
    }

    // Set up the first combination of k bits,
    // i.e. 00..001111..1 (k low bits set)
    constexpr BitCombColexIt()
        : value_{nLeastBits<value_type>(k)}
    {}

private:
    constexpr void next()
    {
        const value_type lowestSetBit = value_ & -value_;
        value_ += lowestSetBit; // replace the lowest block by "1" to the left
        value_type lowestBlock = (value_ & -value_) - lowestSetBit;

        while ((lowestBlock & 1) == 0) // move block to low end of word
            lowestBlock >>= 1;

        value_ |= (lowestBlock >> 1);  // need one bit less of low block
    }

    constexpr bool notAtEnd() const
    {
        return (value_ & endMarker_) == 0;
    }

    static constexpr value_type endMarker_ = value_type{1} << n;
    value_type value_;
};

template<int n, int k>
class BitCombColex
{
public:
    constexpr BitCombColexIt<n, k> begin() const
    {
        return {};
    }

    constexpr BitCombColexIt<n, k>::Sentinel end() const
    {
        return {};
    }
};

#endif // BITCOMBCOLEX_H

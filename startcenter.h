#ifndef STARTCENTER_H
#define STARTCENTER_H

#include <cstdint>

#include "cfg.h"
#include "sieve.h"
#include "util.h"

// Starting position for odd-sized case where one queen is placed in the center.
template <int size, // assume odd size
          template<int, Cfg> class Match,
          template<int...> class BitComb,
          Cfg cfg>
class StartCenter
{
public:
    using Sieve_ = Sieve<Match, cfg, size - 2, 1>;

    auto getBitComb() const
    {
        return BitComb<size - 1, halfSize_>{};
    }

    uint32_t stretchRows(uint32_t bits) const
    {
        static constexpr uint32_t mask = centerBit_ - 1;
        return ((bits & ~mask) << 1) | (bits & mask);
    }

    uint32_t getFreeRows() const
    {
        return allRC_ & ~centerBit_;
    }

    uint32_t getColumns() const
    {
        return centerBit_;
    }

    template <int offset>
    bool matchDiags(const auto& diags) const
    {
        static constexpr int flip = (offset != 0);
        static constexpr auto middle = uint32_t{1} << (halfSize_ - 1);

        return (diags.first[0 ^ flip] & middle) == 0
            && (diags.second[1 ^ flip] & middle) == 0;
    }

    static constexpr bool internalSymmetry()
    {
        return true;
    }

    static constexpr bool diagSymmetry()
    {
        return false;
    }

    static constexpr bool filterDiag()
    {
        return true;
    }

private:
    static constexpr uint32_t allRC_ = nLeastBits<uint32_t>(size);
    static constexpr int halfSize_ = size / 2;
    static constexpr uint32_t centerBit_ = uint32_t{1} << halfSize_;
};

#endif // STARTCENTER_H

#ifndef STARTEMPTY_H
#define STARTEMPTY_H

#include <cstdint>

#include "cfg.h"
#include "sieve.h"
#include "util.h"

// Starting position for even-sized case where there are no additional queens.
template <int size, // assume even size
          template<int, Cfg> class Match,
          template<int...> class BitComb,
          Cfg cfg>
class StartEmpty
{
public:
    using Sieve_ = Sieve<Match, cfg, size - 1, 1>;

    auto getBitComb() const
    {
        return BitComb<size, size / 2>{};
    }

    uint32_t stretchRows(uint32_t bits) const
    {
        return bits;
    }

    uint32_t getFreeRows() const
    {
        return allRC_;
    }

    uint32_t getColumns() const
    {
        return 0;
    }

    template <int offset>
    bool matchDiags(const auto&) const
    {
        return true;
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

private:
    static constexpr uint32_t allRC_ = nLeastBits<uint32_t>(size);
};

#endif // STARTEMPTY_H

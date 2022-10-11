/* Contains 2 classes: Start2D: starting position for odd-sized case where
 * two queens are placed in middle row/column; and Start1D: special case
 * of Start2D where one of these queens is exactly at south border.
 */
#ifndef START2D_H
#define START2D_H

#include <array>
#include <cstdint>

#include "cfg.h"
#include "sieve.h"
#include "util.h"

template<int size,
         template<int, Cfg> class Match,
         template<int...> class BitComb,
         Cfg cfg>
requires(size > 4) // assume odd size
class Start2D
{
public:
    using Sieve_ = Sieve<Match, cfg, size - 2, 2>;

    auto getBitComb() const
    {
        return BitComb<size - 2, halfSize_>{};
    }

    uint32_t stretchRows(uint32_t bits) const
    {
        static constexpr uint32_t lm = centerBit_ - 1;
        const uint32_t mm = ((uint32_t{1} << (row_ - 1)) - 1) ^ lm;
        const uint32_t hm = ~(lm | mm);
        return ((bits & hm) << 2) | ((bits & mm) << 1) | (bits & lm);
    }

    uint32_t getFreeRows() const
    {
        return freeRows_;
    }

    uint32_t getColumns() const
    {
        return columns_;
    }

    template <int offset>
    bool matchDiags(const auto& diags) const
    {
        return qMatch<offset + qOffset_>(diags.first[0],  diags_[0])
            && qMatch<offset           >(diags.second[0], diags_[0])
            && qMatch<offset           >(diags.first[1],  diags_[1])
            && qMatch<offset + qOffset_>(diags.second[1], diags_[1]);
    }

    static constexpr bool internalSymmetry()
    {
        return false;
    }

    static constexpr bool diagSymmetry()
    {
        return false;
    }

    static constexpr bool filterDiag()
    {
        return true;
    }

    uint64_t forCR(auto& env, auto& quad)
    {
        uint64_t res = 0;

        for (int col = halfSize_ + 1; col != size - 2; ++col)
        {
            quad.setSBit(env, col - 1);

            for (int row = col + 1; row != size - 1; ++row)
            {
                setColumnRow(col, row);
                env.sink.setHoles(mkHoles());
                res += 8 * quad(env);
            }
        }

        return res;
    }

protected:
    template <int offset>
    static bool qMatch(const uint64_t q, const uint64_t d)
    {
        return ((q << offset) & d) == 0;
    }

    void setColumnRow(const int col, const int row)
    {
        uint32_t columns1 = uint32_t{1} << col;
        columns_ = columns1 | centerBit_;
        row_ = row;
        freeRows_ = stretchRows(nLeastBits<uint32_t>(size - 2));
        const auto cBit = uint64_t{columns1} << halfSize_;
        diags_[0] = cBit | (uint64_t{1} << (3 * halfSize_ - row_));
        diags_[1] = cBit | (uint64_t{1} << (halfSize_ + row_));
    }

    std::array<uint64_t, 2> mkHoles() const
    {
        return {diags_[0] >> qOffset_, diags_[1] >> qOffset_};
    }

    static constexpr uint32_t allRC_ = nLeastBits<uint32_t>(size);
    static constexpr int halfSize_ = size / 2;
    static constexpr int qOffset_ = (size + 1) / 2;
    static constexpr uint32_t centerBit_ = uint32_t{1} << halfSize_;
    int row_;
    uint32_t freeRows_;
    uint32_t columns_;
    std::array<uint64_t, 2> diags_;
};

template <int size,
          template<int, Cfg> class Match,
          template<int...> class BitComb,
          Cfg cfg>
class Start1D: public Start2D<size, Match, BitComb, cfg>
{
public:
    using Sieve_ = Sieve<Match, cfg, size - 2, 1>;

    static constexpr bool filterDiag()
    {
        return false;
    }

    uint64_t forCR(auto& env, const auto& quad)
    {
        uint64_t res = 0;

        for (int col = this->halfSize_ + 1; col != size - 1; ++col)
        {
            this->setColumnRow(col, size - 1);
            env.sink.setHoles(this->mkHoles());
            res += 8 * quad(env);
        }

        return res;
    }
};

#endif // START2D_H

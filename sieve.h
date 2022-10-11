#ifndef SIEVE_H
#define SIEVE_H

#include <array>
#include <cstdint>
#include <ranges>

#include "bmintrin.h"
#include "cfg.h"
#include "util.h"

/* Optimizes bitset matcher by (1) cutting out several bits (cuts_) to use them
 * for indexing instead of matching and (2) possibly grouping all significant
 * bits (holes_) together so that matcher would perform less computations.
 *
 * Several bitset matchers are arranged in an array, every incoming pattern
 * is passed directly to corresponding matcher, then every incoming item
 * is arranged to visit only matchers that correspond to its bit pattern
 * (that's why this is named Sieve).
 *
 * Template parameters:
 * Match - class that performs bitset matching
 * cfg - configuration parameters
 * halfLen - 1/2 of number of input bits (includes holes)
 * holes - 1/2 of number of insignificant input bits that may be ignored
 */
template <template<int, Cfg> class Match, Cfg cfg, int halfLen, int holes>
class Sieve
{
public:
    // Specify which input bits should be ignored
    void setHoles(const std::array<uint64_t, 2> h)
    {
        uint64_t hcat = (h[1] << halfLen) | h[0];
        cuts_ = mkCuts(~hcat);
        holes_ = ~(hcat | cuts_);
    }

    void appendPattern(const auto& diags)
    {
        const auto sb = stitch(diags);
        match_[cutMask_ & ~sb.index].appendPattern(sb.bits);
    }

    // Should be called when stream of patterns ends.
    void closePatterns()
    {
        for (auto& m: match_)
            m.closePatterns();
    }

    uint64_t count(const auto& diags) const
    {
        uint64_t total = 0;
        const auto sb = stitch(diags);
        auto next = [&sb](auto i) { return (i + 1) | sb.index; };

        for (uint64_t i = sb.index; i != cutMask_; i = next(i))
        {
            match_[next(i)].prefetch(sb.bits);
            total += match_[i].count(sb.bits);
        }

        total += match_[cutMask_].count(sb.bits);
        return total;
    }

    /* Pull patterns from matchers pointed by ptrs and pass them to local
     * matchers. This is the only thread-aware method of this class. It
     * divides work between threads so that each thread gets a disjoint subset
     * of matchers array.
     */
    void pull(const std::ranges::range auto ptrs, auto* thread)
    {
        for (uint32_t i = 0; i != cutSize_; ++i)
        {
            if (thread->rejected())
                continue;

            if (i == 0)
            {
                holes_ = ptrs[0]->holes_;
                cuts_ = ptrs[0]->cuts_;
            }

            for (auto* p: ptrs)
                p->match_[i].passTo(match_[i]);

            match_[i].closePatterns();
        }
    }

    void clear()
    {
        for (auto& m: match_)
            m.clear();
    }

    void shrink()
    {
        for (auto& m: match_)
            m.shrink();
    }

private:
    struct SrcBits
    {
        uint64_t bits;
        uint64_t index;
    };

    SrcBits stitch(const auto& diags) const
    {
        return splitBits(((uint64_t{diags.second} & halfMask_) << halfLen)
                        | (uint64_t{diags.first} & halfMask_), diags);
    }

    /* Divides input bits into two groups: bits that should go to matcher(s),
     * and bits that should be used for matcher indexing. If BMI intrinsics
     * are available, uses optimal bit positions to cut bits for indexing;
     * also passes only significant bits to matcher(s). If no BMI intrinsics
     * are available, uses for indexing some bit positions that always store
     * significant bits; passes all (uncompressed) bits to matcher(s).
     */
    static_assert(!cfg.bmiIntrin || isBMAvail);
    SrcBits splitBits(const uint64_t r, const auto& diags) const
    {
        if constexpr (cfg.bmiIntrin)
            return {bext(r, holes_), cut_? bext(r, cuts_): 0};
        else
            return {r, (diags.second >> (halfLen / 2 - cut_)) & cutMask_};
    }

    // Determine optimal bit positions to cut bits for indexing
    static uint64_t mkCuts(const uint64_t bits)
    {
        uint64_t res = 0;
        int cutCnt = cut_;

        std::array<uint64_t, 4> bit {
            center_, center2_, center_ >> 1, center2_ >> 1};

        for (int toggle = 0; cutCnt; toggle = (toggle + 1) % 4)
        {
            if (bit[toggle] & bits)
            {
                res |= bit[toggle];
                --cutCnt;
            }

            if (toggle < 2)
                bit[toggle] <<= 1;
            else
                bit[toggle] >>= 1;
        }

        return res;
    }

    static constexpr uint64_t halfMask_ = nLeastBits<uint64_t>(halfLen);
    static constexpr int cut_ = cfg.sieveCuts;
    static constexpr uint32_t cutMask_ = nLeastBits<uint32_t>(cut_);
    static constexpr uint32_t cutSize_ = uint32_t{1} << cut_;
    static constexpr uint64_t center_ = uint32_t{1} << (halfLen / 2);
    static constexpr uint64_t center2_ = center_<< halfLen;
    static constexpr uint64_t centerBits_ = center2_ | center_;
    static constexpr int ml = halfLen * 2 - cfg.bmiIntrin * (holes * 2 + cut_);
    static_assert(cut_ <= halfLen / 2);

    std::array<Match<ml, cfg>, cutSize_> match_;
    uint64_t cuts_ = mkCuts(~centerBits_);
    uint64_t holes_ = ~(centerBits_ | cuts_);
};

#endif // SIEVE_H

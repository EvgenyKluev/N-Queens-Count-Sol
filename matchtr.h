#ifndef MATCHTR_H
#define MATCHTR_H

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <functional>
#include <numeric>
#include <vector>

#include "cfg.h"
#include "prefetch.h"
#include "util.h"

/* Matches a bitset against a container of bitsets and counts number of
 * non-conflicting matches (where logical AND of a pair of bitsets is zero).
 * For example, matching item 0101 against patterns {1010, 0010} gives count 2
 * and matching 0101 against {0100, 0001} gives count 0.
 *
 * A container of patterns is either std::vector<uint64_t> (for not many
 * patterns), or std::vector<Piece> where Piece stores bits of bitsets in
 * transposed form (when the amount of patterns fits conveniently into
 * N * patMaxSize_), or both.
 *
 * In addition to storing bits of bitsets in transposed form, it is possible
 * to precompute the result of several (chunkSize_) logical ANDs to reduce
 * volume of computations needed to match a single item.
 *
 * Template parameters:
 * size - number of significant bits in patterns and items
 * cfg - configuration parameters
 */
template<int size, Cfg cfg>
class MatchTr
{
public:
    MatchTr()
    {
        patterns_.reserve(patMaxSize_);
    }

    void appendPattern(uint64_t pattern)
    {
        patterns_.push_back(pattern);

        if (patterns_.size() == patMaxSize_)
            processPatterns();
    }

    /* Should be called when stream of patterns ends. Decides if patterns
     * that are still in raw form should be transposed.
     */
    void closePatterns()
    {
        if (patterns_.size() >= patMinSize_)
        {
            patterns_.resize(patMaxSize_, -1ull);
            processPatterns();
        }
    }

    uint64_t count(uint64_t item) const
    {
        auto hit = [item](uint64_t pat) { return (item & pat) == 0; };
        return countTr(item) + std::ranges::count_if(patterns_, hit);
    }

    void clear()
    {
        patterns_.clear();
        transposed_.clear();
    }

    void shrink()
    {
        patterns_.shrink_to_fit();
        transposed_.shrink_to_fit();
    }

    // Merge patterns from this object to "other" one.
    void passTo(MatchTr<size, cfg>& other)
    {
        for (const Piece& p: transposed_)
            other.transposed_.push_back(p);

        for (uint64_t p: patterns_)
            other.appendPattern(p);

        clear();
    }

    void prefetch(uint64_t item) const
    {
        static_assert(!cfg.prefetch || isPFAvail);

        if (cfg.prefetch && chunkSize_ > 1 && !transposed_.empty())
            prefetchL2(&transposed_[0][0][item & chunkMask_]);
    }

    // Only for unit tests
    size_t testPatternsSize() const
    {
        return patterns_.size();
    }

    // Only for unit tests
    size_t testTransposedSize() const
    {
        return transposed_.size();
    }

private:
    static constexpr int chunkSize_ = cfg.matchChunkSize;
    static constexpr int groupSize_ = cfg.matchGroupSize;
    static constexpr int patMinSize_ = cfg.matchMinSize;
    static constexpr int bitWidth_ = 64;
    static constexpr int patMaxSize_ = bitWidth_ * groupSize_;
    static constexpr uint64_t chunkMask_ = nLeastBits<uint64_t>(chunkSize_);
    static constexpr int numChunks_ = (size + chunkSize_ - 1) / chunkSize_;
    static constexpr int trChunkSize_ = (chunkSize_ == 1) ? 1: 1 << chunkSize_;

    using Group = std::array<uint64_t, groupSize_>;
    using Chunk = std::array<Group, trChunkSize_>;
    using Piece = std::array<Chunk, numChunks_>;
    using PieceVec = std::vector<Piece>;

    void processPatterns()
    {
        invertPatterns();
        transposePatterns();
        transformPatterns();
        patterns_.clear();
    }

    void invertPatterns()
    {
        for (auto& p: patterns_)
            p = ~p;
    }

    void transposePatterns()
    {
        const int half = bitWidth_ / 2;
        uint64_t mask = nLeastBits<uint64_t>(half);

        for (int dist = half; dist; dist /= 2, mask ^= mask << dist)
        {
            for (int off = 0; off != bitWidth_; off += 2 * dist)
            {
                for (int pos = 0; pos != dist; ++pos)
                {
                    for (int elem = 0; elem != groupSize_; ++elem)
                    {
                        cross(patterns_[(pos + off) * groupSize_ + elem],
                              patterns_[(pos + off + dist) * groupSize_ + elem],
                              mask, dist);
                    }
                }
            }
        }
    }

    static void cross(uint64_t& a,
                      uint64_t& b,
                      const uint64_t mask,
                      int dist)
    {
        const uint64_t fixA = (b & mask) << dist;
        const uint64_t fixB = (a & ~mask) >> dist;
        a = (a & mask) | fixA;
        b = (b & ~mask) | fixB;
    }

    void transformPatterns()
    {
        transposed_.push_back({});

        for (int chunkNr = 0; Chunk& chunk: transposed_.back())
        {
            for (unsigned groupNr = 0; Group& group: chunk)
                 makeGroup(chunkNr, groupNr++, group);
            ++chunkNr;
        }
    }

    void makeGroup(const int chunkNr, int, auto& dst)
    requires (chunkSize_ == 1)
    {
        const auto offset = chunkNr * dst.size();
        std::copy_n(patterns_.data() + offset, dst.size(), dst.begin());
    }

    void makeGroup(const int chunkNr, unsigned groupNr, auto& dst)
    requires (chunkSize_ > 1)
    {
        std::ranges::fill(dst, -1ull);

        for (; groupNr; groupNr &= groupNr - 1)
        {
            const unsigned bitPos = std::countr_zero(groupNr);
            const auto offset = (chunkNr * chunkSize_ + bitPos) * dst.size();
            conjTo(dst, patterns_.data() + offset);
        }
    }

    uint64_t countTr(const uint64_t item) const
    {
        auto countOne = [item](const Piece& piece) {
            Group accum;
            std::ranges::fill(accum, -1ull);
            collectBits(accum, item, piece);

            return std::transform_reduce(
                        accum.begin(), accum.end(), 0ull, std::plus<>(),
                        [](const auto& x) { return std::popcount(x); });
        };

        return std::transform_reduce(transposed_.begin(), transposed_.end(),
                                     0ull, std::plus<>(), countOne);
    }

    static void collectBits(Group& accum, uint64_t item, const Piece& piece)
    {
        if (chunkSize_ == 1)
        {
            for (; item; item &= item - 1)
                conjTo(accum, piece[std::countr_zero(item)][0]);
        }
        else
        {
            for (const Chunk& chunk: piece)
            {
                conjTo(accum, chunk[item & chunkMask_]);
                item >>= chunkSize_;
            }
        }
    }

    static void conjTo(Group& dst, const Group& src)
    {
        conjTo(dst, src.data());
    }

    static void conjTo(Group& dst, const uint64_t* src)
    {
        for (unsigned elem = 0; elem != dst.size(); ++elem)
            dst[elem] &= src[elem];
    }

    std::vector<uint64_t> patterns_;
    PieceVec transposed_;
};

#endif // MATCHTR_H

/* Contains 3 classes that control how Subsquare handles its data index:
 * PackNothing stores index as 2D array (rows * columns)
 * PackIter also stores it as 2D array but iteration skips obviously empty cells
 * PackColumns stores index compactly, without obviously empty cells
 *
 * Members:
 * RowInfo - info (about set of rows) to be stored in qsymmetry CellFactory
 * getLastIndex - position of the last index element (= index size - 1)
 * getRowInfo - computes RowInfo
 * getColIndex - computes position in index for given set of columns
 * forColumns - iterates columns for given RowInfo
 */
#ifndef PACK_H
#define PACK_H

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <numeric>

#include "util.h"

template<int size>
struct PackNothing
{
    struct RowInfo
    {
        uint32_t posInIndex;
    };

    static consteval uint32_t getLastIndex()
    {
        return rcCnt_ * rcCnt_;
    }

    static RowInfo getRowInfo(uint32_t rows)
    {
        return {rows * rcCnt_};
    }

    static uint32_t getColIndex(RowInfo rowInfo, uint32_t columns)
    {
        return rowInfo.posInIndex + columns;
    }

    static void forColumns(RowInfo rowInfo, uint32_t, const auto& action)
    {
        for (uint32_t columns = 0; columns != rcCnt_; ++columns)
            action(getColIndex(rowInfo, columns), columns);
    }

    static constexpr uint32_t rcCnt_ = 1 << size;
};

template<int size>
struct PackIter
{
    struct RowInfo
    {
        uint32_t posInIndex;
    };

    static consteval uint32_t getLastIndex()
    {
        return rcCnt_ * rcCnt_;
    }

    static RowInfo getRowInfo(uint32_t rows)
    {
        return {rows * rcCnt_};
    }

    static uint32_t getColIndex(RowInfo rowInfo, uint32_t columns)
    {
        return rowInfo.posInIndex + columns;
    }

    static void forColumns(RowInfo rowInfo, uint32_t rows, const auto& action)
    {
        const int pop = std::popcount(rows);
        const uint16_t* const begin = &getUnpack()[getUnpackInd()[pop]];
        const uint16_t* const end = &getUnpack()[getUnpackInd()[pop + 1]];

        for (const uint16_t* pos = begin; pos != end; ++pos)
            action(getColIndex(rowInfo, *pos), *pos);
    }

    static constexpr uint32_t rcCnt_ = 1 << size;
    using Unpack = std::array<uint16_t, rcCnt_>;
    using UnpackInd = std::array<uint32_t, size + 2>;

    static consteval Unpack calcUnpack()
    {
        Unpack res {};
        uint32_t pos = 0;

        for (int rowPop = 0; rowPop <= size; ++rowPop)
        {
            static_assert(size < 16);
            for (uint16_t i = 0; i != (uint16_t{1} << size); ++i)
            {
                if (std::popcount(i) == rowPop)
                    res[pos++] = i;
            }
        }

        return res;
    }

    static consteval UnpackInd calcUnpackInd()
    {
        UnpackInd res {};
        for (int i = 0; i <= size; ++i)
            res[i + 1] = combinations(size, i);

        auto begin = std::next(res.begin());
        std::partial_sum(begin, res.end(), begin);
        return res;
    }

    static constexpr const Unpack& getUnpack()
    { return unpack_; }

    static constexpr const UnpackInd& getUnpackInd()
    { return unpackInd_; }

    static constexpr Unpack unpack_ = calcUnpack();
    static constexpr UnpackInd unpackInd_ = calcUnpackInd();
};

template<int size>
struct PackColumns
{
    using Unpack = PackIter<size>;

    struct RowInfo
    {
        uint32_t posInIndex;
        uint32_t posInPacker;
    };

    static consteval uint32_t getLastIndex()
    {
        return getRowInd().back();
    }

    static RowInfo getRowInfo(uint32_t rows)
    {
        return {getRowInd()[rows], getPackerInd(std::popcount(rows))};
    }

    static uint32_t getColIndex(RowInfo rowInfo, uint32_t columns)
    {
        return rowInfo.posInIndex + getPacker()[rowInfo.posInPacker + columns];
    }

    static void forColumns(const RowInfo rowInfo,
                           const uint32_t rows,
                           const auto& action)
    {
        const uint32_t pop = std::popcount(rows);
        const uint32_t unpackPos = Unpack::getUnpackInd()[pop];
        const uint32_t end = getRowInd()[rows + 1] - rowInfo.posInIndex;

        for (uint32_t i = 0; i != end; ++i)
            action(rowInfo.posInIndex + i, Unpack::getUnpack()[unpackPos + i]);
    }

    static constexpr uint32_t rcCnt_ = 1 << size;
    using RowInd = std::array<uint32_t, rcCnt_ + 1>;
    using Packer = std::array<uint16_t, rcCnt_ * (size + 1)>;

    static consteval auto rowIndex(const int setBitCount)
    {
        std::array<uint16_t, rcCnt_> res {};
        uint16_t pos = 0;

        for (uint32_t i = 0; i != res.size(); ++i)
        {
            if (std::popcount(i) == setBitCount)
                res[i] = pos++;
        }

        return res;
    }

    static consteval Packer calcPacker()
    {
        Packer res {};

        for (int rowPop = 0; rowPop <= size; ++rowPop)
        {
            const auto row = rowIndex(rowPop);
            std::ranges::copy(row, &res[rowPop * row.size()]);
        }

        return res;
    }

    static consteval RowInd calcRowInd()
    {
        RowInd res {};
        for (unsigned i = 0; i != res.size() - 1; ++i)
            res[i + 1] = combinations(size, std::popcount(i));

        auto begin = std::next(res.begin());
        std::partial_sum(begin, res.end(), begin);
        return res;
    }

    static uint32_t getPackerInd(uint32_t pop)
    {
        return pop * rcCnt_;
    }

    static constexpr const Packer& getPacker()
    {
        return packer_;
    }

    static constexpr const RowInd& getRowInd()
    {
        return rowInd_;
    }

    static constexpr Packer packer_ = calcPacker();
    static constexpr RowInd rowInd_ = calcRowInd();
};

#endif // PACK_H

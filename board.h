#ifndef BOARD_H
#define BOARD_H

#include <array>
#include <cstdint>

#include "util.h"

/* Represents chessboard (for Subsquare class). Chessboard consists of four
 * bitsets with nonzero bits for unoccupied rows, columns, and diagonals.
 */
template <int size>
requires(0 < size && size <= 16)
class Board
{
public:
    Board() = default;

    uint32_t getFreeColumns(int row) const
    {
        return columns_
            & (diags_[0] >> fromBottom(row))
            & (diags_[1] >> row);
    }

    Board addQueen(const int row, const uint32_t singleBit) const
    {
        return {
            rows_     ^ (uint32_t{1} << row),
            columns_  ^ singleBit,
            diags_[0] ^ (singleBit << fromBottom(row)),
            diags_[1] ^ (singleBit << row)
        };
    }

    uint32_t rows() const
    {
        return rows_ ^ allRC;
    }

    uint32_t columns() const
    {
        return columns_ ^ allRC;
    }

    uint32_t diags(int index) const
    {
        return ~diags_[index];
    }

private:
    Board(uint32_t r, uint32_t c, uint32_t d1, uint32_t d2)
        : rows_{r}
        , columns_{c}
        , diags_{{d1, d2}}
    {}

    static int fromBottom(int r)
    {
        return size - 1 - r;
    }

    static constexpr uint32_t allRC = nLeastBits<uint32_t>(size);
    uint32_t rows_ = allRC;
    uint32_t columns_ = allRC;
    std::array<uint32_t, 2> diags_ = {~uint32_t{0}, ~uint32_t{0}};
};

#endif // BOARD_H

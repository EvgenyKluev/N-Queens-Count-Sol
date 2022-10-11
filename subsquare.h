#ifndef SUBSQUARE_H
#define SUBSQUARE_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

#include "board.h"
#include "foreach2.h"

/* Generates and stores all partial solutions for n-queen problem for part
 * of the chessboard (where this part is occupied by 0..k queens). Then
 * provides methods to iterate these solutions (which are represented by
 * lists of sets of occupied diagonals, and each such list is indexed by
 * sets of occupied rows/columns).
 *
 * Sets of occupied rows/columns/diagonals are implemented as bitsets and
 * stored as uint32_t values. Indexing also uses 32-bit integers which
 * should be enough for quarter-boards of size up to 13 (whole chessboard
 * of size up to 27). A combination of sets of rows and columns is named here
 * as "cell".
 *
 * Partial solutions are computed with backtracking algorithm: placeQueens()
 * is recursively called to place a queen in the first row, second one, etc.
 * Bitwise operations (see board.h) are perfect fit for this task
 * (run quickly and are easy to implement).
 *
 * Template parameters:
 * size - size of the quarter-board
 * Symmetry - a class controlling how partial solutions are stored in memory:
 *            either simply or compactly, using 1 or 3 square symmetries
 *            (see qsymmtry.h); this is independent on how symmetries are used
 *            in the main algorithm
 * Packer - a class controlling how index is stored in memory and iterated
 *            (see pack.h)
 */
template <int size,
          template<int, class...> class Symmetry,
          template<int...> class Packer>
class Subsquare
{
    using Pack = Packer<size>;
    using Symm = Symmetry<size, Pack>;
    using CellInd = Symm::CellInd;
    using CellFactory = Symm::CellFactory;
    using Diagonals = Symm::Diagonals;

    // check that uint32_t is enough for index_
    static_assert(size <= 12 + (Symm::factor > 4));

public:
    Subsquare()
    {
        fill();
    }

    // Prepare rows info to be used in forCells().
    auto withRows(uint32_t rows) const
    {
        return CellFactory(rows);
    }

    // Iterate all cells for given set of rows.
    void forCells(const CellFactory& cf, const auto& action) const
    {
        Pack::forColumns(cf.getRowInfo(), cf.getRows(),
                         [&](uint32_t ind, uint32_t columns) {
            if (Symm::factor > 2 || index(ind) != index(ind + 1)) {// non-empty
                action(cf.makeCell(ind, columns));
            }
        });
    }

    /* Iterate all pairs (cartesian product) of diagonals for given pair
     * of cells.
     */
    template<bool filterDiags, bool other>
    void forDiags(const CellInd& first,
                  const CellInd& second,
                  const auto& action) const
    {
        const auto ix = pIndex(first.index);

        if (Symm::factor > 2 && ix[0] == ix[1])
            return;

        forDiagsImpl<filterDiags, other>(first, second, action);
    }

    /* Marks a bit so that we will ignore any results having non-zero diagonal
     * associated with this bit.
     */
    void setSBit(int bitPos)
    {
        specialBit_ = uint32_t{1} << bitPos;
        partitionCells(pIndex(0), pIndex(Pack::getLastIndex()));
    }

    // Indicates that no additional check is needed for special bit.
    static bool handlesSpecialBit()
    {
        return Symm::factor <= 2;
    }

private:
    struct Piece
    {
        uint32_t columns;
        std::array<uint32_t, 2> diags;
    };

    static constexpr uint32_t rcCnt_ = 1 << size;
    using UnsortedRow = std::vector<Piece>;
    using Unsorted = std::array<UnsortedRow, rcCnt_>;
    using Index = std::array<uint32_t, Pack::getLastIndex() + 1>;

    template<bool filter, bool other>
    void forDiagsImpl(const CellInd& first,
                      const auto& second,
                      const auto& action) const
    {
        auto next = [&, this](Diagonals d) {
            forDiagsImpl<filter, !other>(second, Symm::fix(d, first), action);
        };

        namespace r = std::ranges;
        namespace v = std::views;
        const auto ix = pIndex(first.index);
        auto list = r::subrange(&diags_[ix[0]], &diags_[ix[1]]);

        if (!filter || !Symm::template filter<other>(first, specialBit_))
        {
            r::for_each(list, next);
        }
        else if (other ^ Symm::reflect(first))
        {
            forEach2(list | v::take_while(hasZeroSBit(1)),
                     list | v::reverse | v::take_while(hasZeroSBit(1)),
                     list.size(),
                     next);
        }
        else
        {
            r::for_each(list | v::take_while(hasZeroSBit(0)), next);
        }
    }

    template<bool, bool>
    void forDiagsImpl(const Diagonals& first,
                      const Diagonals& second,
                      const auto& action) const
    {
        action(std::make_pair(first, second));
    }

    // Produce all partial solutions for [0..size] queens on chessboard.
    void fill()
    {
        Board<size> board;
        auto pSink = std::make_unique<Unsorted>();

        if constexpr (requires {UnsortedRow{}.reserve(1);} && size > 11)
        {
            /* Try to allocate this using mmap (>=128K for each allocation)
             * so that temporary data do not bloat sbrk space (use vector) */
            for (UnsortedRow& r: *pSink)
                r.reserve(130 * 1024 / sizeof(Piece));
        }

        placeQueens(board, 0, *pSink);
        diags_.reserve(std::transform_reduce(pSink->begin(), pSink->end(),
            0ull, std::plus<>(), [](const auto& c){ return c.size(); }));

        for (uint32_t rows = 0; UnsortedRow& r: *pSink)
            reorder(rows++, r);
    }

    // Recursively place queens to get all partial solutions
    static void placeQueens(const Board<size> board,
                            const unsigned row,
                            Unsorted& sink)
    {
        if (row == size)
        {
            addPiece(board, sink);
            return;
        }

        uint32_t columns = board.getFreeColumns(row);

        while (columns != 0)
        {
            uint32_t firstBit = columns & -columns;
            placeQueens(board.addQueen(row, firstBit), row + 1, sink);
            columns ^= firstBit;
        }

        placeQueens(board, row + 1, sink); // empty row
    }

    // Record a single solution (if needed)
    static void addPiece(const Board<size>& board, Unsorted& sink)
    {
        if (Symm::isUniq(board.rows(), board.columns()))
            sink[board.rows()].emplace_back(
                  board.columns(), Diagonals{board.diags(0), board.diags(1)});
    }

    // Move solutions to permanent locations and update index
    void reorder(const uint32_t rows, UnsortedRow& rowData)
    {
        auto ri = Pack::getRowInfo(rows);
        auto ri1 =  Pack::getRowInfo(rows + 1);
        uint32_t* const dstBegin = pIndex(ri.posInIndex);
        uint32_t* const dstEnd = pIndex(ri1.posInIndex);
        std::fill(dstBegin, dstEnd, 0);
        *dstBegin = static_cast<uint32_t>(diags_.size());
        diags_.resize(diags_.size() + rowData.size());

        for (const Piece& piece: rowData)
            ++index(Pack::getColIndex(ri, piece.columns));

        std::partial_sum(dstBegin, dstEnd, dstBegin);

        for (const Piece& piece: rowData)
            diags_[--index(Pack::getColIndex(ri, piece.columns))] = piece.diags;

        *dstEnd = static_cast<uint32_t>(diags_.size());
        partitionCells(dstBegin, dstEnd);
        rowData.clear();
        rowData.shrink_to_fit();
    }

    // Move solutions having nonzero special bit closer to each other
    void partitionCells(const uint32_t* begin, const uint32_t* end)
    {
        for (const uint32_t* it = begin; it != end; ++it)
        {
            namespace r = std::ranges;
            auto wholeR = r::subrange(&diags_[*it], &diags_[it[1]]);
            auto bit1R = r::partition(wholeR, hasZeroSBit(0));
            auto bit0R = r::subrange(wholeR.begin(), bit1R.begin());
            r::partition(bit0R, hasZeroSBit(1));
            r::partition(bit1R, std::not_fn(hasZeroSBit(1)));
        }
    }

    auto hasZeroSBit(int whichDiag) const
    {
        return [whichDiag, this](Diagonals d) {
            return (d[whichDiag] & specialBit_) == 0;
        };
    }

    uint32_t& index(auto pos)
    {
        return (*index_)[pos];
    }

    const uint32_t& index(auto pos) const
    {
        return (*index_)[pos];
    }

    uint32_t* pIndex(auto pos) const
    {
        return index_->data() + pos;
    }

    std::unique_ptr<Index> index_ = std::make_unique<Index>();
    std::vector<Diagonals> diags_;
    uint32_t specialBit_ = uint32_t{1} << (size - 1);
};

#endif // SUBSQUARE_H

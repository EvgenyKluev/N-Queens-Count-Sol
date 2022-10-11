/* Contains 3 classes that control how compactly Subsquare stores its data:
 * QNoSymmetry: simple strategy, largest amount of data, fast
 * QRowSymmetry: uses one symmetry, only half the size, slightly slower
 * QSymmetry: uses all three symmetries, 1/8 size, pretty slow
 *
 * Members:
 * Cell - set of rows + set of columns, position in the index, symmetries used
 * CellInd - position in the index, symmetries used
 * CellFactory - factory for Cell and CellInd, stores set of rows, position of
 *               rows in the index and in the packer, and symmetries
 * isUniq - used by subsquare solution generator to drop duplicates
 * filter - returns true if subsquare is allowed to filter solutions
 *          by special bit
 * reflect - "true" signals to subsquare that diagonals are swapped
 * fix - uses symmetry flags to restore original diagonals
 */
#ifndef QSYMMETRY_H
#define QSYMMETRY_H

#include <array>
#include <cstdint>
#include <variant> // monostate

#include "util.h"

template <int size, class Pack, class Flags = std::monostate>
struct QNoSymmetry
{
    using Diagonals = std::array<uint32_t, 2>;

    struct Cell
    {
        uint32_t index;
        uint32_t rows;
        uint32_t columns;
        [[no_unique_address]] Flags flags;
    };

    struct CellInd
    {
        uint32_t index;
        [[no_unique_address]] Flags flags;
    };

    class CellFactory
    {
    public:
        explicit CellFactory(uint32_t rows)
            : rows_{rows}
        {
            if constexpr (sizeof(Cell::flags) > 1)
            {
                const uint32_t reversed = revBits<size, size>(rows);

                if (reversed < rows)
                {
                    rows_ = reversed;
                    flags_.swapDiag = true;
                }
            }

            rowInfo_ = Pack::getRowInfo(rows_);
        }

        Cell makeCell(uint32_t index, uint32_t columns) const
        {
            return {index, rows_, columns, flags_};
        }

        CellInd makeCellInd(uint32_t columns) const
        {
            return {Pack::getColIndex(rowInfo_, columns), flags_};
        }

        CellInd makeCellInd(const Cell& cell) const
        {
            return makeCellInd(cell.columns);
        }

        Pack::RowInfo getRowInfo() const
        {
            return rowInfo_;
        }

        uint32_t getRows() const
        {
            return rows_;
        }

    protected:
        Pack::RowInfo rowInfo_;
        uint32_t rows_;
        [[no_unique_address]] Flags flags_;
    };

    static bool isUniq(uint32_t, uint32_t)
    {
        return true;
    }

    template<bool>
    static bool filter(const CellInd&, uint32_t)
    {
        return true;
    }

    static bool reflect(const CellInd&)
    {
        return false;
    }

    static Diagonals fix(const Diagonals& d, const CellInd&)
    {
        return d;
    }

    static constexpr int factor = 1;
};

template <int size, class Pack>
struct QRowSymmetry
{
    struct Flags
    {
        bool revDiag[2] {false, false};
        bool swapDiag {false};
    };

    using Base = QNoSymmetry<size, Pack, Flags>;
    using Diagonals = Base::Diagonals;
    using Cell = Base::Cell;
    using CellInd = Base::CellInd;
    using CellFactory = Base::CellFactory;

    static bool isUniq(uint32_t rows, uint32_t)
    {
        return rows <= revBits<size, size>(rows);
    }

    template<bool>
    static bool filter(const CellInd&, uint32_t)
    {
        return true;
    }

    static bool reflect(const CellInd& c)
    {
        return c.flags.swapDiag;
    }

    static Diagonals fix(const Diagonals& d, const CellInd& c)
    {
        if (c.flags.swapDiag)
            return {d[1], d[0]};
        else
            return d;
    }

    static constexpr int factor = 2;
};

template <int size, class Pack>
struct QSymmetry
{
    using Base = QRowSymmetry<size, Pack>;
    using Diagonals = Base::Diagonals;
    using Cell = Base::Cell;
    using CellInd = Base::CellInd;
    using Flags = Base::Flags;

    class CellFactory: public Base::CellFactory
    {
    public:
        using Base::CellFactory::CellFactory;

        Cell makeCell(uint32_t index, uint32_t columns) const
        {
            return {index, this->rows_, columns, this->flags_};
        }

        CellInd makeCellInd(uint32_t columns) const
        {
            return makeCIImpl(
                        this->rowInfo_, this->rows_, columns, this->flags_);
        }

        CellInd makeCellInd(const Cell& c) const
        {
            return makeCIImpl(this->rowInfo_, c.rows, c.columns, c.flags);
        }

    private:
        CellInd makeCIImpl(Pack::RowInfo rowInfo,
                           uint32_t rows,
                           uint32_t columns,
                           Flags flags) const
        {
            const uint32_t reversed = revBits<size, size>(columns);
            if (reversed < columns)
            {
                columns = reversed;
                flags.revDiag[0] = true;
                flags.revDiag[1] = true;
                flags.swapDiag ^= true;
            }

            if (columns < rows)
            {
                std::swap(columns, rows);
                flags.revDiag[0] ^= true;
                rowInfo = Pack::getRowInfo(rows);
            }

            const uint32_t index = Pack::getColIndex(rowInfo, columns);
            return {index, flags};
        }
    };

    static bool isUniq(uint32_t rows, uint32_t columns)
    {
        return rows <= revBits<size, size>(rows)
            && columns <= revBits<size, size>(columns)
            && rows <= columns;
    }

    template<bool other>
    static bool filter(const CellInd& c, uint32_t bit)
    {
        const bool inCenter = (bit == (uint32_t{1} << (size - 1)));
        return inCenter || !c.flags.revDiag[other ^ reflect(c)];
    }

    static bool reflect(const CellInd& c)
    {
        return c.flags.swapDiag;
    }

    static Diagonals fix(Diagonals d, const CellInd& c)
    {
        static constexpr int dSize = size * 2 - 1;

        if (c.flags.revDiag[0])
            d[0] = revBits<dSize, size>(d[0]);

        if (c.flags.revDiag[1])
            d[1] = revBits<dSize, size>(d[1]);

        if (c.flags.swapDiag)
            return {d[1], d[0]};
        else
            return d;
    }

    static constexpr int factor = 8;
};

#endif // QSYMMETRY_H

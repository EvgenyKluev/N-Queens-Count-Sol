/* Contains 2 classes: Quadrants1: main n-queens solution counting algorithm;
 * and Context: a collection of objects needed for main algorithm.
 */
#ifndef QUADRANTS1_H
#define QUADRANTS1_H

#include <cstdint>
#include <utility>

#include "divider.h"
#include "freeze.h"
#include "scheduler.h"
#include "util.h"

/* Context Object pattern. Represents information and services needed for
 * main algorithm.
 */
template<class Start, typename Thread>
struct Context
{
    using Sieve_ = Start::Sieve_;
    using Freeze_ = Freeze<Sieve_, Start>;

    const Sieve_& counter() const&
    {
        if (ThreadPolicy::isThreaded())
            return freeze->getObj();
        else
            return sink;
    }

    void sync() const
    {
        thread->sync();
    }

    Start start;
    Thread* thread;
    Sieve_ sink;
    Freeze_* freeze;
    Divider divider;
};

/* Algorithm for counting solutions of n-queens problem (or elements of
 * OEIS A000170 sequence). Unlike other approaches, this algorithm does
 * not use backtracking (directly). Instead it splits the chessboard
 * into four quadrants, generates all possible solutions for each quadrant
 * (stored in member quarter_), joins 'north' and 'south' parts of each
 * ('east' and 'west') halfboard, and finally counts all cases where these
 * halfboards match each other (do not have queens competing for same diagonal).
 *
 * For odd board sizes two queens are placed in middle row/column
 * (only one if in the center). Then the remaining four quarters of the board
 * are handled as said above.
 *
 * Most (but not all) relatively symmetrical cases are computed only once,
 * which gives 8x speedup (but does not give the number of unique solutions).
 * For odd board sizes this is obtained by middle row/column placement.
 * For even sizes we choose unique row combinations (for one symmetry) and
 * then filter out redundant half-solutions based on longest diagonals
 * occupation (for other two symmetries).
 *
 * Main entry point (operator()) could be entered by several threads at once.
 * Then small part of work is duplicated by all threads until
 * env.thread->rejected/accepted is called, which distributes work between
 * threads. (Although work distribution could be done at start of main
 * loop, where env.divider is called, this would result in many threads
 * competing for L3 CPU cache). So we split the work later, when choosing
 * sets of columns. This approach results in shared mutable data. env.sync()
 * sets barriers that divide different steps of computation and allow to use
 * this shared mutable data either as shared immutable or as unshared mutable
 * at different steps. Unlike other methods, constructor/destructor are
 * single-threaded.
 *
 * env.start object controls many details of this class' behavoir. It determines
 * where one or two initial queens are located (for odd sizes). Also it commands
 * when symmetry optimizations are applied. And it determines classes for
 * choosing row combinations (getBitComb()) and merging half-board solutions
 * (Sieve_). There are four types of env.start: StartEmpty for even sizes,
 * StartCenter for single queen in the center, Start2D for most other odd sizes,
 * and Start1D for special case of one queen at the border.
 *
 * Template parameters:
 * size - size of the chessboard
 * Quarter - solution generator for board's quadrants
 */
template <int size, class Quarter>
requires(size > 4)
class Quadrants1
{
public:
    uint64_t operator() (auto& env) const
    {
        return doWhole(env);
    }

    /* Marks a bit to west of the center so that quarter_ will not produce
     * any partial results having non-zero diagonal associated with this bit.
     */
    void setSBit(auto& env, int bitPos)
    {
        if (env.thread->accepted())
            quarter_.setSBit(bitPos);
    }

    /* Frees memory that is still allocated by some containers. (These
     * containers are not needed anymore but cannot be deleted yet).
     */
    void shrink(auto& env)
    {
        env.sync();

        if (env.thread->accepted())
            env.freeze->shrink();
    }

private:
    /* For each row, set queen to east/west side of the board, then process
     * east/west halfboards and update number of solutions accordingly.
     */
    uint64_t doWhole(auto& env) const
    {
        uint64_t counter = 0;

        for (uint32_t bits: env.start.getBitComb())
        {
            if (env.divider()) continue;
            const uint32_t eastRows = env.start.stretchRows(bits);
            const uint32_t westRows = eastRows ^ env.start.getFreeRows();

            if (unsigned m = getRowsSymm(env, eastRows))
            {
                env.sync(); fill(env, eastRows);
                env.sync(); env.freeze->freeze(env.thread);
                env.sync(); counter += m * count(env, westRows);
                env.sync(); if (env.thread->accepted()) env.freeze->clear();
            }
        }

        return counter;
    }

    /* Try to avoid double work if we already counted solutions for the board
     * turned upside down.
     */
    static unsigned getRowsSymm(auto& env, uint32_t eastRows)
    {
        const uint32_t revRows = revBits<size, halfSize>(eastRows);

        if (!env.start.internalSymmetry() || eastRows == revRows)
            return 1;

        if (eastRows < revRows)
            return 2;

        return 0;
    }

    // Process east half-board and store results
    void fill(auto& env, uint32_t eastRows) const
    {
        doHalf<false>(env, eastRows, [&, this](const auto& d) {
            if (env.start.diagSymmetry() && !bothDiagsEmpty(d))
                return;
            const auto halfDiags = joinQuarters<halfCeil, 0>(d);
            env.sink.appendPattern(halfDiags);
        });
    }

    // Process west half-board and count matchings with east side
    uint64_t count(auto& env, uint32_t westRows) const
    {
        uint64_t total = 0;
        doHalf<true>(env, westRows, [&, this](const auto& d) {
            const unsigned m = diagsSymmetryFactor(env, d);
            const auto& halfDiags = joinQuarters<0, halfCeil>(d);
            total += m * env.counter().count(halfDiags);
        });

        return total;
    }

    /* For east/west half-board: for each column, set queen to north/south side
     * of the board, then request pairs of precomputed sets of occupied
     * diagonals, and filter out incompatible pairs.
     */
    template<bool west>
    void doHalf(auto& env, const uint32_t rows, const auto& action) const
    {
        const uint32_t halfColumns = halfBits<west>(env.start.getColumns());
        const auto north = quarter_.withRows(loBits(rows));
        const auto south = quarter_.withRows(hiBits(rows));

        quarter_.forCells(north, [&, this](const auto& northCell) {
            if ((northCell.columns & halfColumns) || env.thread->rejected())
                return;

            const auto sColumns = (northCell.columns ^ ~halfColumns) & lowHalf;
            const auto northCellInd = north.makeCellInd(northCell);
            const auto southCellInd = south.makeCellInd(sColumns);

            static constexpr bool filterDiag =
                  (env.start.diagSymmetry() && !west) || env.start.filterDiag();

            quarter_.template forDiags<filterDiag, west>(
                        northCellInd, southCellInd, [&](const auto& d) {
                if (!matchQuarters(d))
                    return;
                if (!env.start.template matchDiags<west? halfCeil: 0>(d))
                    return;
                action(d);
            });
        });
    }

    /* Uses longest diagonals occupation to determine how many relatively
     * symmetrical solutions we have got.
     */
    static unsigned diagsSymmetryFactor(auto& env, const auto& diags)
    {
        unsigned symmetryFactor = 1;

        if (env.start.diagSymmetry())
        {
            if (!isLongestHalfDiagEmpty(diags.first[1]))
                symmetryFactor *= 2;

            if (!isLongestHalfDiagEmpty(diags.second[0]))
                symmetryFactor *= 2;
        }

        return symmetryFactor;
    }

    /* Returns false to filter out redundant half-solutions based on
     * longest diagonals occupation.
     */
    static bool bothDiagsEmpty(const auto& diags)
    {
        return Quarter::handlesSpecialBit()
                || ( isLongestHalfDiagEmpty(diags.second[1])
                  && isLongestHalfDiagEmpty(diags.first[0]) );
    }

    static bool isLongestHalfDiagEmpty(const uint32_t halfDiag)
    {
        static constexpr auto middle = uint32_t{1} << (halfSize - 1);
        return (halfDiag & middle) == 0;
    }

    // true if north/south quarters are compatible
    static bool matchQuarters(const auto& diags)
    {
        uint32_t fwdMeet = (diags.first[1] >> halfCeil) & diags.second[1];
        uint32_t bkwdMeet = diags.first[0] & (diags.second[0] >> halfCeil);
        return fwdMeet == 0 && bkwdMeet == 0;
    }

    template<int offsetL, int offsetH>
    static auto joinQuarters(const auto& diags)
    {
        return std::make_pair(
            (diags.first[0] << offsetH) | (diags.second[0] >> offsetL),
            (diags.first[1] >> offsetL) | (diags.second[1] << offsetH)
        );
    }

    template<bool west>
    static uint32_t halfBits(uint32_t bits)
    {
        return west? hiBits(bits): loBits(bits);
    }

    static uint32_t loBits(uint32_t bits)
    {
        return bits & lowHalf;
    }

    static uint32_t hiBits(uint32_t bits)
    {
        return bits >> halfCeil;
    }

    static constexpr int halfSize = size / 2;
    static constexpr int halfCeil = (size + 1) / 2;
    static constexpr uint32_t lowHalf = nLeastBits<uint32_t>(halfSize);

    Quarter quarter_;
};

#endif // QUADRANTS1_H

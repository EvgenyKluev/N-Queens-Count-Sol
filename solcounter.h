#ifndef SOLCOUNTER_H
#define SOLCOUNTER_H

#include <cstdint>
#include <tuple>

#include "bitcombcolex.h"
#include "cfg.h"
#include "divider.h"
#include "freeze.h"
#include "matchtr.h"
#include "pack.h"
#include "qsymmetry.h"
#include "quadrants1.h"
#include "scheduler.h"
#include "start2d.h"
#include "startcenter.h"
#include "startempty.h"
#include "subsquare.h"

inline constexpr int size = 18;
inline constexpr Cfg cfg { // s= 16   17   18   19   20   21   22
    .sieveCuts = 0,        //     0    0    0    0    3    3    6
    .matchMinSize = 40,    //    40   40   40   40   60   60   80
    .bmiIntrin = false,    //     F    F    F    T    T    T    T
    .prefetch = false,     //     T    T    T    T    *    F    T
    // Subsquare Symmetry      NoSy NoSy NoSy NoSy NoSy NoSy RowS
};

using Quarter = Subsquare<size / 2, QNoSymmetry, PackIter>;
using Quadrants = Quadrants1<size, Quarter>;

template<class Start>
uint64_t countStep(auto& thread, auto& frzs, auto& quad, Divider& div)
{
    using Sieve = Start::Sieve_;
    auto& frz = std::get<Freeze<Sieve, Start>>(frzs);
    Context env{Start{}, &thread, Sieve{}, &frz, div};
    frz.reg(&env.sink);
    uint64_t res;

    if constexpr (requires {env.start.forCR(env, quad);})
        res = env.start.forCR(env, quad);
    else
        res = quad(env);

    quad.shrink(env);
    return res;
}

template<class... Starts>
uint64_t countSteps(Scheduler& sch, Divider& div)
{
    std::tuple<Freeze<typename Starts::Sieve_, Starts>...> frzs;
    Quadrants quad;

    return sch.launch([&](auto& thread) {
        return (... + countStep<Starts>(thread, frzs, quad, div));
    });
}

uint64_t countSolutions(int threads, int part, int parts)
{
    Scheduler sch(threads);
    Divider div(part, parts);

    if (size & 1)
    {
        return countSteps<
                    StartCenter<size, MatchTr, BitCombColex, cfg>,
                    Start2D<size, MatchTr, BitCombColex, cfg>,
                    Start1D<size, MatchTr, BitCombColex, cfg>
                >(sch, div);
    }
    else
    {
        return countSteps<
                    StartEmpty<size, MatchTr, BitCombColex, cfg>
                >(sch, div);
    }
}

#endif // SOLCOUNTER_H

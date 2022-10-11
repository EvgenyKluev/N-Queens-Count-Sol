#ifndef FOREACH2_H
#define FOREACH2_H

#include <cstddef>
#include <ranges>

/* Iterates first range, then if not enough iterations done iterates second
 * one. Calls supplied function that accepts range element and returns void.
 */
template<std::ranges::range R1, std::ranges::range R2, typename F>
inline void forEach2(R1&& range1, R2&& range2, ptrdiff_t count, F fn)
{
    for (auto& elem: range1)
    {
        fn(elem);
        --count;
    }

    if (count <= 0)
        return;

    for (auto& elem: range2)
    {
        fn(elem);
    }
}

#endif // FOREACH2_H

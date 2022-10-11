#ifndef PREFETCHX86_H
#define PREFETCHX86_H

#include <immintrin.h>

void prefetchL2(const void* p)
{
    _mm_prefetch(p, _MM_HINT_T1);
}

inline constexpr bool _isPFAvail(bool) { return true; }

#endif // PREFETCHX86_H

#ifndef PREFETCH_H
#define PREFETCH_H

// Uncomment this include to allow prefetch instruction.
//#include "prefetchx86.h"

inline constexpr bool _isPFAvail(int) { return false; }
inline constexpr bool isPFAvail = _isPFAvail(false);

template<typename T>
void prefetchL2(const T*)
requires (!isPFAvail);

#endif // PREFETCH_H

#ifndef BMI2_H
#define BMI2_H

#include <immintrin.h>

template <typename Bits>
inline constexpr Bits bdep(const Bits src, const Bits mask)
{
    if constexpr (sizeof(Bits) > 4)
        return _pdep_u64(src, mask);
    else
        return _pdep_u32(src, mask);
}

template <typename Bits>
inline constexpr Bits bext(const Bits src, const Bits mask)
{
    if constexpr (sizeof(Bits) > 4)
        return _pext_u64(src, mask);
    else
        return _pext_u32(src, mask);
}

inline constexpr bool _isBMAvail(bool) { return true; }

#endif // BMI2_H

#ifndef BMINTRIN_H
#define BMINTRIN_H

// Uncomment this include to allow BMI2 instructions.
//#include "bmi2.h"

inline constexpr bool _isBMAvail(int) { return false; }
inline constexpr bool isBMAvail = _isBMAvail(false);

template <typename Bits>
inline constexpr Bits bdep(const Bits, const Bits)
requires (!isBMAvail);

template <typename Bits>
inline constexpr Bits bext(const Bits, const Bits)
requires (!isBMAvail);

#endif // BMINTRIN_H

#pragma once

#include "xSIMD.h"

#if ! MARSDSP_NO_XSIMD
/** Simplified way of `using` the same function from both `std::` and `xsimd::` */
#define MARSDSP_USING_XSIMD_STD(func) \
using std::func;                  \
using xsimd::func
#else
#define MARSDSP_USING_XSIMD_STD(func) \
using std::func
#endif

namespace MarsDSP
{
    /** Useful methods for working with SIMD batches via XSIMD */
    namespace SIMDUtils
    {
#if ! MARSDSP_NO_XSIMD
        /** Default byte alignment to use for SIMD-aligned data. */
        constexpr auto defaultSIMDAlignment = xsimd::default_arch::alignment();
#else
        /** Default byte alignment to use for SIMD-aligned data. */
        constexpr size_t defaultSIMDAlignment = 16;
#endif
    }
}

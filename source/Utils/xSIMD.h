#pragma once
#include "../Includes.h"
#include <cassert>


/** Config: MARSDSP_NO_XSIMD
    Enable this flag to skip including XSIMD headers.
*/

#ifndef MARSDSP_NO_XSIMD
#if JUCE_TEENSY
#define MARSDSP_NO_XSIMD 1
#else
#define MARSDSP_NO_XSIMD 0
#endif
#endif

#if MARSDSP_USING_JUCE
#include <juce_audio_basics/juce_audio_basics.h>
#endif

#if ! MARSDSP_NO_XSIMD

// Third-party includes
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wcast-align",
                                     "-Wimplicit-int-conversion",
                                     "-Wshadow",
                                     "-Wshadow-field",
                                     "-Wsign-conversion",
                                     "-Wzero-as-null-pointer-constant",
                                     "-Wsign-compare",
                                     "-Wc++98-compat-extra-semi",
                                     "-Wshorten-64-to-32",
                                     "-Wfloat-equal",
                                     "-Woverflow",
                                     "-Wdeprecated")
JUCE_BEGIN_IGNORE_WARNINGS_MSVC (4244)
#if defined(__has_include)
#  if __has_include(<xsimd.hpp>)
#    include <xsimd.hpp>
#  else
#    include "xsimd/xsimd.hpp"
#  endif
#else
#  include "xsimd/xsimd.hpp"
#endif
#include "SampleTypeHelpers.h"
#include "SIMDUtils.h"
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_END_IGNORE_WARNINGS_MSVC

#else
// No xSIMD - do not include third-party headers here
#endif

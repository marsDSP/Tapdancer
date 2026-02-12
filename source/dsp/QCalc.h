#pragma once

#ifndef TAPDANCER_QCALC_H
#define TAPDANCER_QCALC_H

#include <cmath>
#include <algorithm>
#include <numbers>

// QCalculator for included filters on the delay module
// Not sure if the plugin will have FFT though
// Might just keep it knobs only

struct BiquadCoeffs { double b0, b1, b2, a1, a2; };
enum class QMode { NormalQ, ProportionalQ };
enum class FilterType { LowShelf, Peak, HighShelf };

class QCalc {
public:
    template <QMode mode, FilterType type>
    static BiquadCoeffs calculator (double sampleRate,
                                    double invSampleRate, // division is slowwwwww
                                    double frequency,
                                    double gainDB,
                                    double QControl)
    {
        if (sampleRate <= 0.0 || frequency <= 0.0) { return { 1.0, 0.0, 0.0, 0.0, 0.0 }; }

        frequency = std::clamp(frequency, 10e-9, 0.5 * sampleRate - 10e-9);

        constexpr double ln10_40 = std::numbers::ln10_v<double> / 40.0;

        // std::pow is not fast.
        const double A = std::exp(gainDB * (ln10_40));
        const double sqrtA = std::sqrt(A);
        constexpr double twoPI = 2.0 * std::numbers::pi_v<double>;

        // 100x gains just by multiplying here
        const double w0 = twoPI * frequency * invSampleRate;

        // declaring these for compiler optimization (fsincos)
        const double sinW0 = std::sin(w0);
        const double cosW0 = std::cos(w0);
        double finalQ = QControl;

        // 'if' is slow; lets optimize for compile-time
        if constexpr (mode == QMode::ProportionalQ && type == FilterType::Peak) {
            constexpr double minQ = 0.5;
            constexpr double maxQ = 3.0;
            const double gainFactor = std::min(std::abs(gainDB) * (0.0833333333333333), 1.0);
            finalQ = minQ + (gainFactor * (maxQ - minQ));
            finalQ *= QControl;
        }

        finalQ = std::max(finalQ, 10e-9);
        double b0, b1, b2, a0, a1, a2;

        if constexpr (type == FilterType::LowShelf) {
            // LowShelf math
        } else if constexpr (type == FilterType::Peak) {
            // Peak math
        } else if constexpr (type == FilterType::HighShelf) {
            // HighShelf math
        }
    }
};
#endif
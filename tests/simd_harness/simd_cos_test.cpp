#include <iostream>
#include <fstream>
#include <cmath>
#include <numbers>
#include <vector>
#include <iomanip>
#include "dsp/math/fastermath.h"

int main()
{
    const float start = -std::numbers::pi_v<float>;
    const float end = std::numbers::pi_v<float>;
    const int steps = 4096; // more points for smoother curves
    const float step_size = (end - start) / steps;

    std::ofstream csv("tests/simd_harness/logs/simd_cos_results.csv");
    if (!csv.is_open())
    {
        std::cerr << "Failed to open tests/simd_harness/logs/simd_cos_results.csv" << std::endl;
        return 1;
    }

    csv << "x,std_cos,pade_scalar,pade_simd,abs_err_scalar,abs_err_simd,diff_simd_scalar\n";
    csv << std::fixed << std::setprecision(10);

    for (int i = 0; i <= steps; i += 4)
    {
        float x_vals[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int j = 0; j < 4; ++j)
        {
            if (i + j <= steps)
                x_vals[j] = start + static_cast<float>(i + j) * step_size;
            else
                x_vals[j] = 0.0f; // pad with zeros
        }

        // 1. Calculate std::cos and scalar Pade
        float std_results[4];
        float scalar_results[4];
        for (int j = 0; j < 4; ++j)
        {
            std_results[j] = std::cos(x_vals[j]);
            scalar_results[j] = MarsDSP::padeCosApprox(x_vals[j]);
        }

        // 2. Calculate SIMD Pade
        // Note: set_ps takes arguments in reverse order (x3, x2, x1, x0)
        SIMD_M128 vx = SIMD_MM(set_ps)(x_vals[3], x_vals[2], x_vals[1], x_vals[0]);
        SIMD_M128 vres = MarsDSP::fasterCos(vx);

        float simd_results[4];
        SIMD_MM(storeu_ps)(simd_results, vres);

        // 3. Output results
        for (int j = 0; j < 4; ++j)
        {
            if (i + j > steps) break;

            float x = x_vals[j];
            float val_std = std_results[j];
            float val_scalar = scalar_results[j];
            float val_simd = simd_results[j];

            float err_scalar = std::abs(val_std - val_scalar);
            float err_simd = std::abs(val_std - val_simd);
            float diff_simd_scalar = std::abs(val_scalar - val_simd);

            csv << x << ","
                << val_std << ","
                << val_scalar << ","
                << val_simd << ","
                << err_scalar << ","
                << err_simd << ","
                << diff_simd_scalar << "\n";
        }
    }

    csv.close();
    std::cout << "Successfully generated tests/simd_harness/logs/simd_cos_results.csv with " << (steps + 1) << " data points." << std::endl;

    return 0;
}

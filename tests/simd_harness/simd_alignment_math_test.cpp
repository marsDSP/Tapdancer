#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <string>
#include <numeric>
#include <algorithm>
#include <random>
#include "dsp/math/fastermath.h"

using namespace MarsDSP;

struct MathAlignmentResult {
    std::string functionName;
    int offset;
    double maxError;
};

void exportToCSV(const std::string& filename, const std::vector<MathAlignmentResult>& results)
{
    std::ofstream csv(filename);
    if (!csv.is_open()) return;

    csv << "function,offset,max_error\n";
    for (const auto& res : results) {
        csv << res.functionName << "," 
            << res.offset << "," 
            << res.maxError << "\n";
    }
    csv.close();
}

int main()
{
    std::cout << "Starting FasterMath SIMD Alignment Test..." << std::endl;

    const int bufferSize = 64;
    alignas(16) float inputBuffer[bufferSize];
    alignas(16) float outputSIMD[bufferSize];
    float outputScalar[bufferSize];

    std::mt19937 gen(123);
    std::uniform_real_distribution<float> dis(-10.0f, 10.0f);
    for (int i = 0; i < bufferSize; ++i) inputBuffer[i] = dis(gen);

    std::vector<MathAlignmentResult> results;

    auto runTest = [&](const std::string& name, auto simdFunc, auto scalarFunc) {
        for (int offset = 0; offset < 4; ++offset) {
            // Scalar
            for (int i = 0; i < 4; ++i) {
                outputScalar[offset + i] = scalarFunc(inputBuffer[offset + i]);
            }

            // SIMD
            SIMD_M128 vx = SIMD_MM(loadu_ps)(inputBuffer + offset);
            SIMD_M128 vres = simdFunc(vx);
            SIMD_MM(storeu_ps)(outputSIMD + offset, vres);

            double maxErr = 0;
            for (int i = 0; i < 4; ++i) {
                maxErr = std::max(maxErr, (double)std::abs(outputSIMD[offset + i] - outputScalar[offset + i]));
            }
            results.push_back({name, offset, maxErr});
        }
    };

    runTest("sin", [](SIMD_M128 x) { return fasterSin(x); }, [](float x) { return fasterSin(x); });
    runTest("cos", [](SIMD_M128 x) { return fasterCos(x); }, [](float x) { return fasterCos(x); });
    runTest("tan", [](SIMD_M128 x) { return fasterTan(x); }, [](float x) { return fasterTan(x); });
    runTest("tanh", [](SIMD_M128 x) { return fasterTanh(x); }, [](float x) { return fasterTanh(x); });
    runTest("boundToPi", [](SIMD_M128 x) { return boundToPiSIMD(x); }, [](float x) { return boundToPi(x); });

    exportToCSV("tests/simd_harness/logs/simd_alignment_math.csv", results);
    std::cout << "Results exported to tests/simd_harness/logs/simd_alignment_math.csv" << std::endl;

    for (const auto& res : results) {
        if (res.maxError > 1e-5) {
            std::cout << "FAILED alignment test: " << res.functionName << " offset=" << res.offset << " err=" << res.maxError << std::endl;
            return 1;
        }
    }

    std::cout << "All FasterMath alignment tests PASSED." << std::endl;
    return 0;
}

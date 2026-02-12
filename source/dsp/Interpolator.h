#pragma once

#ifndef TAPDANCER_INTERPOLATOR_H
#define TAPDANCER_INTERPOLATOR_H

struct Lagrange5th
{
    template <typename T>
    void updateInternalVariables (int& delayIntOffset, T& delayFrac)
    {
        if (delayIntOffset >= 2)
        {
            delayFrac += static_cast<T>(2);
            delayIntOffset -= 2;
        }
    }

    template <typename SampleType, typename NumericType, typename StorageType = SampleType>
    inline SampleType call (const StorageType* buffer, int delayInt,
        NumericType delayFrac, const SampleType& /*state*/ = {})
    {
        auto index1 = delayInt;
        auto index2 = index1 + 1;
        auto index3 = index2 + 1;
        auto index4 = index3 + 1;
        auto index5 = index4 + 1;
        auto index6 = index5 + 1;

        auto value1 = static_cast<SampleType> (buffer[index1]);
        auto value2 = static_cast<SampleType> (buffer[index2]);
        auto value3 = static_cast<SampleType> (buffer[index3]);
        auto value4 = static_cast<SampleType> (buffer[index4]);
        auto value5 = static_cast<SampleType> (buffer[index5]);
        auto value6 = static_cast<SampleType> (buffer[index6]);

        auto d1 = delayFrac - static_cast<NumericType>(1.0);
        auto d2 = delayFrac - static_cast<NumericType>(2.0);
        auto d3 = delayFrac - static_cast<NumericType>(3.0);
        auto d4 = delayFrac - static_cast<NumericType>(4.0);
        auto d5 = delayFrac - static_cast<NumericType>(5.0);

        auto c1 = -d1 * d2 * d3 * d4 * d5 / static_cast<NumericType>(120.0);
        auto c2 = d2 * d3 * d4 * d5 / static_cast<NumericType>(24.0);
        auto c3 = -d1 * d3 * d4 * d5 / static_cast<NumericType>(12.0);
        auto c4 = d1 * d2 * d4 * d5 / static_cast<NumericType>(12.0);
        auto c5 = -d1 * d2 * d3 * d5 / static_cast<NumericType>(24.0);
        auto c6 = d1 * d2 * d3 * d4 / static_cast<NumericType>(120.0);

        return value1 * c1 + static_cast<SampleType>(delayFrac) *
            (value2 * c2 + value3 * c3 + value4 * c4 + value5 * c5 + value6 * c6);
    }
};

#endif
#include "tests.h"

namespace MarsTester
{
    TEST(TestTopic, SimpleTest)
    {
        EXPECT_EQ(easyTest(), true);
    }
}
#include <gtest/gtest.h>

#include "ui/hold_detector.hpp"

using namespace ui;

TEST(HoldDetectorTest, DoesNotFireBeforeThreshold)
{
    HoldDetector detector{2500};

    EXPECT_FALSE(detector.update(true, 1000));
    EXPECT_FALSE(detector.update(true, 3499));
}

TEST(HoldDetectorTest, FiresExactlyAtThreshold)
{
    HoldDetector detector{2500};

    EXPECT_FALSE(detector.update(true, 1000));
    EXPECT_TRUE(detector.update(true, 3500));
}

TEST(HoldDetectorTest, DoesNotRepeatWhileStillPressed)
{
    HoldDetector detector{2500};

    EXPECT_FALSE(detector.update(true, 1000));
    EXPECT_TRUE(detector.update(true, 3500));
    EXPECT_FALSE(detector.update(true, 5000));
}

TEST(HoldDetectorTest, ReleasingBeforeThresholdRestartsMeasurement)
{
    HoldDetector detector{2500};

    EXPECT_FALSE(detector.update(true, 1000));
    EXPECT_FALSE(detector.update(true, 2000));
    EXPECT_FALSE(detector.update(false, 2100));
    EXPECT_FALSE(detector.update(true, 3000));
    EXPECT_FALSE(detector.update(true, 5000));
    EXPECT_TRUE(detector.update(true, 5500));
}

TEST(HoldDetectorTest, RearmsAfterRelease)
{
    HoldDetector detector{2500};

    EXPECT_FALSE(detector.update(true, 1000));
    EXPECT_TRUE(detector.update(true, 3500));
    EXPECT_FALSE(detector.update(false, 3600));
    EXPECT_FALSE(detector.update(true, 4000));
    EXPECT_TRUE(detector.update(true, 6500));
}

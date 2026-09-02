#include <gtest/gtest.h>
#include "hal/ws2812_strip.hpp"

using namespace hal;

class Ws2812StripTest : public ::testing::Test {
protected:
    Ws2812Strip strip{6, 9}; // Pin 6, 9 pixels
};

TEST_F(Ws2812StripTest, InitializesAllPixelsToZero)
{
    EXPECT_EQ(strip.get_num_pixels(), 9);

    for (uint8_t i = 0; i < strip.get_num_pixels(); ++i) {
        RgbColor c = strip.get_pixel(i);
        EXPECT_EQ(c.r, 0);
        EXPECT_EQ(c.g, 0);
        EXPECT_EQ(c.b, 0);
    }
}

TEST_F(Ws2812StripTest, SetsAndGetsIndividualPixelRgb)
{
    strip.set_pixel(0, 255, 20, 120); // Softener pink
    strip.set_pixel(2, 0, 220, 255);  // Level cyan
    strip.set_pixel(6, 255, 255, 255); // Program white

    RgbColor p0 = strip.get_pixel(0);
    EXPECT_EQ(p0.r, 255);
    EXPECT_EQ(p0.g, 20);
    EXPECT_EQ(p0.b, 120);

    RgbColor p2 = strip.get_pixel(2);
    EXPECT_EQ(p2.r, 0);
    EXPECT_EQ(p2.g, 220);
    EXPECT_EQ(p2.b, 255);

    RgbColor p6 = strip.get_pixel(6);
    EXPECT_EQ(p6.r, 255);
    EXPECT_EQ(p6.g, 255);
    EXPECT_EQ(p6.b, 255);
}

TEST_F(Ws2812StripTest, SetsPixelUsingRgbColorStruct)
{
    RgbColor cyan{0, 200, 250};
    strip.set_pixel(3, cyan);

    EXPECT_EQ(strip.get_pixel(3), cyan);
}

TEST_F(Ws2812StripTest, IgnoresOutOfBoundsPixelIndicesSafely)
{
    // Strip has 9 pixels (0..8). Index 9, 10, 255 are out of bounds.
    strip.set_pixel(9, 255, 255, 255);
    strip.set_pixel(255, 255, 255, 255);

    RgbColor out = strip.get_pixel(9);
    EXPECT_EQ(out.r, 0);
    EXPECT_EQ(out.g, 0);
    EXPECT_EQ(out.b, 0);
}

TEST_F(Ws2812StripTest, ClearsAllPixelsBackToBlack)
{
    strip.set_pixel(0, 255, 255, 255);
    strip.set_pixel(1, 100, 100, 100);
    strip.set_pixel(8, 50, 50, 50);

    strip.clear();

    for (uint8_t i = 0; i < strip.get_num_pixels(); ++i) {
        EXPECT_EQ(strip.get_pixel(i), RgbColor(0, 0, 0));
    }
}

TEST_F(Ws2812StripTest, ShowExecutesWithoutCrashingAndTracksCount)
{
    EXPECT_EQ(strip.get_show_count(), 0u);
    strip.show();
    EXPECT_EQ(strip.get_show_count(), 1u);
    strip.show();
    EXPECT_EQ(strip.get_show_count(), 2u);
}

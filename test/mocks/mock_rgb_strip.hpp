#pragma once

#include <gmock/gmock.h>
#include "hal/interfaces/i_rgb_strip.hpp"

namespace mocks {

class MockRgbStrip : public hal::IRgbStrip {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, set_pixel, (uint8_t index, uint8_t r, uint8_t g, uint8_t b), (override));
    MOCK_METHOD(void, set_pixel, (uint8_t index, const hal::RgbColor& color), (override));
    MOCK_METHOD(hal::RgbColor, get_pixel, (uint8_t index), (const, override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(void, show, (), (override));
    MOCK_METHOD(uint8_t, get_num_pixels, (), (const, override));
};

} // namespace mocks

#pragma once

#include "interfaces/i_rgb_strip.hpp"

namespace hal {

/**
 * @class Ws2812Strip
 * @brief Zero-heap, cycle-accurate WS2812B (NeoPixel) LED strip driver for ATmega328P and Host PC testing.
 */
class Ws2812Strip : public IRgbStrip {
public:
    static constexpr uint8_t k_max_pixels{16};

    explicit Ws2812Strip(uint8_t pin = 6, uint8_t num_pixels = 9);
    ~Ws2812Strip() override = default;

    void init() override;
    void set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) override;
    void set_pixel(uint8_t index, const RgbColor& color) override;
    RgbColor get_pixel(uint8_t index) const override;
    void clear() override;
    void show() override;
    uint8_t get_num_pixels() const override { return num_pixels_; }

    // Inspection helpers for host tests
    uint32_t get_show_count() const { return show_count_; }

private:
    uint8_t pin_{6};
    uint8_t num_pixels_{9};
    uint8_t raw_buffer_[k_max_pixels * 3]{0}; // GRB byte ordering for WS2812
    uint32_t show_count_{0};
};

} // namespace hal

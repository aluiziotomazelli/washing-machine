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

    /**
     * @brief Construct a WS2812 strip driver.
     * @param pin GPIO data pin connected to the strip DIN line (default D6).
     * @param num_pixels Number of addressable pixels (default 9, capped at k_max_pixels).
     */
    explicit Ws2812Strip(uint8_t pin = 6, uint8_t num_pixels = 9);
    ~Ws2812Strip() override = default;

    /** @copydoc IRgbStrip::init */
    void init() override;
    /** @copydoc IRgbStrip::set_pixel(uint8_t, uint8_t, uint8_t, uint8_t) */
    void set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) override;
    /** @copydoc IRgbStrip::set_pixel(uint8_t, const RgbColor&) */
    void set_pixel(uint8_t index, const RgbColor& color) override;
    /** @copydoc IRgbStrip::get_pixel */
    RgbColor get_pixel(uint8_t index) const override;
    /** @copydoc IRgbStrip::clear */
    void clear() override;
    /** @copydoc IRgbStrip::show */
    void show() override;
    /** @copydoc IRgbStrip::get_num_pixels */
    uint8_t get_num_pixels() const override { return num_pixels_; }

    /**
     * @brief Number of show() calls issued since boot (host-test inspection helper).
     * @return Monotonic frame counter.
     */
    uint32_t get_show_count() const { return show_count_; }

private:
    uint8_t pin_{6};
    uint8_t num_pixels_{9};
    uint8_t raw_buffer_[k_max_pixels * 3]{0}; // GRB byte ordering for WS2812
    uint32_t show_count_{0};
};

} // namespace hal

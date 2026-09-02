#pragma once

#include <stdint.h>

namespace hal {

/**
 * @brief Standard 24-bit RGB Color structure.
 */
struct RgbColor {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};

    constexpr RgbColor() = default;
    constexpr RgbColor(uint8_t red, uint8_t green, uint8_t blue)
        : r(red), g(green), b(blue) {}

    bool operator==(const RgbColor& other) const {
        return (r == other.r) && (g == other.g) && (b == other.b);
    }

    bool operator!=(const RgbColor& other) const {
        return !(*this == other);
    }
};

/**
 * @interface IRgbStrip
 * @brief Abstract interface for addressable RGB LED strips (e.g. WS2812B, WS2813, SK6812).
 */
class IRgbStrip {
public:
    virtual ~IRgbStrip() = default;

    /**
     * @brief Initialize hardware pin / peripheral driver.
     */
    virtual void init() = 0;

    /**
     * @brief Set RGB color of a specific pixel in the buffer.
     * @param index Pixel index (0-indexed).
     * @param r Red component (0..255).
     * @param g Green component (0..255).
     * @param b Blue component (0..255).
     */
    virtual void set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) = 0;

    /**
     * @brief Set RGB color of a specific pixel in the buffer using RgbColor struct.
     */
    virtual void set_pixel(uint8_t index, const RgbColor& color) = 0;

    /**
     * @brief Get current RGB color of a pixel from the buffer.
     */
    virtual RgbColor get_pixel(uint8_t index) const = 0;

    /**
     * @brief Clear all pixels in buffer to (0, 0, 0).
     */
    virtual void clear() = 0;

    /**
     * @brief Push pixel buffer out to physical hardware strip.
     */
    virtual void show() = 0;

    /**
     * @brief Get total number of configured pixels in strip.
     */
    virtual uint8_t get_num_pixels() const = 0;
};

} // namespace hal

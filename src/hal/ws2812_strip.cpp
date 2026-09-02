#include "ws2812_strip.hpp"

#if defined(__AVR__)
#include <Arduino.h>
#include <avr/interrupt.h>
#endif

namespace hal {

Ws2812Strip::Ws2812Strip(uint8_t pin, uint8_t num_pixels)
    : pin_(pin),
      num_pixels_(num_pixels > k_max_pixels ? k_max_pixels : num_pixels)
{
    clear();
}

void Ws2812Strip::init()
{
#if defined(__AVR__)
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
#endif
    clear();
}

void Ws2812Strip::set_pixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= num_pixels_) {
        return;
    }

    uint16_t offset = static_cast<uint16_t>(index) * 3;
    // WS2812 standard byte order is GRB (Green, Red, Blue)
    raw_buffer_[offset + 0] = g;
    raw_buffer_[offset + 1] = r;
    raw_buffer_[offset + 2] = b;
}

void Ws2812Strip::set_pixel(uint8_t index, const RgbColor& color)
{
    set_pixel(index, color.r, color.g, color.b);
}

RgbColor Ws2812Strip::get_pixel(uint8_t index) const
{
    if (index >= num_pixels_) {
        return RgbColor{0, 0, 0};
    }

    uint16_t offset = static_cast<uint16_t>(index) * 3;
    return RgbColor{
        raw_buffer_[offset + 1], // Red
        raw_buffer_[offset + 0], // Green
        raw_buffer_[offset + 2]  // Blue
    };
}

void Ws2812Strip::clear()
{
    for (uint16_t i = 0; i < sizeof(raw_buffer_); ++i) {
        raw_buffer_[i] = 0;
    }
}

void Ws2812Strip::show()
{
    show_count_++;

#if defined(__AVR__)
    if (num_pixels_ == 0) {
        return;
    }

    volatile uint8_t *port = portOutputRegister(digitalPinToPort(pin_));
    uint8_t pin_mask = digitalPinToBitMask(pin_);
    uint8_t hi = *port | pin_mask;
    uint8_t lo = *port & ~pin_mask;
    uint8_t *ptr = raw_buffer_;
    uint16_t num_bytes = num_pixels_ * 3;

    uint8_t next_lo, count, bit_val;

    uint8_t old_sreg = SREG;
    cli();

    // 16 MHz AVR cycle-accurate 800 kHz WS2812 transmission loop
    asm volatile(
        "1:\n\t"
        "ld   %[bit_val], %a[ptr]+\n\t" // 2 cycles: load byte
        "ldi  %[count], 8\n\t"         // 1 cycle: 8 bits counter
        "2:\n\t"
        "st   %a[port], %[hi]\n\t"     // 2 cycles: PIN HIGH (T0H/T1H start)
        "sbrs %[bit_val], 7\n\t"        // 1-2 cycles: skip if bit is 1
        "mov  %[next_lo], %[lo]\n\t"   // 1 cycle: if bit 0, set next to LO
        "lsl  %[bit_val]\n\t"          // 1 cycle: shift next bit to MSB
        "nop\n\t"                      // 1 cycle: padding
        "st   %a[port], %[next_lo]\n\t"// 2 cycles: set pin to LO if bit was 0, or keep HI if bit was 1
        "mov  %[next_lo], %[hi]\n\t"   // 1 cycle: restore for next loop
        "dec  %[count]\n\t"            // 1 cycle: decrement bit counter
        "nop\n\t"
        "nop\n\t"
        "st   %a[port], %[lo]\n\t"     // 2 cycles: PIN LOW (end of bit)
        "brne 2b\n\t"                  // 2 cycles: loop for all 8 bits
        "sbiw %[num_bytes], 1\n\t"     // 2 cycles: decrement byte count
        "brne 1b\n\t"                  // 2 cycles: loop for all bytes
        : [ptr] "+e" (ptr),
          [num_bytes] "+w" (num_bytes),
          [bit_val] "=&r" (bit_val),
          [count] "=&r" (count),
          [next_lo] "=&r" (next_lo)
        : [port] "e" (port),
          [hi] "r" (hi),
          [lo] "r" (lo)
    );

    SREG = old_sreg;
#endif
}

} // namespace hal

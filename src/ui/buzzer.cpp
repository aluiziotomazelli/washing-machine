#include "buzzer.hpp"

namespace ui {

Buzzer::Buzzer(
    hal::IGpioHAL& gpio_hal,
    hal::ITimerHAL& timer_hal,
    uint8_t pin,
    uint16_t frequency_hz
)
    : gpio_hal_(gpio_hal)
    , timer_hal_(timer_hal)
    , pin_(pin)
    , frequency_hz_(frequency_hz)
    , is_initialized_(false)
    , is_playing_(false)
    , is_sounding_(false)
    , continuous_(false)
    , on_time_ms_(0)
    , off_time_ms_(0)
    , remaining_beeps_(0)
    , phase_start_time_ms_(0)
{
}

void Buzzer::init()
{
    gpio_hal_.set_mode(pin_, hal::GpioMode::MODE_OUTPUT);
    apply_sound(false);
    is_initialized_ = true;
}

void Buzzer::beep(uint16_t duration_ms)
{
    start_sequence(duration_ms, 0, 1, false);
}

void Buzzer::play_pattern(BuzzerPattern pattern)
{
    switch (pattern) {
    case BuzzerPattern::SHORT_BEEP:
        start_sequence(50, 0, 1, false);
        break;
    case BuzzerPattern::DOUBLE_BEEP:
        start_sequence(60, 60, 2, false);
        break;
    case BuzzerPattern::LONG_BEEP:
        start_sequence(500, 0, 1, false);
        break;
    case BuzzerPattern::CYCLE_FINISHED:
        start_sequence(120, 100, 4, false);
        break;
    case BuzzerPattern::ERROR_ALARM:
        start_sequence(250, 250, 1, true);
        break;
    case BuzzerPattern::NONE:
    default:
        stop();
        break;
    }
}

void Buzzer::stop()
{
    is_playing_ = false;
    is_sounding_ = false;
    continuous_ = false;
    remaining_beeps_ = 0;
    apply_sound(false);
}

bool Buzzer::is_playing() const
{
    return is_playing_;
}

void Buzzer::update()
{
    if (!is_initialized_ || !is_playing_) {
        return;
    }

    uint32_t now = timer_hal_.get_time_ms();

    if (is_sounding_) {
        if (now - phase_start_time_ms_ >= on_time_ms_) {
            apply_sound(false);
            is_sounding_ = false;
            phase_start_time_ms_ = now;

            if (off_time_ms_ == 0 || (!continuous_ && remaining_beeps_ == 0)) {
                is_playing_ = false;
            }
        }
    } else {
        if (now - phase_start_time_ms_ >= off_time_ms_) {
            if (remaining_beeps_ > 0) {
                remaining_beeps_--;
                is_sounding_ = true;
                phase_start_time_ms_ = now;
                apply_sound(true);
            } else if (continuous_) {
                is_sounding_ = true;
                phase_start_time_ms_ = now;
                apply_sound(true);
            } else {
                is_playing_ = false;
            }
        }
    }
}

void Buzzer::apply_sound(bool on)
{
    if (on) {
        gpio_hal_.play_tone(pin_, frequency_hz_);
    } else {
        gpio_hal_.stop_tone(pin_);
    }
}

void Buzzer::start_sequence(uint16_t on_ms, uint16_t off_ms, uint8_t count, bool continuous)
{
    if (!is_initialized_) {
        return;
    }

    on_time_ms_ = on_ms;
    off_time_ms_ = off_ms;
    remaining_beeps_ = (count > 0) ? (count - 1) : 0;
    continuous_ = continuous;
    phase_start_time_ms_ = timer_hal_.get_time_ms();
    is_playing_ = true;
    is_sounding_ = true;

    apply_sound(true);
}

} // namespace ui

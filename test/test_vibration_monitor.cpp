#include <gtest/gtest.h>
#include "controllers/vibration_monitor.hpp"
#include "mocks/mock_accelerometer.hpp"
#include "mocks/mock_timer_hal.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;

class VibrationMonitorTest : public ::testing::Test {
protected:
    mocks::MockAccelerometer mock_accel;
    NiceMock<mocks::MockTimerHAL> mock_timer;

    controllers::VibrationConfig config{
        20,    // 20 ms sample period (50 Hz)
        10,    // 10 samples per window (200 ms)
        400,   // 400 LSB motion threshold
        8500,  // 8500 LSB warning threshold
        11000, // 11000 LSB sustained trip threshold
        14000, // 14000 LSB immediate shock threshold
        1000   // 1000 ms sustained trip duration
    };

    controllers::VibrationMonitor monitor{mock_accel, mock_timer, config};
    uint32_t current_time_ms{0};

    void SetUp() override
    {
        current_time_ms = 0;
        ON_CALL(mock_timer, get_time_ms()).WillByDefault(Invoke([this]() {
            return current_time_ms;
        }));
    }

    // Helper: Feed a window of 10 samples spanning min/max values
    void feed_window(int16_t x_min, int16_t x_max,
                     int16_t y_min, int16_t y_max,
                     int16_t z_min, int16_t z_max)
    {
        for (int i = 0; i < 10; ++i) {
            current_time_ms += 20;
            hal::Vector3 sample;
            if (i == 0) {
                sample = {x_min, y_min, z_min};
            } else if (i == 1) {
                sample = {x_max, y_max, z_max};
            } else {
                sample = {static_cast<int16_t>((x_min + x_max) / 2),
                          static_cast<int16_t>((y_min + y_max) / 2),
                          static_cast<int16_t>((z_min + z_max) / 2)};
            }

            EXPECT_CALL(mock_accel, read_accel(_))
                .WillOnce(DoAll(SetArgReferee<0>(sample), Return(true)));

            monitor.update();
        }
    }
};

TEST_F(VibrationMonitorTest, InitializesInCleanState)
{
    monitor.init();
    EXPECT_EQ(monitor.get_vibration(), 0);
    EXPECT_FALSE(monitor.is_in_motion());
    EXPECT_FALSE(monitor.is_warning());
    EXPECT_FALSE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, EnforcesSamplingPeriod)
{
    // First sample at t = 20ms
    current_time_ms = 20;
    EXPECT_CALL(mock_accel, read_accel(_)).Times(1).WillOnce(Return(true));
    monitor.update();

    // Call again before 20ms elapsed -> should not sample
    current_time_ms = 30;
    EXPECT_CALL(mock_accel, read_accel(_)).Times(0);
    monitor.update();

    // Call at t = 40ms -> should sample
    current_time_ms = 40;
    EXPECT_CALL(mock_accel, read_accel(_)).Times(1).WillOnce(Return(true));
    monitor.update();
}

TEST_F(VibrationMonitorTest, CalculatesPeakToPeakEnvelopeCorrectly)
{
    // dx = 300, dy = 400, dz = 100 -> Vib = 800
    feed_window(0, 300, 0, 400, 0, 100);

    EXPECT_EQ(monitor.get_vibration(), 800);
    EXPECT_TRUE(monitor.is_in_motion()); // 800 > 400
    EXPECT_FALSE(monitor.is_warning());  // 800 < 8500
    EXPECT_FALSE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, RejectsConstantStaticGravityOffset)
{
    // 1g Earth gravity static on X (-7700 LSB), completely still
    feed_window(-7700, -7700, 50, 50, 800, 800);

    EXPECT_EQ(monitor.get_vibration(), 0);
    EXPECT_FALSE(monitor.is_in_motion());
    EXPECT_FALSE(monitor.is_warning());
    EXPECT_FALSE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, DetectsWarningThreshold)
{
    // dx = 3000, dy = 3000, dz = 3000 -> Vib = 9000
    feed_window(0, 3000, 0, 3000, 0, 3000);

    EXPECT_EQ(monitor.get_vibration(), 9000);
    EXPECT_TRUE(monitor.is_in_motion());
    EXPECT_TRUE(monitor.is_warning());
    EXPECT_FALSE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, TripsImmediatelyOnSevereShockAbove14000)
{
    // Single window with Vib = 14500 (shock limit)
    feed_window(0, 5000, 0, 5000, 0, 4500);

    EXPECT_EQ(monitor.get_vibration(), 14500);
    EXPECT_TRUE(monitor.is_warning());
    EXPECT_TRUE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, TripsWhenVibrationAbove11000IsSustainedForOneSecond)
{
    // Vib = 11500 (above 11000, below 14000)
    // Window 1 (t = 200 ms): trip started, but not yet sustained
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_EQ(monitor.get_vibration(), 11500);
    EXPECT_FALSE(monitor.is_critical_unbalance());

    // Window 2 (t = 400 ms)
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_FALSE(monitor.is_critical_unbalance());

    // Window 3 (t = 600 ms)
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_FALSE(monitor.is_critical_unbalance());

    // Window 4 (t = 800 ms)
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_FALSE(monitor.is_critical_unbalance());

    // Window 5 (t = 1000 ms)
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_FALSE(monitor.is_critical_unbalance());

    // Window 6 (t = 1200 ms): sustained >= 1000 ms elapsed -> TRIPPED!
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_TRUE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, RejectsTransientSpikeAbove11000WithoutTripping)
{
    // Two windows above 11000 (400 ms)
    feed_window(0, 4000, 0, 4000, 0, 3500);
    feed_window(0, 4000, 0, 4000, 0, 3500);
    EXPECT_FALSE(monitor.is_critical_unbalance());

    // Then vibration subsides to calm level (Vib = 2000)
    for (int w = 0; w < 10; ++w) {
        feed_window(0, 700, 0, 700, 0, 600);
        EXPECT_FALSE(monitor.is_critical_unbalance());
    }

    EXPECT_FALSE(monitor.is_critical_unbalance());
}

TEST_F(VibrationMonitorTest, ResetClearsTripAndEnvelope)
{
    // Trip it with a shock
    feed_window(0, 5000, 0, 5000, 0, 4500);
    EXPECT_TRUE(monitor.is_critical_unbalance());

    monitor.reset();
    EXPECT_EQ(monitor.get_vibration(), 0);
    EXPECT_FALSE(monitor.is_critical_unbalance());
    EXPECT_FALSE(monitor.is_warning());
    EXPECT_FALSE(monitor.is_in_motion());
}

TEST_F(VibrationMonitorTest, DisabledMonitorIgnoresUpdates)
{
    monitor.set_enabled(false);
    EXPECT_FALSE(monitor.is_enabled());

    current_time_ms = 100;
    EXPECT_CALL(mock_accel, read_accel(_)).Times(0);
    monitor.update();
}

TEST_F(VibrationMonitorTest, TracksSensorCommunicationHealth)
{
    // Initially sensor_ok is false before any successful read
    EXPECT_FALSE(monitor.is_sensor_ok());

    // Successful read -> is_sensor_ok becomes true
    current_time_ms = 20;
    hal::Vector3 sample{100, 200, 300};
    EXPECT_CALL(mock_accel, read_accel(_))
        .WillOnce(DoAll(SetArgReferee<0>(sample), Return(true)));
    monitor.update();
    EXPECT_TRUE(monitor.is_sensor_ok());

    // Sensor disconnected / failing for > 200ms -> is_sensor_ok becomes false
    current_time_ms = 300;
    EXPECT_CALL(mock_accel, read_accel(_))
        .WillOnce(Return(false));
    monitor.update();
    EXPECT_FALSE(monitor.is_sensor_ok());
    EXPECT_EQ(monitor.get_vibration(), 0);
}

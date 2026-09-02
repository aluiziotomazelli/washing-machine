#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mocks/mock_watchdog_hal.hpp"

using ::testing::NiceMock;
using ::testing::Return;

class WatchdogTest : public ::testing::Test {
protected:
    NiceMock<mocks::MockWatchdogHAL> mock_watchdog;
};

TEST_F(WatchdogTest, EnablesWatchdogWithConfiguredTimeout)
{
    EXPECT_CALL(mock_watchdog, enable(hal::WatchdogTimeout::TIMEOUT_2S)).Times(1);
    mock_watchdog.enable(hal::WatchdogTimeout::TIMEOUT_2S);
}

TEST_F(WatchdogTest, KickingWatchdogResetsTimer)
{
    EXPECT_CALL(mock_watchdog, kick()).Times(3);
    mock_watchdog.kick();
    mock_watchdog.kick();
    mock_watchdog.kick();
}

TEST_F(WatchdogTest, ReportsWatchdogResetWhenFlagWasSet)
{
    EXPECT_CALL(mock_watchdog, was_reset_by_watchdog()).WillOnce(Return(true));
    EXPECT_TRUE(mock_watchdog.was_reset_by_watchdog());

    EXPECT_CALL(mock_watchdog, was_reset_by_watchdog()).WillOnce(Return(false));
    EXPECT_FALSE(mock_watchdog.was_reset_by_watchdog());
}

TEST_F(WatchdogTest, DisablesWatchdogCleanly)
{
    EXPECT_CALL(mock_watchdog, disable()).Times(1);
    mock_watchdog.disable();
}

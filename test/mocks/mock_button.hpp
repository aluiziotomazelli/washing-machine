#pragma once

#include <gmock/gmock.h>
#include "ui/interfaces/i_button.hpp"

namespace mocks {

class MockButton : public ui::IButton {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, deinit, (), (override));
    MOCK_METHOD(void, update, (), (override));
    MOCK_METHOD(ui::ButtonClickType, get_last_click, (), (override));
    MOCK_METHOD(bool, is_pressed, (), (const, override));
};

} // namespace mocks

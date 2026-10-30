#pragma once
#include "LV_Interface/LVScreen.h"
#include "Devices/Input.h"
#include "Util/Events.h"
#include <array>
#include <cstdint>

class HackerModeScreen : public LVScreen {
public:
    HackerModeScreen();
    ~HackerModeScreen() override;
private:
    void onStart() override;
    void onStop() override;
    void loop() override;
    void render();
    EventQueue events;
    std::array<lv_obj_t*, 16> columns{};
    std::array<uint8_t, 16> offsets{};
    uint64_t lastFrameUs = 0;
    bool amber = false;
};

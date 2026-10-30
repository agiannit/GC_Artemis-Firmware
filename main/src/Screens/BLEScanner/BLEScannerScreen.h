#pragma once
#include "LV_Interface/LVScreen.h"
#include "Devices/Input.h"
#include "Util/Events.h"
#include "BLEScanService.h"
#include <array>
#include <vector>
#include <cstdint>

class BLEScannerScreen : public LVScreen {
public:
    BLEScannerScreen();
    ~BLEScannerScreen() override;
private:
    void onStart() override;
    void onStop() override;
    void loop() override;
    void draw();
    BLEScanService ble;
    EventQueue events;
    std::vector<BLEScanService::Device> devices;
    lv_obj_t* title = nullptr;
    lv_obj_t* status = nullptr;
    static constexpr int VisibleRows = 4;
    std::array<lv_obj_t*, VisibleRows> names{};
    std::array<lv_obj_t*, VisibleRows> bars{};
    std::array<lv_obj_t*, VisibleRows> values{};
    int selected = 0;
    uint64_t lastUpdateUs = 0;
};

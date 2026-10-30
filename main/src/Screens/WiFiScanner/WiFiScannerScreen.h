#pragma once
#include "LV_Interface/LVScreen.h"
#include "Devices/Input.h"
#include "Util/Events.h"
#include "WiFiScanService.h"
#include <vector>
#include <array>
#include <cstdint>

class WiFiScannerScreen : public LVScreen {
public:
    WiFiScannerScreen();
    ~WiFiScannerScreen() override;
private:
    void onStart() override;
    void onStop() override;
    void loop() override;
    void draw();
    void scan();

    WiFiScanService wifi;
    EventQueue events;
    std::vector<WiFiScanService::APEntry> aps;
    lv_obj_t* title = nullptr;
    lv_obj_t* status = nullptr;
    static constexpr int VisibleRows = 4;
    std::array<lv_obj_t*, VisibleRows> names{};
    std::array<lv_obj_t*, VisibleRows> bars{};
    std::array<lv_obj_t*, VisibleRows> values{};
    uint64_t lastScanUs = 0;
    int selected = 0;
    bool scanning = false;
    bool ready = false;
};

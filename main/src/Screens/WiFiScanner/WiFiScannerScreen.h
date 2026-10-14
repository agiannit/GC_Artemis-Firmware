#pragma once
#include "LV_Interface/LVScreen.h"
#include "Devices/Input.h"
#include "Util/Events.h"
#include "WiFiScanService.h"
#include <vector>

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
    lv_obj_t* detail = nullptr;
    int selected = 0;
    bool scanning = false;
};

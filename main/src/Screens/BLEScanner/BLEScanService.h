#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <esp_gap_ble_api.h>

class BLEScanService {
public:
    struct Device {
        std::string name;
        std::string address;
        int rssi = -100;
    };
    bool start();
    void stop();
    void snapshot(std::vector<Device>& out);
    static void onGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
private:
    struct Record {
        uint8_t address[6]{};
        char name[24]{};
        int8_t rssi = -100;
    };
    static BLEScanService* active;
    portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;
    std::array<Record, 40> records{};
    int count = 0;
    bool enabled = false;
    bool scanReady = false;
    bool scanning = false;
    void handle(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
};

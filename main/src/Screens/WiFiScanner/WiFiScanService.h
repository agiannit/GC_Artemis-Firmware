#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <esp_event.h>
#include <esp_wifi_types.h>

class WiFiScanService {
public:
    struct APEntry {
        std::string ssid;
        int8_t rssi = 0;
        uint8_t channel = 0;
        wifi_auth_mode_t auth = WIFI_AUTH_OPEN;
    };

    WiFiScanService() = default;
    ~WiFiScanService();

    bool begin();
    bool startScan();
    bool scanFinished();
    bool readResults(std::vector<APEntry>& out);
    void stop();

private:
    static void onWifiEvent(void* arg, esp_event_base_t base, int32_t id, void* data);
    esp_event_handler_instance_t handler = nullptr;
    std::atomic<bool> finished{false};
    bool initialized = false;
    bool running = false;
    bool ownsEventLoop = false;
};

#include "WiFiScanService.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include <algorithm>
#include <cstring>

WiFiScanService::~WiFiScanService() { stop(); }

bool WiFiScanService::begin() {
    if(initialized) return true;
    esp_err_t err = esp_event_loop_create_default();
    if(err == ESP_OK) ownsEventLoop = true;
    else if(err != ESP_ERR_INVALID_STATE) return false;
    err = esp_netif_init();
    if(err != ESP_OK && err != ESP_ERR_INVALID_STATE) { stop(); return false; }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&cfg) != ESP_OK) { stop(); return false; }
    initialized = true;
    if(esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK ||
       esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
       esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
           &WiFiScanService::onWifiEvent, this, &handler) != ESP_OK ||
       esp_wifi_start() != ESP_OK) {
        stop();
        return false;
    }
    running = true;
    return true;
}

void WiFiScanService::onWifiEvent(void* arg, esp_event_base_t, int32_t id, void*) {
    if(id == WIFI_EVENT_SCAN_DONE) static_cast<WiFiScanService*>(arg)->finished.store(true);
}

bool WiFiScanService::startScan() {
    if(!running) return false;
    finished.store(false);
    wifi_scan_config_t cfg = {};
    cfg.channel = 0;
    cfg.scan_type = WIFI_SCAN_TYPE_PASSIVE;
    cfg.show_hidden = true;
    return esp_wifi_scan_start(&cfg, false) == ESP_OK;
}

bool WiFiScanService::scanFinished() { return finished.exchange(false); }

bool WiFiScanService::readResults(std::vector<APEntry>& out) {
    out.clear();
    if(!running) return false;
    uint16_t count = 0;
    if(esp_wifi_scan_get_ap_num(&count) != ESP_OK) return false;
    constexpr uint16_t MaxResults = 40;
    wifi_ap_record_t records[MaxResults] = {};
    count = std::min<uint16_t>(count, MaxResults);
    if(count == 0) return true;
    if(esp_wifi_scan_get_ap_records(&count, records) != ESP_OK) return false;
    out.reserve(count);
    for(uint16_t i = 0; i < count; ++i) {
        APEntry ap;
        auto* name = reinterpret_cast<const char*>(records[i].ssid);
        ap.ssid.assign(name, strnlen(name, sizeof(records[i].ssid)));
        ap.rssi = records[i].rssi;
        ap.channel = records[i].primary;
        ap.auth = records[i].authmode;
        out.push_back(std::move(ap));
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a.rssi > b.rssi;
    });
    return true;
}

void WiFiScanService::stop() {
    if(initialized) {
        esp_wifi_scan_stop();
        if(handler) {
            esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, handler);
            handler = nullptr;
        }
        esp_wifi_stop();
        esp_wifi_deinit();
        initialized = false;
    }
    running = false;
    finished.store(false);
    if(ownsEventLoop) {
        esp_event_loop_delete_default();
        ownsEventLoop = false;
    }
}

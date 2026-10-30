#include "BLEScanService.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

BLEScanService* BLEScanService::active = nullptr;

bool BLEScanService::start() {
    if(active && active != this) return false;
    active = this;
    enabled = true;
    scanReady = false;
    scanning = false;
    portENTER_CRITICAL(&mutex);
    count = 0;
    portEXIT_CRITICAL(&mutex);
    esp_ble_scan_params_t params{};
    params.scan_type = BLE_SCAN_TYPE_ACTIVE;
    params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
    params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
    params.scan_interval = 0x80;
    params.scan_window = 0x40;
    params.scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE;
    if(esp_ble_gap_set_scan_params(&params) != ESP_OK) {
        enabled = false;
        active = nullptr;
        return false;
    }
    return true;
}

void BLEScanService::stop() {
    enabled = false;
    if(active == this) active = nullptr;
    esp_ble_gap_stop_scanning();
}

void BLEScanService::onGapEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    if(active) active->handle(event, param);
}

void BLEScanService::handle(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    if(!enabled) return;
    if(event == ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT) {
        if(param->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            scanReady = true;
            esp_ble_gap_start_scanning(8);
        }
    } else if(event == ESP_GAP_BLE_SCAN_START_COMPLETE_EVT) {
        scanning = param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS;
    } else if(event == ESP_GAP_BLE_SCAN_RESULT_EVT) {
        auto& result = param->scan_rst;
        if(result.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            scanning = false;
            if(enabled && scanReady) esp_ble_gap_start_scanning(8);
            return;
        }
        if(result.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT) return;
        char name[24]{};
        uint8_t len = 0;
        uint8_t* raw = esp_ble_resolve_adv_data(result.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &len);
        if(!raw || !len) raw = esp_ble_resolve_adv_data(result.ble_adv, ESP_BLE_AD_TYPE_NAME_SHORT, &len);
        if(raw && len) {
            const size_t n = std::min<size_t>(len, sizeof(name) - 1);
            memcpy(name, raw, n);
        }
        portENTER_CRITICAL(&mutex);
        int index = 0;
        for(; index < count; ++index) {
            if(memcmp(records[index].address, result.bda, 6) == 0) break;
        }
        if(index < (int)records.size()) {
            if(index == count) ++count;
            auto& record = records[index];
            memcpy(record.address, result.bda, 6);
            record.rssi = result.rssi;
            if(name[0]) memcpy(record.name, name, sizeof(name));
        }
        portEXIT_CRITICAL(&mutex);
    }
}

void BLEScanService::snapshot(std::vector<Device>& out) {
    std::array<Record, 40> copy{};
    int n;
    portENTER_CRITICAL(&mutex);
    n = count;
    memcpy(copy.data(), records.data(), sizeof(records));
    portEXIT_CRITICAL(&mutex);
    out.clear();
    out.reserve(n);
    for(int i = 0; i < n; ++i) {
        const auto& r = copy[i];
        char address[18];
        snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                 r.address[0],r.address[1],r.address[2],r.address[3],r.address[4],r.address[5]);
        out.push_back({r.name[0] ? std::string(r.name) : std::string("Unknown"), address, r.rssi});
    }
    std::sort(out.begin(), out.end(), [](const Device& a, const Device& b){ return a.rssi > b.rssi; });
}

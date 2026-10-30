#include "Services/Time.h"
#include "WiFiCredentials.local.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <esp_log.h>
#include <cstring>
#include <cstdlib>
#include <ctime>

namespace {
EventGroupHandle_t connected;
void gotIP(void*, esp_event_base_t, int32_t, void*) { ESP_LOGI("NTP", "Wi-Fi obtained IP address"); xEventGroupSetBits(connected, BIT0); }
void wifiEvent(void*, esp_event_base_t, int32_t id, void* data) {
    if(id == WIFI_EVENT_STA_DISCONNECTED) {
        auto* evt = static_cast<wifi_event_sta_disconnected_t*>(data);
        ESP_LOGW("NTP", "Wi-Fi disconnected, reason=%u", (unsigned)evt->reason);
        xEventGroupSetBits(connected, BIT1);
    }
}
void syncTask(void* arg) {
    Time* clock = static_cast<Time*>(arg);
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();
    if(strcmp(ARTEMIS_WIFI_SSID, "YOUR_WIFI_SSID") == 0) { vTaskDelete(nullptr); return; }
    connected = xEventGroupCreate();
    bool ownLoop = esp_event_loop_create_default() == ESP_OK;
    esp_netif_init();
    esp_netif_t* netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    bool initialized = esp_wifi_init(&init) == ESP_OK;
    esp_event_handler_instance_t handler = nullptr;
    esp_event_handler_instance_t wifiHandler = nullptr;
    bool started = false;
    if(initialized && esp_wifi_set_storage(WIFI_STORAGE_RAM) == ESP_OK &&
       esp_wifi_set_mode(WIFI_MODE_STA) == ESP_OK &&
       esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, gotIP, nullptr, &handler) == ESP_OK &&
       esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifiEvent, nullptr, &wifiHandler) == ESP_OK) {
        wifi_config_t config{};
        strncpy((char*)config.sta.ssid, ARTEMIS_WIFI_SSID, sizeof(config.sta.ssid) - 1);
        strncpy((char*)config.sta.password, ARTEMIS_WIFI_PASSWORD, sizeof(config.sta.password) - 1);
        if(esp_wifi_set_config(WIFI_IF_STA, &config) == ESP_OK && esp_wifi_start() == ESP_OK) {
            started = true;
            if(esp_wifi_connect() == ESP_OK &&
               (xEventGroupWaitBits(connected, BIT0 | BIT1, pdFALSE, pdFALSE, pdMS_TO_TICKS(12000)) & BIT0)) {
                esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                esp_sntp_setservername(0, "pool.ntp.org");
                esp_sntp_init();
                bool success = false;
                for(int i = 0; i < 20; ++i) {
                    time_t now;
                    time(&now);
                    if(now > 1700000000 && esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
                        tm local{};
                        localtime_r(&now, &local);
                        // Time service uses mktime() on RTC wall time and gmtime() for display.
                        // Restore UTC before writing so its conversion does not shift the clock.
                        setenv("TZ", "UTC0", 1);
                        tzset();
                        clock->setTime(local);
                        ESP_LOGI("NTP", "RTC synchronized");
                        success = true;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                if(!success) ESP_LOGW("NTP", "No valid NTP response");
                esp_sntp_stop();
            } else ESP_LOGW("NTP", "Wi-Fi connection failed or timed out");
        }
    }
    if(started) { esp_wifi_disconnect(); esp_wifi_stop(); }
    if(handler) esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, handler);
    if(wifiHandler) esp_event_handler_instance_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifiHandler);
    if(initialized) esp_wifi_deinit();
    if(netif) esp_netif_destroy_default_wifi(netif);
    if(ownLoop) esp_event_loop_delete_default();
    vEventGroupDelete(connected);
    vTaskDelete(nullptr);
}
}
void startNTPTimeSync(Time* clock) {
    xTaskCreate(syncTask, "NTPTimeSync", 6144, clock, 3, nullptr);
}

#include "WiFiScannerScreen.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Services/SleepMan.h"
#include "Util/Services.h"
#include "LV_Interface/InputLVGL.h"
#include <cstdio>
#include <memory>

namespace {
const char* authName(wifi_auth_mode_t auth) {
    switch(auth) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "802.1X";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/3";
        default: return "OTHER";
    }
}
}

WiFiScannerScreen::WiFiScannerScreen() : events(6) {
    lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
    title = lv_label_create(*this);
    lv_obj_set_pos(title, 4, 4);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "Wi-Fi Scanner");

    status = lv_label_create(*this);
    lv_obj_set_pos(status, 4, 25);
    lv_obj_set_style_text_color(status, lv_color_white(), 0);
    lv_label_set_text(status, "Initializing...");

    detail = lv_label_create(*this);
    lv_obj_set_pos(detail, 4, 46);
    lv_obj_set_width(detail, 120);
    lv_obj_set_style_text_color(detail, lv_color_white(), 0);
    lv_label_set_long_mode(detail, LV_LABEL_LONG_WRAP);
    lv_label_set_text(detail, "Select: scan\nAlt: back");
}

WiFiScannerScreen::~WiFiScannerScreen() {
    Events::unlisten(&events);
    wifi.stop();
}

void WiFiScannerScreen::onStart() {
    auto* sleep = (SleepMan*) Services.get(Service::Sleep);
    if(sleep) sleep->enAutoSleep(false);
    Events::listen(Facility::Input, &events);
    lv_indev_wait_release(InputLVGL::getInstance()->getIndev());
    if(wifi.begin()) scan();
    else lv_label_set_text(status, "Wi-Fi init failed");
}

void WiFiScannerScreen::onStop() {
    Events::unlisten(&events);
    wifi.stop();
    auto* sleep = (SleepMan*) Services.get(Service::Sleep);
    if(sleep) sleep->enAutoSleep(true);
}

void WiFiScannerScreen::scan() {
    aps.clear();
    selected = 0;
    scanning = wifi.startScan();
    lv_label_set_text(status, scanning ? "Scanning..." : "Scan failed");
    lv_label_set_text(detail, scanning ? "Passive scan\nPlease wait..." : "Select: retry");
}

void WiFiScannerScreen::draw() {
    if(aps.empty()) {
        lv_label_set_text(status, "0 networks");
        lv_label_set_text(detail, "Select: rescan\nAlt: back");
        return;
    }
    char line[160];
    const auto& ap = aps[selected];
    const auto name = ap.ssid.empty() ? std::string("<hidden>") : ap.ssid;
    lv_label_set_text_fmt(status, "%d/%d networks", selected + 1, (int)aps.size());
    snprintf(line, sizeof(line), "%.20s\nRSSI: %d dBm\nChannel: %u\nSecurity: %s\n\nUp/Down: browse\nSelect: rescan",
             name.c_str(), (int)ap.rssi, (unsigned)ap.channel, authName(ap.auth));
    lv_label_set_text(detail, line);
}

void WiFiScannerScreen::loop() {
    if(scanning && wifi.scanFinished()) {
        scanning = false;
        if(wifi.readResults(aps)) draw();
        else {
            lv_label_set_text(status, "Scan read failed");
            lv_label_set_text(detail, "Select: retry");
        }
    }
    for(Event evt{}; events.get(evt, 0); ) {
        bool exit = false;
        if(evt.facility == Facility::Input) {
            auto* input = (Input::Data*) evt.data;
            if(input->action == Input::Data::Press) {
                if(input->btn == Input::Alt) exit = true;
                else if(input->btn == Input::Select && !scanning) scan();
                else if(!scanning && !aps.empty()) {
                    if(input->btn == Input::Up && selected > 0) { --selected; draw(); }
                    if(input->btn == Input::Down && selected + 1 < (int)aps.size()) { ++selected; draw(); }
                }
            }
        }
        free(evt.data);
        if(exit) {
            transition([](){ return std::make_unique<MainMenu>(); });
            return;
        }
    }
}

#include "WiFiScannerScreen.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Services/SleepMan.h"
#include "Util/Services.h"
#include "LV_Interface/InputLVGL.h"
#include "esp_timer.h"
#include <cstdio>
#include <memory>
#include <algorithm>

namespace {
constexpr uint64_t ScanIntervalUs = 12000000;
int strength(int rssi) {
    return std::max(2, std::min(100, (rssi + 100) * 2));
}
lv_color_t signalColour(int rssi) {
    if(rssi >= -50) return lv_color_make(32, 220, 85);
    if(rssi >= -65) return lv_color_make(245, 220, 35);
    if(rssi >= -75) return lv_color_make(255, 135, 20);
    return lv_color_make(245, 55, 55);
}
}

WiFiScannerScreen::WiFiScannerScreen() : events(6) {
    lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
    lv_obj_clear_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
    title = lv_label_create(*this);
    lv_obj_set_pos(title, 3, 2);
    lv_obj_set_style_text_color(title, lv_color_make(70, 210, 255), 0);
    lv_label_set_text(title, "WI-FI SCANNER");

    status = lv_label_create(*this);
    lv_obj_set_pos(status, 3, 17);
    lv_obj_set_style_text_color(status, lv_color_white(), 0);
    lv_label_set_text(status, "Initializing...");

    for(int i = 0; i < VisibleRows; ++i) {
        const int y = 33 + i * 23;
        names[i] = lv_label_create(*this);
        lv_obj_set_pos(names[i], 3, y);
        lv_obj_set_width(names[i], 85);
        lv_label_set_long_mode(names[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_color(names[i], lv_color_white(), 0);
        lv_label_set_text(names[i], "");

        values[i] = lv_label_create(*this);
        lv_obj_set_pos(values[i], 89, y);
        lv_obj_set_style_text_color(values[i], lv_color_white(), 0);
        lv_label_set_text(values[i], "");

        bars[i] = lv_obj_create(*this);
        lv_obj_set_pos(bars[i], 3, y + 15);
        lv_obj_set_size(bars[i], 2, 5);
        lv_obj_set_style_radius(bars[i], 0, 0);
        lv_obj_set_style_border_width(bars[i], 0, 0);
        lv_obj_set_style_pad_all(bars[i], 0, 0);
        lv_obj_set_style_bg_color(bars[i], lv_color_make(32, 220, 85), 0);
        lv_obj_clear_flag(bars[i], LV_OBJ_FLAG_SCROLLABLE);
    }
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
    ready = wifi.begin();
    if(ready) scan();
    else lv_label_set_text(status, "Wi-Fi init failed");
}

void WiFiScannerScreen::onStop() {
    Events::unlisten(&events);
    wifi.stop();
    auto* sleep = (SleepMan*) Services.get(Service::Sleep);
    if(sleep) sleep->enAutoSleep(true);
}

void WiFiScannerScreen::scan() {
    if(!ready || scanning) return;
    scanning = wifi.startScan();
    if(scanning) {
        lastScanUs = esp_timer_get_time();
        lv_label_set_text(status, "Scanning...");
    } else {
        lv_label_set_text(status, "Scan failed");
    }
}

void WiFiScannerScreen::draw() {
    if(aps.empty()) {
        lv_label_set_text(status, "0 networks");
    } else {
        if(selected >= static_cast<int>(aps.size())) selected = static_cast<int>(aps.size()) - 1;
        lv_label_set_text_fmt(status, "%d networks  [%d/%d]", (int)aps.size(),
                              selected + 1, (int)aps.size());
    }

    // Show four consecutive networks, starting with the selected network.
    const int start = (selected / VisibleRows) * VisibleRows;
    for(int i = 0; i < VisibleRows; ++i) {
        const int index = start + i;
        if(index >= static_cast<int>(aps.size())) {
            lv_label_set_text(names[i], "");
            lv_label_set_text(values[i], "");
            lv_obj_add_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const auto& ap = aps[index];
        const auto& name = ap.ssid.empty() ? std::string("<hidden>") : ap.ssid;
        lv_label_set_text_fmt(names[i], "%c%.12s", index == selected ? '>' : ' ', name.c_str());
        lv_label_set_text_fmt(values[i], "%d", (int)ap.rssi);
        lv_obj_set_style_text_color(values[i], signalColour(ap.rssi), 0);
        lv_obj_set_size(bars[i], strength(ap.rssi), 5);
        lv_obj_set_style_bg_color(bars[i], signalColour(ap.rssi), 0);
        lv_obj_clear_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void WiFiScannerScreen::loop() {
    if(scanning && wifi.scanFinished()) {
        scanning = false;
        if(wifi.readResults(aps)) draw();
        else lv_label_set_text(status, "Scan read failed");
    }
    if(ready && !scanning && esp_timer_get_time() - lastScanUs >= ScanIntervalUs) scan();

    for(Event evt{}; events.get(evt, 0); ) {
        bool exit = false;
        if(evt.facility == Facility::Input) {
            auto* input = (Input::Data*) evt.data;
            if(input->action == Input::Data::Press) {
                if(input->btn == Input::Alt) exit = true;
                else if(input->btn == Input::Select) scan();
                else if(!aps.empty()) {
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

#include "BLEScannerScreen.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Services/SleepMan.h"
#include "Util/Services.h"
#include "LV_Interface/InputLVGL.h"
#include "esp_timer.h"
#include <algorithm>
#include <memory>
#include <cstdlib>

namespace {
int strength(int rssi) { return std::max(2, std::min(100, (rssi + 100) * 2)); }
lv_color_t signalColour(int rssi) {
    if(rssi >= -50) return lv_color_make(32, 220, 85);
    if(rssi >= -65) return lv_color_make(245, 220, 35);
    if(rssi >= -75) return lv_color_make(255, 135, 20);
    return lv_color_make(245, 55, 55);
}
}

BLEScannerScreen::BLEScannerScreen() : events(6) {
    lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
    lv_obj_clear_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
    title = lv_label_create(*this);
    lv_obj_set_pos(title, 3, 2);
    lv_obj_set_style_text_color(title, lv_color_make(100, 170, 255), 0);
    lv_label_set_text(title, "BLE SCANNER");
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
        bars[i] = lv_bar_create(*this);
        lv_obj_set_pos(bars[i], 3, y + 14);
        lv_obj_set_size(bars[i], 120, 7);
        lv_bar_set_range(bars[i], 0, 100);
        lv_bar_set_value(bars[i], 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_opa(bars[i], LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bars[i], lv_color_make(32, 43, 55), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bars[i], LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bars[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(bars[i], 0, LV_PART_INDICATOR);
        lv_obj_set_style_border_width(bars[i], 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(bars[i], 0, LV_PART_MAIN);
    }
}

BLEScannerScreen::~BLEScannerScreen() { Events::unlisten(&events); ble.stop(); }

void BLEScannerScreen::onStart() {
    if(auto* sleep = (SleepMan*) Services.get(Service::Sleep)) sleep->enAutoSleep(false);
    Events::listen(Facility::Input, &events);
    lv_indev_wait_release(InputLVGL::getInstance()->getIndev());
    lv_label_set_text(status, ble.start() ? "Scanning..." : "BLE scan failed");
    lastUpdateUs = 0;
}

void BLEScannerScreen::onStop() {
    Events::unlisten(&events);
    ble.stop();
    if(auto* sleep = (SleepMan*) Services.get(Service::Sleep)) sleep->enAutoSleep(true);
}

void BLEScannerScreen::draw() {
    if(selected >= (int)devices.size()) selected = devices.empty() ? 0 : (int)devices.size() - 1;
    lv_label_set_text_fmt(status, "%d devices [%d/%d]", (int)devices.size(),
                          devices.empty() ? 0 : selected + 1, (int)devices.size());
    const int start = (selected / VisibleRows) * VisibleRows;
    for(int i = 0; i < VisibleRows; ++i) {
        int index = start + i;
        if(index >= (int)devices.size()) {
            lv_label_set_text(names[i], "");
            lv_label_set_text(values[i], "");
            lv_obj_add_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const auto& d = devices[index];
        lv_label_set_text_fmt(names[i], "%c%.12s", index == selected ? '>' : ' ', d.name.c_str());
        lv_label_set_text_fmt(values[i], "%d", d.rssi);
        lv_obj_set_style_text_color(values[i], signalColour(d.rssi), 0);
        lv_bar_set_value(bars[i], strength(d.rssi), LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bars[i], signalColour(d.rssi), LV_PART_INDICATOR);
        lv_obj_clear_flag(bars[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void BLEScannerScreen::loop() {
    uint64_t now = esp_timer_get_time();
    if(now - lastUpdateUs >= 2000000) {
        lastUpdateUs = now;
        ble.snapshot(devices);
        draw();
    }
    for(Event evt{}; events.get(evt, 0); ) {
        bool exit = false;
        if(evt.facility == Facility::Input) {
            auto* input = (Input::Data*) evt.data;
            if(input->action == Input::Data::Press) {
                if(input->btn == Input::Alt) exit = true;
                else if(input->btn == Input::Up && selected > 0) { --selected; draw(); }
                else if(input->btn == Input::Down && selected + 1 < (int)devices.size()) { ++selected; draw(); }
                else if(input->btn == Input::Select) { ble.stop(); ble.start(); selected = 0; }
            }
        }
        free(evt.data);
        if(exit) { transition([](){ return std::make_unique<MainMenu>(); }); return; }
    }
}

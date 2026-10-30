#include "HackerModeScreen.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Services/SleepMan.h"
#include "Util/Services.h"
#include "LV_Interface/InputLVGL.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_timer.h"
#include <cstdlib>
#include <memory>

namespace {
constexpr char Glyphs[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ@#$%";
}

HackerModeScreen::HackerModeScreen() : events(8) {
    lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
    lv_obj_clear_flag(*this, LV_OBJ_FLAG_SCROLLABLE);
    for(size_t i = 0; i < columns.size(); ++i) {
        columns[i] = lv_label_create(*this);
        lv_obj_set_pos(columns[i], static_cast<lv_coord_t>(i * 10), 0);
        lv_obj_set_style_text_color(columns[i], lv_color_make(0, 200, 55), 0);
        lv_label_set_text(columns[i], "");
        offsets[i] = static_cast<uint8_t>(esp_random() % 10);
    }
}

HackerModeScreen::~HackerModeScreen() {
    Events::unlisten(&events);
}

void HackerModeScreen::onStart() {
    if(auto* sleep = (SleepMan*)Services.get(Service::Sleep)) sleep->enAutoSleep(false);
    Events::listen(Facility::Input, &events);
    lv_indev_wait_release(InputLVGL::getInstance()->getIndev());
    lastFrameUs = 0;
    render();
}

void HackerModeScreen::onStop() {
    Events::unlisten(&events);
    if(auto* sleep = (SleepMan*)Services.get(Service::Sleep)) sleep->enAutoSleep(true);
}

void HackerModeScreen::render() {
    const auto colour = amber ? lv_color_make(255, 150, 0) : lv_color_make(0, 220, 70);
    for(size_t i = 0; i < columns.size(); ++i) {
        char text[40]{};
        size_t p = 0;
        for(int row = 0; row < 13; ++row) {
            const int distance = (static_cast<int>(offsets[i]) - row + 18) % 18;
            if(distance < 8) {
                text[p++] = Glyphs[esp_random() % (sizeof(Glyphs) - 1)];
            } else {
                text[p++] = ' ';
            }
            if(row != 12) text[p++] = '\n';
        }
        text[p] = '\0';
        lv_label_set_text(columns[i], text);
        lv_obj_set_style_text_color(columns[i], colour, 0);
        // Each column advances independently for a cascading effect.
        if((esp_random() % 4) != 0) offsets[i] = (offsets[i] + 1) % 18;
    }
}

void HackerModeScreen::loop() {
    const uint64_t now = esp_timer_get_time();
    if(now - lastFrameUs >= 140000) {
        lastFrameUs = now;
        render();
    }
    for(Event evt{}; events.get(evt, 0); ) {
        bool exit = false;
        if(evt.facility == Facility::Input) {
            const auto* input = (Input::Data*)evt.data;
            if(input->action == Input::Data::Press) {
                if(input->btn == Input::Alt) exit = true;
                else if(input->btn == Input::Select) amber = !amber;
            }
        }
        free(evt.data);
        if(exit) {
            transition([](){ return std::make_unique<MainMenu>(); });
            return;
        }
    }
}

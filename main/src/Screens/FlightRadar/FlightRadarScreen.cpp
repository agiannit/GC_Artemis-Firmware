#include "FlightRadarScreen.h"
#include "Screens/MainMenu/MainMenu.h"
#include "LV_Interface/InputLVGL.h"
#include "Services/SleepMan.h"
#include "Util/Services.h"
#include <memory>
#include <cstdlib>

FlightRadarScreen::FlightRadarScreen() : events(6) {
    lv_obj_set_style_bg_color(*this, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
    lv_obj_clear_flag(*this, LV_OBJ_FLAG_SCROLLABLE);

    auto* title = lv_label_create(*this);
    lv_obj_set_pos(title, 3, 3);
    lv_obj_set_style_text_color(title, lv_color_make(70, 210, 255), 0);
    lv_label_set_text(title, "FLIGHT RADAR");

    status = lv_label_create(*this);
    lv_obj_set_pos(status, 4, 27);
    lv_obj_set_width(status, 120);
    lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(status, lv_color_white(), 0);
    lv_label_set_text(status, "Aircraft lookup\ncoming soon.\n\nALT: Back");
}

FlightRadarScreen::~FlightRadarScreen() {
    Events::unlisten(&events);
}

void FlightRadarScreen::onStart() {
    Events::listen(Facility::Input, &events);
    lv_indev_wait_release(InputLVGL::getInstance()->getIndev());
}

void FlightRadarScreen::onStop() {
    Events::unlisten(&events);
}

void FlightRadarScreen::loop() {
    for(Event evt{}; events.get(evt, 0); ) {
        bool exit = false;
        if(evt.facility == Facility::Input) {
            auto* input = static_cast<Input::Data*>(evt.data);
            exit = input->action == Input::Data::Press && input->btn == Input::Alt;
        }
        free(evt.data);
        if(exit) {
            transition([](){ return std::make_unique<MainMenu>(); });
            return;
        }
    }
}

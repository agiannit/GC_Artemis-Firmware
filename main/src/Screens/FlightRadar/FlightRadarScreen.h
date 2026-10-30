#pragma once
#include "LV_Interface/LVScreen.h"
#include "Devices/Input.h"
#include "Util/Events.h"

class FlightRadarScreen : public LVScreen {
public:
    FlightRadarScreen();
    ~FlightRadarScreen() override;
private:
    void onStart() override;
    void onStop() override;
    void loop() override;
    EventQueue events;
    lv_obj_t* status = nullptr;
};

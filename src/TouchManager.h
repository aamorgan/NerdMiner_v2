#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SPIFFS.h>

typedef void (*TouchCallback)();

#define TOUCH_DEBOUNCE 200

class TouchManager {
public:
    explicit TouchManager(TFT_eSPI &tft);

    bool begin(bool forceCalibration = false);
    bool getTouch(uint16_t *x, uint16_t *y);
    bool isTouched();

    bool calibrate();
    bool loadCalibration();
    bool saveCalibration(uint16_t *calData);

    bool isButtonPressed(uint16_t x, uint16_t y, uint16_t buttonX, uint16_t buttonY,
                         uint16_t buttonW, uint16_t buttonH);
    bool isAreaPressed(uint16_t x, uint16_t y, uint16_t areaX, uint16_t areaY,
                       uint16_t areaW, uint16_t areaH);

    void setScreenSwitchCallback(TouchCallback callback);
    void setScreenSwitchAltCallback(TouchCallback callback);

private:
    TouchCallback _screenSwitchCallback = nullptr;
    TouchCallback _screenSwitchAltCallback = nullptr;
    TFT_eSPI &_tft;
    const char *CALIBRATION_FILE = "/touchcal.dat";
    uint16_t _calData[5];
    bool _calibrated;
    bool _spiffsMounted;
    unsigned long _lastTouch;
};

#endif

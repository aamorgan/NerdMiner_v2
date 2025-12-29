#include "TouchManager.h"

TouchManager::TouchManager(TFT_eSPI &tft)
    : _tft(tft), _calibrated(false), _spiffsMounted(false), _lastTouch(0)
{
    for (int i = 0; i < 5; i++)
    {
        _calData[i] = 0;
    }
}

bool TouchManager::begin(bool forceCalibration)
{
    Serial.printf("[TouchManager] begin() called, forceCalibration=%d\n", forceCalibration);
    
    if (!_spiffsMounted)
    {
        if (!SPIFFS.begin(true))
        {
            Serial.println(F("[TouchManager] SPIFFS Mount Failed"));
            return false;
        }
        _spiffsMounted = true;
        Serial.println(F("[TouchManager] SPIFFS Mounted Successfully"));
    }

    if (forceCalibration || !loadCalibration())
    {
        Serial.println(F("[TouchManager] Starting calibration..."));
        return calibrate();
    }

    Serial.printf("[TouchManager] Calibration loaded, _calibrated=%d\n", _calibrated);
    return true;
}

bool TouchManager::getTouch(uint16_t *x, uint16_t *y)
{
    static unsigned long checkCount = 0;
    static bool loggedCalibrationStatus = false;
    
    if (!_calibrated)
    {
        if (!loggedCalibrationStatus)
        {
            Serial.println(F("[TouchManager] getTouch() called but NOT CALIBRATED!"));
            loggedCalibrationStatus = true;
        }
        return false;
    }
    
    bool result = _tft.getTouch(x, y);
    checkCount++;
    
    if (checkCount % 100 == 0)
    {
        Serial.printf("[TouchManager] Touch check #%lu, result=%d\n", checkCount, result);
    }
    
    return result;
}

bool TouchManager::calibrate()
{
    Serial.println(F("[TouchManager] calibrate() starting..."));
    _tft.fillScreen(TFT_BLACK);
    _tft.setCursor(20, 0);
    _tft.setTextFont(2);
    _tft.setTextSize(1);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);

    _tft.println(F("Touch corners as indicated"));
    _tft.setTextFont(1);
    _tft.println();

    Serial.println(F("[TouchManager] Calling TFT_eSPI.calibrateTouch()..."));
    _tft.calibrateTouch(_calData, TFT_MAGENTA, TFT_BLACK, 15);

    Serial.print(F("[TouchManager] Calibration data: "));
    for (int i = 0; i < 5; i++)
    {
        Serial.printf("%d ", _calData[i]);
    }
    Serial.println();

    _tft.setTextColor(TFT_GREEN, TFT_BLACK);
    _tft.println(F("Calibration complete!"));

    if (saveCalibration(_calData))
    {
        _calibrated = true;
        _tft.setTouch(_calData);
        Serial.println(F("[TouchManager] Calibration saved and applied successfully"));
        return true;
    }

    Serial.println(F("[TouchManager] ERROR: Failed to save calibration"));
    return false;
}

bool TouchManager::loadCalibration()
{
    Serial.println(F("[TouchManager] loadCalibration() called"));
    
    if (SPIFFS.exists(CALIBRATION_FILE))
    {
        Serial.printf("[TouchManager] Found calibration file: %s\n", CALIBRATION_FILE);
        fs::File f = SPIFFS.open(CALIBRATION_FILE, "r");
        if (f)
        {
            if (f.readBytes(reinterpret_cast<char *>(_calData), sizeof(_calData)) == sizeof(_calData))
            {
                f.close();
                
                Serial.print(F("[TouchManager] Loaded calibration data: "));
                for (int i = 0; i < 5; i++)
                {
                    Serial.printf("%d ", _calData[i]);
                }
                Serial.println();
                
                _tft.setTouch(_calData);
                _calibrated = true;
                Serial.println(F("[TouchManager] Calibration applied successfully"));
                return true;
            }
            f.close();
            Serial.println(F("[TouchManager] ERROR: Failed to read calibration data"));
        }
        else
        {
            Serial.println(F("[TouchManager] ERROR: Failed to open calibration file"));
        }
    }
    else
    {
        Serial.printf("[TouchManager] Calibration file not found: %s\n", CALIBRATION_FILE);
    }
    
    return false;
}

bool TouchManager::saveCalibration(uint16_t *calData)
{
    fs::File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
        f.write(reinterpret_cast<const uint8_t *>(calData), sizeof(_calData));
        f.close();
        return true;
    }
    return false;
}

bool TouchManager::isButtonPressed(uint16_t x, uint16_t y, uint16_t buttonX, uint16_t buttonY,
                                   uint16_t buttonW, uint16_t buttonH)
{
    return (x >= buttonX && x <= (buttonX + buttonW) &&
            y >= buttonY && y <= (buttonY + buttonH));
}

bool TouchManager::isAreaPressed(uint16_t x, uint16_t y, uint16_t areaX, uint16_t areaY,
                                 uint16_t areaW, uint16_t areaH)
{
    return (x >= areaX && x <= (areaX + areaW) &&
            y >= areaY && y <= (areaY + areaH));
}

bool TouchManager::isTouched()
{
    uint16_t x, y;
    bool pressed = getTouch(&x, &y);
    if (pressed)
    {
        if (millis() - _lastTouch > TOUCH_DEBOUNCE)
        {
            _lastTouch = millis();

            if (_screenSwitchAltCallback && isAreaPressed(x, y, 109, 185, 102, 56))
            {
                _screenSwitchAltCallback();
                return true;
            }

            if (_screenSwitchCallback)
            {
                _screenSwitchCallback();
            }

            return true;
        }
    }

    return false;
}

void TouchManager::setScreenSwitchCallback(TouchCallback callback)
{
    _screenSwitchCallback = callback;
}

void TouchManager::setScreenSwitchAltCallback(TouchCallback callback)
{
    _screenSwitchAltCallback = callback;
}

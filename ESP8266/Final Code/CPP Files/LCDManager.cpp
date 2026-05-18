#include "LCDManager.h"

LCDManager::LCDManager() : lcd(0x27, 20, 4) {}

void LCDManager::begin() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("USB Power Controller");
    delay(1500);
    lcd.clear();
}

void LCDManager::update(float v1, float v2, float v3, float v4,
                        float c1, float c2, float c3, float c4,
                        float vSupply,
                        String usbState[4],
                        bool scheduleEnabled[4],
                        unsigned long schedStart[4],
                        unsigned long schedEnd[4],
                        int schedTarget[4],
                        unsigned long epoch,
                        bool schedAck) 
{
    lcd.clear();

    float volts[4] = { v1, v2, v3, v4 };
    float currs[4] = { c1, c2, c3, c4 };

    // Row 0: sw1 + VS:
    lcd.setCursor(0, 0);
    lcd.print("sw1: ");
    lcd.print(String(volts[0], 1));
    lcd.print("V ");
    lcd.print(String(currs[0] * 1000.0f, 0));
    lcd.print("mA ");
    lcd.print(usbState[0] == "ON" ? "1" : "0");

    lcd.setCursor(17, 0);
    lcd.print("VS:");

    // Row 1: sw2 + supply voltage
    lcd.setCursor(0, 1);
    lcd.print("sw2: ");
    lcd.print(String(volts[1], 1));
    lcd.print("V ");
    lcd.print(String(currs[1] * 1000.0f, 0));
    lcd.print("mA ");
    lcd.print(usbState[1] == "ON" ? "1" : "0");

    lcd.setCursor(17, 1);
    lcd.print(String(vSupply, 1));

    // Row 2: sw3 + ACK under VS
    lcd.setCursor(0, 2);
    lcd.print("sw3: ");
    lcd.print(String(volts[2], 1));
    lcd.print("V ");
    lcd.print(String(currs[2] * 1000.0f, 0));
    lcd.print("mA ");
    lcd.print(usbState[2] == "ON" ? "1" : "0");

    lcd.setCursor(17, 2);
    lcd.print(schedAck ? "1" : "0");

    // Row 3: sw4
    lcd.setCursor(0, 3);
    lcd.print("sw4: ");
    lcd.print(String(volts[3], 1));
    lcd.print("V ");
    lcd.print(String(currs[3] * 1000.0f, 0));
    lcd.print("mA ");
    lcd.print(usbState[3] == "ON" ? "1" : "0");
}

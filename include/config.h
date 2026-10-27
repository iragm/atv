#pragma once
#include <Arduino.h>
namespace config {
constexpr uint8_t THROTTLE_PIN = D0;     // GPIO16, external 10k to 3.3V
constexpr uint8_t I2C_SDA_PIN = D2;
constexpr uint8_t I2C_SCL_PIN = D1;
constexpr uint8_t ONEWIRE_PIN = D4;
constexpr uint8_t RPWM_PIN = D5;
constexpr uint8_t LPWM_PIN = D6;
constexpr uint8_t CHARGER_PIN = D7;
constexpr uint8_t ENABLE_PIN = D8;
constexpr uint8_t DIRECTION_ADC_PIN = A0;
constexpr uint16_t PWM_RANGE = 1023;
constexpr uint16_t PWM_FREQUENCY_HZ = 1000;
constexpr uint16_t FORWARD_MIN = 45;
constexpr uint16_t FORWARD_MAX = 145;
constexpr uint16_t REVERSE_MIN = 275;
constexpr uint16_t REVERSE_MAX = 410;
constexpr uint8_t INA226_ADDRESS = 0x40;
constexpr float SHUNT_OHMS = 0.0015f;
#define OLED_IS_SH1106 0
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint32_t INPUT_PERIOD_MS = 10;
constexpr uint32_t DISPLAY_PERIOD_MS = 250;
constexpr uint32_t INA_PERIOD_MS = 25; // sampled current-protection response, not hardware-instantaneous
constexpr uint32_t TEMPERATURE_PERIOD_MS = 1000;
constexpr uint32_t LOG_PERIOD_MS = 1000;
constexpr uint32_t PRESS_DEBOUNCE_MS = 30;
constexpr uint32_t WIFI_DRIVE_TTL_MS = 600;
// Choose a unique WPA2 passphrase before putting the ATV in service.
constexpr char AP_PASSWORD[] = "quadsetup26";
}

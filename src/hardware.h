#pragma once

#ifdef BUDDY_STICKC_PLUS2
#include <M5Unified.h>
// Keep the shared renderer API independent of the selected graphics library.
using TFT_eSPI = lgfx::LGFXBase;
using TFT_eSprite = M5Canvas;
using lgfx::MC_DATUM;
using lgfx::TL_DATUM;
struct RTC_TimeTypeDef { uint8_t Hours, Minutes, Seconds; };
struct RTC_DateTypeDef { uint8_t WeekDay, Month, Date; uint16_t Year; };
constexpr int LED_PIN = 19;
constexpr int LED_ON = HIGH;
constexpr int LED_OFF = LOW;
constexpr const char* HARDWARE_NAME = "M5StickC Plus2";
#else
#include <M5StickCPlus.h>
constexpr int LED_PIN = 10;
constexpr int LED_ON = LOW;
constexpr int LED_OFF = HIGH;
constexpr const char* HARDWARE_NAME = "M5StickC Plus";
#endif

inline void hardwareBegin() {
  Serial.begin(115200);
  Serial.printf("boot: %s hardware init\n", HARDWARE_NAME);
#ifdef BUDDY_STICKC_PLUS2
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.fallback_board = m5::board_t::board_M5StickCPlus2;
  M5.begin(cfg);
  if (M5.getBoard() != m5::board_t::board_M5StickCPlus2) {
    Serial.println("ERROR: selected Plus2 target does not match detected hardware");
    while (true) delay(1000);
  }
  if (!M5.Imu.isEnabled()) {
    Serial.println("ERROR: IMU initialization failed");
    while (true) delay(1000);
  }
  if (!M5.Rtc.isEnabled()) Serial.println("ERROR: RTC initialization failed");
  Serial.println("boot: USB/current sensing unavailable; automatic USB clock disabled");
  M5.Speaker.setVolume(64);
#else
  M5.begin();
  M5.Imu.Init();
  M5.Beep.begin();
#endif
  Serial.println("boot: hardware ready");
}

inline void hardwareUpdate() {
  M5.update();
#ifdef BUDDY_STICKC_PLUS2
  if (M5.BtnPWR.pressedFor(6000)) M5.Power.powerOff();
#else
  M5.Beep.update();
#endif
}

inline void hardwareTone(uint16_t freq, uint16_t duration) {
#ifdef BUDDY_STICKC_PLUS2
  M5.Speaker.tone(freq, duration);
#else
  M5.Beep.tone(freq, duration);
#endif
}

inline void hardwareBrightness(uint8_t percent) {
#ifdef BUDDY_STICKC_PLUS2
  M5.Display.setBrightness(percent * 255 / 100);
#else
  M5.Axp.ScreenBreath(percent);
#endif
}

inline void hardwareScreen(bool on) {
#ifdef BUDDY_STICKC_PLUS2
  if (on) M5.Display.wakeup();
  else {
    M5.Display.setBrightness(0);
    M5.Display.sleep();
  }
#else
  M5.Axp.SetLDO2(on);
#endif
}

inline void hardwarePowerOff() {
#ifdef BUDDY_STICKC_PLUS2
  M5.Power.powerOff();
#else
  M5.Axp.PowerOff();
#endif
}

inline bool hardwarePowerClicked() {
#ifdef BUDDY_STICKC_PLUS2
  return M5.BtnPWR.wasClicked();
#else
  return M5.Axp.GetBtnPress() == 0x02;
#endif
}

inline int hardwareBatteryMV() {
#ifdef BUDDY_STICKC_PLUS2
  return M5.Power.getBatteryVoltage();
#else
  return M5.Axp.GetBatVoltage() * 1000;
#endif
}

enum class HardwareUsb { Unknown, Disconnected, Connected };

inline HardwareUsb hardwareUsb() {
#ifdef BUDDY_STICKC_PLUS2
  // Plus2 has no VBUS monitor. GPIO35 is the power key, not USB detect.
  return HardwareUsb::Unknown;
#else
  return M5.Axp.GetVBusVoltage() > 4.0f ? HardwareUsb::Connected : HardwareUsb::Disconnected;
#endif
}

inline bool hardwareRtcRead(RTC_TimeTypeDef& time, RTC_DateTypeDef& date) {
#ifdef BUDDY_STICKC_PLUS2
  m5::rtc_datetime_t dt;
  if (!M5.Rtc.getDateTime(&dt)) {
    Serial.println("ERROR: RTC read failed");
    return false;
  }
  time = {uint8_t(dt.time.hours), uint8_t(dt.time.minutes), uint8_t(dt.time.seconds)};
  date = {uint8_t(dt.date.weekDay), uint8_t(dt.date.month), uint8_t(dt.date.date), uint16_t(dt.date.year)};
#else
  M5.Rtc.GetTime(&time);
  M5.Rtc.GetDate(&date);
#endif
  return true;
}

inline bool hardwareRtcSet(const tm& local) {
#ifdef BUDDY_STICKC_PLUS2
  if (!M5.Rtc.isEnabled()) {
    Serial.println("ERROR: cannot sync unavailable RTC");
    return false;
  }
  M5.Rtc.setDateTime(&local);
  m5::rtc_datetime_t verified;
  if (!M5.Rtc.getDateTime(&verified)) {
    Serial.println("ERROR: RTC sync readback failed");
    return false;
  }
#else
  RTC_TimeTypeDef time = {uint8_t(local.tm_hour), uint8_t(local.tm_min), uint8_t(local.tm_sec)};
  RTC_DateTypeDef date = {uint8_t(local.tm_wday), uint8_t(local.tm_mon + 1),
                         uint8_t(local.tm_mday), uint16_t(local.tm_year + 1900)};
  M5.Rtc.SetTime(&time);
  M5.Rtc.SetDate(&date);
#endif
  return true;
}

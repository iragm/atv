#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#include <Updater.h>
#include <time.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <math.h>
#include <string.h>
#include "config.h"
#include "control.h"
#include "energy.h"
#include "settings.h"
#include "web_ui.h"
#include "voltage_guard.h"
#include "ota_roots.h"

#if OLED_IS_SH1106
static U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
#else
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
#endif
static ESP8266WebServer web(80);
static DNSServer dns;
static OneWire oneWire(config::ONEWIRE_PIN);
static DallasTemperature thermometers(&oneWire);
static quad::Settings settings;
static quad::Controller controller;
static quad::Output output;
static DeviceAddress probeRom[2] = {{0}, {0}};
static float probeC[2] = {NAN, NAN};
static uint8_t probeCount = 0;
static bool temperaturePending = false;
static bool oledAvailable = false;
static bool inaAvailable = false;
static quad::LoadProtection loadProtection;
static quad::EnergyTracker energy;
static quad::VoltageGuard voltageGuard;
static bool otaBusy = false, otaRequested = false, otaUploadOk = false;
static bool rebootQueued = false;
static uint32_t rebootAt = 0;
static String otaResult = "Idle";
static const char OTA_URL[] = "https://github.com/iragm/atv/releases/download/firmware-latest/quad-controller-d1-mini.bin";
static float busVolts = NAN;
static float currentDerateV = 12.0f;
static float motorAmps = NAN;
static bool motorHot = false;
static bool gearboxHot = false;
static float motorTempC = NAN;
static float gearboxTempC = NAN;
static bool motorFound = false;
static bool gearboxFound = false;
static bool chargerPresent = false;
static bool priorRawPress = false;
static uint32_t throttlePressedSince = 0;
static bool remoteHeld = false;
static uint32_t lastRemoteMs = 0;
static quad::Direction remoteDirection = quad::Direction::Invalid;
static quad::Direction physicalDirection = quad::Direction::Invalid;
static quad::Direction appliedDirection = quad::Direction::Invalid;
static uint16_t adc = 0;
static uint32_t lastControl = 0, lastIna = 0, lastDisplay = 0, lastTempRequest = 0, lastLog = 0;

static const char* directionName(quad::Direction d) {
  switch(d) { case quad::Direction::Forward: return "Forward"; case quad::Direction::Reverse: return "Reverse"; default: return "Invalid"; }
}
static const char* stateName(quad::State s) {
  switch(s) { case quad::State::Ready:return "Ready"; case quad::State::Ramp:return "Driving";
    case quad::State::ReleaseHold:return "Release hold"; case quad::State::WaitRelease:return "Release throttle";
    case quad::State::Charging:return "Charging"; default:return "Fault"; }
}
static const char* faultName(quad::Fault f) {
  switch(f) { case quad::Fault::Direction:return "Direction"; case quad::Fault::Sensor:return "Sensor";
    case quad::Fault::Overcurrent:return "Sustained stall current"; case quad::Fault::EmergencyCurrent:return "Emergency overcurrent"; case quad::Fault::NoLoad:return "Possible no-load"; default:return "None"; }
}
static const char* sourceName(quad::Source s) {
  switch(s) { case quad::Source::Pedal:return "Pedal"; case quad::Source::Wifi:return "Wi-Fi"; default:return "None"; }
}
static bool readInaRegister(uint8_t reg, uint16_t& out) {
  Wire.beginTransmission(config::INA226_ADDRESS);
  Wire.write(reg);
  if(Wire.endTransmission(false)!=0) return false;
  if(Wire.requestFrom(config::INA226_ADDRESS,static_cast<uint8_t>(2))!=2) return false;
  out=static_cast<uint16_t>((Wire.read()<<8)|Wire.read());
  return true;
}
static bool writeInaRegister(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(config::INA226_ADDRESS); Wire.write(reg);
  Wire.write(static_cast<uint8_t>(val>>8)); Wire.write(static_cast<uint8_t>(val));
  return Wire.endTransmission()==0;
}
static bool pollIna() {
  uint16_t shunt=0,bus=0;
  if(!readInaRegister(0x01,shunt)||!readInaRegister(0x02,bus)) { busVolts=NAN;motorAmps=NAN;return false; }
  motorAmps=static_cast<int16_t>(shunt)*2.5e-6f/config::SHUNT_OHMS;
  busVolts=bus*0.00125f;
  return true;
}
static void disableDrive() {
  digitalWrite(config::ENABLE_PIN,LOW);
  analogWrite(config::RPWM_PIN,0);
  analogWrite(config::LPWM_PIN,0);
  appliedDirection=quad::Direction::Invalid;
}
static void applyDrive(const quad::Output& command) {
  if(!command.enabled||!command.duty||command.direction==quad::Direction::Invalid) { disableDrive();return; }
  if(appliedDirection!=command.direction) { disableDrive();appliedDirection=command.direction; }
  if(command.direction==quad::Direction::Forward) {
    analogWrite(config::LPWM_PIN,0);analogWrite(config::RPWM_PIN,command.duty);
  } else {
    analogWrite(config::RPWM_PIN,0);analogWrite(config::LPWM_PIN,command.duty);
  }
  digitalWrite(config::ENABLE_PIN,HIGH);
}
static uint16_t readDirectionAdc() {
  uint32_t sum=0;for(int i=0;i<4;i++)sum+=analogRead(config::DIRECTION_ADC_PIN);
  return static_cast<uint16_t>(sum/4);
}
static String romHex(const uint8_t rom[8]) {
  char buf[17];snprintf(buf,sizeof(buf),"%02X%02X%02X%02X%02X%02X%02X%02X",rom[0],rom[1],rom[2],rom[3],rom[4],rom[5],rom[6],rom[7]);return String(buf);
}
static uint8_t hexDigit(char c) {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return 255;
}
static bool parseRom(const String& hex, uint8_t rom[8]) {
  if(hex.isEmpty()) { memset(rom,0,8);return true; }
  if(hex.length()!=16)return false;
  for(int i=0;i<8;i++) { auto a=hexDigit(hex[2*i]),b=hexDigit(hex[2*i+1]);
    if(a>15||b>15)return false;
    rom[i]=(a<<4)|b;
  }
  return true;
}
static int probeIndex(const uint8_t rom[8]) {
  for(uint8_t i=0;i<probeCount;i++)if(memcmp(probeRom[i],rom,8)==0)return i;
  return -1;
}
static const uint8_t* effectiveRom(const uint8_t preferred[8], uint8_t fallbackIndex) {
  if(!quad::emptyRom(preferred))return preferred;
  return fallbackIndex<probeCount?probeRom[fallbackIndex]:nullptr;
}
static float selectedTemp(const uint8_t preferred[8], uint8_t fallbackIndex, bool& present) {
  const uint8_t* rom=effectiveRom(preferred,fallbackIndex);
  const int index=rom?probeIndex(rom):-1;
  present=index>=0 && isfinite(probeC[index]);
  return present?probeC[index]:NAN;
}
static void refreshThermal() {
  motorTempC=selectedTemp(settings.motorRom,0,motorFound);
  gearboxTempC=selectedTemp(settings.gearboxRom,1,gearboxFound);
  // 2C hysteresis avoids toggling the limit at the threshold.
  motorHot=motorFound && motorTempC >= settings.motorCutoffC-(motorHot?2.0f:0.0f);
  gearboxHot=gearboxFound && gearboxTempC >= settings.gearboxCutoffC-(gearboxHot?2.0f:0.0f);
}
static void scanProbes() {
  probeCount=0;
  const int found=thermometers.getDeviceCount();
  for(int i=0;i<found&&probeCount<2;i++) {
    DeviceAddress addr;
    if(thermometers.getAddress(addr,i)) {
      memcpy(probeRom[probeCount],addr,8);
      thermometers.setResolution(addr,12);
      probeCount++;
    }
  }
  Serial.printf("DS18B20 probes: %u\n",probeCount);
  for(int i=0;i<probeCount;i++)Serial.printf("probe%d=%s\n",i+1,romHex(probeRom[i]).c_str());
}
static void pollTemperatures(uint32_t now) {
  if(probeCount==0)return;
  if(!temperaturePending) {
    if(now-lastTempRequest>=config::TEMPERATURE_PERIOD_MS) {
      thermometers.requestTemperatures(); // asynchronous
      temperaturePending=true;lastTempRequest=now;
    }
    return;
  }
  if(static_cast<uint32_t>(now-lastTempRequest)<800) return; // 12-bit conversion maximum 750ms
  for(uint8_t i=0;i<probeCount;i++) {
    const float c=thermometers.getTempC(probeRom[i]);
    probeC[i]=(c==DEVICE_DISCONNECTED_C||!isfinite(c)||c< -55||c>125)?NAN:c;
  }
  temperaturePending=false;
  refreshThermal();
}
static float supplyForPwm() {return (inaAvailable&&isfinite(busVolts)&&busVolts>=5&&busVolts<=17)?busVolts:12.0f;}
static quad::Limits controllerLimits() {
  quad::Limits l;l.rampMs=settings.rampMs;l.graceMs=2000;l.maxForwardV=settings.forwardV;l.maxReverseV=settings.reverseV;return l;
}
static void loadSettings() {
  EEPROM.begin(512);
  EEPROM.get(0,settings);
  if(!quad::validSettings(settings)) {
    quad::Settings upgraded;
    quad::LegacySettingsV4 old4{};
    quad::LegacySettingsV3 old3{};
    quad::LegacySettingsV2 old2{};
    EEPROM.get(0,old4);
    if (quad::upgradeSettings(old4,upgraded)) {
      settings=upgraded;Serial.println(F("Migrated v4 settings to v5"));
    } else {
    EEPROM.get(0,old3);
    if(quad::upgradeSettings(old3,upgraded)) {
      settings=upgraded;Serial.println(F("Migrated v3 settings to v5"));
    } else {
      EEPROM.get(0,old2);
      if(quad::upgradeSettings(old2,upgraded)) {
        settings=upgraded;Serial.println(F("Migrated v2 settings to v5"));
      } else {
        settings=quad::Settings{};settings.checksum=quad::settingsChecksum(settings);
        Serial.println(F("Default settings loaded"));
      }
    }
    }
  }
  controller.setLimits(controllerLimits());
}
static String jsonFloat(float f, uint8_t digits=2) {return isfinite(f)?String(f,digits):String("null");}
static String probeListJson() {
  String s="[";
  for(uint8_t i=0;i<probeCount;i++) { if(i)s+=',';s+="{\"rom\":\""+romHex(probeRom[i])+"\",\"temp_c\":"+jsonFloat(probeC[i])+"}"; }
  s+="]";return s;
}
static String escapeJson(const String& v) {
  String result;result.reserve(v.length()+8);
  for(unsigned i=0;i<v.length();i++) {
    const char c=v[i];
    if(c=='"'||c=='\\'){result+='\\';result+=c;}
    else if(static_cast<uint8_t>(c)<32) {result+=' ';}
    else result+=c;
  }
  return result;
}
static String apName() {
  if(settings.apSsid[0]) return String(settings.apSsid);
  String ssid="QUAD-"+String(ESP.getChipId(),HEX);ssid.toUpperCase();return ssid;
}
static String settingsJson() {
  String s;s.reserve(830);
  s="{\"ramp_s\":"+String(settings.rampMs/1000.0f,1)+",\"forward_v\":"+jsonFloat(settings.forwardV,1)+
    ",\"reverse_v\":"+jsonFloat(settings.reverseV,1)+",\"motor_cutoff_c\":"+jsonFloat(settings.motorCutoffC,0)+
    ",\"gearbox_cutoff_c\":"+jsonFloat(settings.gearboxCutoffC,0)+
    ",\"emergency_trip_a\":"+jsonFloat(settings.emergencyTripA,1)+
    ",\"derate_trip_a\":"+jsonFloat(settings.derateTripA,1)+
    ",\"derate_trip_ms\":"+String(settings.derateTripMs)+
    ",\"derate_minimum_v\":"+jsonFloat(settings.derateMinimumV,1)+
    ",\"stall_trip_a\":"+jsonFloat(settings.stallTripA,1)+
    ",\"stall_trip_ms\":"+String(settings.stallTripMs)+
    ",\"no_load_below_a\":"+jsonFloat(settings.noLoadBelowA,1)+
    ",\"no_load_trip_ms\":"+String(settings.noLoadTripMs)+
    ",\"battery_capacity_ah\":"+jsonFloat(settings.batteryCapacityAh,1)+
    ",\"charger_cutoff_v\":"+jsonFloat(settings.chargerCutoffV,1)+
    ",\"ap_ssid\":\""+escapeJson(apName())+"\""+
    ",\"ota_ssid\":\""+escapeJson(String(settings.otaSsid))+"\""+
    ",\"ota_url\":\""+String(OTA_URL)+"\""+
    ",\"motor_sensor\":\"";
  const uint8_t* m=effectiveRom(settings.motorRom,0);if(m)s+=romHex(m);
  s+="\",\"gearbox_sensor\":\"";
  const uint8_t* g=effectiveRom(settings.gearboxRom,1);if(g)s+=romHex(g);
  s+="\",\"probes\":"+probeListJson()+"}";
  return s;
}
static String statusJson() {
  const auto recent=energy.recent(millis());
  String s;s.reserve(1280);
  s="{\"state\":\""+String(stateName(output.state))+"\",\"fault\":\""+faultName(output.fault)+
    "\",\"direction\":\""+directionName(output.direction)+"\",\"source\":\""+sourceName(output.source)+
    "\",\"duty\":"+String(output.duty)+",\"duty_percent\":"+String(output.duty*100.0f/config::PWM_RANGE,1)+
    ",\"bus_volts\":"+jsonFloat(busVolts)+",\"motor_amps\":"+jsonFloat(motorAmps)+
    ",\"motor_watts\":"+jsonFloat(inaAvailable?busVolts*motorAmps:NAN,1)+
    ",\"peak_motor_amps\":"+jsonFloat(energy.peakAmps())+
    ",\"recent_avg_amps\":"+jsonFloat(recent.amps)+
    ",\"recent_avg_watts\":"+jsonFloat(recent.watts,1)+
    ",\"recent_seconds\":"+jsonFloat(recent.observedSeconds,0)+
    ",\"motor_used_ah\":"+jsonFloat(energy.hasData()?energy.consumedAh():NAN,3)+
    ",\"motor_used_wh\":"+jsonFloat(energy.hasData()?energy.consumedWh():NAN,2)+
    ",\"charge_reference_pct\":"+jsonFloat(energy.startingChargePercent(),1)+
    ",\"estimated_charge_pct\":"+jsonFloat(energy.estimatedPercent(settings.batteryCapacityAh),1)+
    ",\"estimated_remaining_min\":"+jsonFloat(energy.minutesRemaining(millis(),settings.batteryCapacityAh),0)+
    ",\"charging_amps\":null,\"charging_current_supported\":false"+
    ",\"motor_temp\":"+jsonFloat(motorTempC,1)+",\"gearbox_temp\":"+jsonFloat(gearboxTempC,1)+
    ",\"pwm_supply_volts\":"+jsonFloat(supplyForPwm(),2)+",\"adc\":"+String(adc)+
    ",\"charger\":"+(chargerPresent?"true":"false")+
    ",\"voltage_inhibit\":"+(voltageGuard.inhibited()?"true":"false")+
    ",\"ota_busy\":"+(otaBusy?"true":"false")+
    ",\"ota_status\":\""+escapeJson(otaResult)+"\""+",\"ina_available\":"+(inaAvailable?"true":"false")+
    ",\"oled_available\":"+(oledAvailable?"true":"false")+
    ",\"load_monitor_trip\":"+String(static_cast<int>(loadProtection.trip()))+
    ",\"current_limit_v\":"+jsonFloat(currentDerateV,2)+
    ",\"current_derating\":"+(loadProtection.derating()?"true":"false")+
    ",\"emergency_monitor_enabled\":"+(settings.emergencyTripA>0?"true":"false")+
    ",\"derate_monitor_enabled\":"+(settings.derateTripA>0?"true":"false")+
    ",\"stall_monitor_enabled\":"+(settings.stallTripA>0?"true":"false")+
    ",\"no_load_monitor_enabled\":"+(settings.noLoadBelowA>0?"true":"false")+
    ",\"motor_hot\":"+(motorHot?"true":"false")+",\"gearbox_hot\":"+(gearboxHot?"true":"false")+
    ",\"motor_sensor_found\":"+(motorFound?"true":"false")+
    ",\"gearbox_sensor_found\":"+(gearboxFound?"true":"false")+",\"probes\":"+probeListJson()+"}";
  return s;
}
static void jsonResponse(const String& value) {
  web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",value);
}
static bool authorizedPost() {
  if(web.header("X-Quad-Control")!="1") {web.send(403,"text/plain","Missing control header");return false;}
  return true;
}
static bool numberArg(const char* key,float minVal,float maxVal,float& out) {
  if(!web.hasArg(key))return false;
  const String value=web.arg(key);
  if(value.isEmpty())return false;
  char* end=nullptr;
  const float parsed=strtof(value.c_str(),&end);
  if(end==value.c_str()||*end!='\0'||!isfinite(parsed)||parsed<minVal||parsed>maxVal)return false;
  out=parsed;return true;
}
static void handleSettingsPost() {
  if(!authorizedPost())return;
  // Prevent remote changes to cutoff thresholds while output can move.
  if(output.enabled || remoteHeld || digitalRead(config::THROTTLE_PIN)==LOW) {
    web.send(409,"text/plain","Release throttle and stop motor before changing settings");return;
  }
  float ramp,forward,reverse,motorCut,gearCut,emergencyA,derateA,derateSecs,derateMinimumV,stallA,stallSecs,noLoadA,noLoadSecs,capacityAh,chargerCutoffV;
  if(!numberArg("ramp_s",0.1f,30,ramp)||!numberArg("forward_v",0.5f,12,forward)||
     !numberArg("reverse_v",0.5f,12,reverse)||!numberArg("motor_cutoff_c",20,120,motorCut)||
     !numberArg("gearbox_cutoff_c",20,120,gearCut)||
     !numberArg("emergency_trip_a",0,50,emergencyA)||
     !numberArg("derate_trip_a",0,50,derateA)||
     !numberArg("derate_trip_s",0.2f,5,derateSecs)||
     !numberArg("derate_minimum_v",0.5f,12,derateMinimumV)||
     !numberArg("stall_trip_a",0,50,stallA)||!numberArg("stall_trip_s",0.2f,5,stallSecs)||
     !numberArg("no_load_below_a",0,20,noLoadA)||!numberArg("no_load_trip_s",0.5f,10,noLoadSecs)||
     !numberArg("battery_capacity_ah",1,100,capacityAh)||
     !numberArg("charger_cutoff_v",12.5f,15.5f,chargerCutoffV)||!web.hasArg("motor_sensor")||!web.hasArg("gearbox_sensor")) {
    web.send(400,"text/plain","Invalid config values");return;
  }
  quad::Settings updated=settings;
  const uint16_t rampMs=static_cast<uint16_t>(ramp*1000.0f+0.5f);
  updated.rampMs=rampMs;updated.forwardV=forward;updated.reverseV=reverse;
  updated.motorCutoffC=motorCut;updated.gearboxCutoffC=gearCut;
  updated.emergencyTripA=emergencyA;
  updated.derateTripA=derateA;
  updated.derateTripMs=static_cast<uint16_t>(derateSecs*1000.0f+0.5f);
  updated.derateMinimumV=derateMinimumV;
  updated.stallTripA=stallA;updated.stallTripMs=static_cast<uint16_t>(stallSecs*1000.0f+0.5f);
  updated.noLoadBelowA=noLoadA;updated.noLoadTripMs=static_cast<uint16_t>(noLoadSecs*1000.0f+0.5f);
  updated.batteryCapacityAh=capacityAh;updated.chargerCutoffV=chargerCutoffV;
  // Password fields are intentionally not sent by GET. Blank means keep.
  const auto storeText=[](char* target, size_t capacity, const String& value) {
    memset(target,0,capacity);
    memcpy(target,value.c_str(),value.length());
  };
  if(!web.hasArg("ap_ssid")||!web.hasArg("ap_password")||
     !web.hasArg("ota_ssid")||!web.hasArg("ota_password")) {
    web.send(400,"text/plain","Missing Wi-Fi settings");return;
  }
  String newAp=web.arg("ap_ssid"),newApPass=web.arg("ap_password");
  String newSta=web.arg("ota_ssid"),newStaPass=web.arg("ota_password");
  if(newAp.length()>32||newApPass.length()>63||newSta.length()>32||newStaPass.length()>63) {
    web.send(400,"text/plain","Wi-Fi setting too long");return;
  }
  // Empty AP SSID means generated chip-specific default.
  if(newAp==apName()) newAp=settings.apSsid;
  storeText(updated.apSsid,sizeof(updated.apSsid),newAp);
  if(newApPass.length()) storeText(updated.apPassword,sizeof(updated.apPassword),newApPass);
  storeText(updated.otaSsid,sizeof(updated.otaSsid),newSta);
  if(!newSta.length()) memset(updated.otaPassword,0,sizeof(updated.otaPassword));
  else if(newStaPass.length()) storeText(updated.otaPassword,sizeof(updated.otaPassword),newStaPass);
  if(!parseRom(web.arg("motor_sensor"),updated.motorRom)||!parseRom(web.arg("gearbox_sensor"),updated.gearboxRom)) {
    web.send(400,"text/plain","Malformed sensor address");return;
  }
  // Valid selection is either a currently present probe or the previously saved missing probe.
  if((!quad::emptyRom(updated.motorRom)&&probeIndex(updated.motorRom)<0&&memcmp(updated.motorRom,settings.motorRom,8)!=0)||
     (!quad::emptyRom(updated.gearboxRom)&&probeIndex(updated.gearboxRom)<0&&memcmp(updated.gearboxRom,settings.gearboxRom,8)!=0)||
     (!quad::emptyRom(updated.motorRom)&&memcmp(updated.motorRom,updated.gearboxRom,8)==0)) {
    web.send(400,"text/plain","Invalid or duplicated probe selection");return;
  }
  updated.checksum=quad::settingsChecksum(updated);
  if(!quad::validSettings(updated)) {web.send(400,"text/plain","Invalid settings");return;}
  const bool wifiChanged = strcmp(updated.apSsid,settings.apSsid)!=0 ||
     strcmp(updated.apPassword,settings.apPassword)!=0 ||
     strcmp(updated.otaSsid,settings.otaSsid)!=0 ||
     strcmp(updated.otaPassword,settings.otaPassword)!=0;
  EEPROM.put(0,updated);
  if(!EEPROM.commit()) {web.send(500,"text/plain","Could not write flash");return;}
  settings=updated;
  // New limits take effect without clearing a latched fault.
  loadProtection.resetDerating();currentDerateV=12.0f;
  controller.setLimits(controllerLimits());
  refreshThermal();
  jsonResponse(settingsJson());
  if(wifiChanged) {rebootQueued=true;rebootAt=millis()+1500;}
}
static void handleChargeReferencePost() {
  if(!authorizedPost())return;
  float percent;
  if(!numberArg("percent",0,100,percent)) {web.send(400,"text/plain","Invalid starting battery charge");return;}
  energy.setChargeReference(percent);
  jsonResponse("{\"ok\":true}");
}
static void handleDrivePost() {
  if(!authorizedPost())return;
  const String hold=web.arg("hold"),direction=web.arg("direction");
  if((hold!="1"&&hold!="0")||(direction!="fwd"&&direction!="rev")) {
    web.send(400,"text/plain","Invalid drive command");return;
  }
  if(otaBusy){web.send(423,"text/plain","Firmware update in progress");return;}
  remoteHeld=hold=="1";
  remoteDirection=direction=="fwd"?quad::Direction::Forward:quad::Direction::Reverse;
  lastRemoteMs=millis();
  jsonResponse("{\"ok\":true}");
}
static void portalRedirect() {
  web.sendHeader("Location","http://192.168.4.1/",true);
  web.sendHeader("Cache-Control","no-store");
  web.send(302,"text/plain","");
}
static bool idleForUpdate() {
  return !otaBusy && !output.enabled && !remoteHeld &&
      digitalRead(config::THROTTLE_PIN)!=LOW;
}
static void handleOtaFromUrl() {
  if(!authorizedPost()) return;
  if(!idleForUpdate()) {web.send(409,"text/plain","Release throttle and stop motor first");return;}
  if(!settings.otaSsid[0]) {web.send(400,"text/plain","Set internet Wi-Fi SSID and password first");return;}
  otaBusy=true;otaRequested=true;otaResult="Connecting for OTA";
  remoteHeld=false;disableDrive();
  jsonResponse("{\"ok\":true,\"message\":\"OTA queued; driving disabled\"}");
}
static void handleOtaUploadDone() {
  if(!authorizedPost()) return;
  if(otaUploadOk && !Update.hasError()) {
    web.send(200,"text/plain","Firmware installed. Rebooting.");
    rebootQueued=true;rebootAt=millis()+1200;
  } else {
    web.send(400,"text/plain","Upload failed. Check binary and flash space.");
    otaBusy=false;otaResult="Upload failed";
  }
}
static void handleOtaUpload() {
  HTTPUpload& upload=web.upload();
  if(upload.status==UPLOAD_FILE_START) {
    otaUploadOk=false;
    if(web.header("X-Quad-Control")!="1"||!idleForUpdate())return;
    otaBusy=true;remoteHeld=false;disableDrive();
    otaResult="Receiving firmware";
    otaUploadOk=Update.begin(ESP.getFreeSketchSpace(), U_FLASH);
  } else if(upload.status==UPLOAD_FILE_WRITE) {
    if(otaBusy && otaUploadOk && Update.write(upload.buf, upload.currentSize)!=upload.currentSize)
      otaUploadOk=false;
  } else if(upload.status==UPLOAD_FILE_END) {
    if(otaBusy && otaUploadOk) otaUploadOk=Update.end(true);
    else if(otaBusy) Update.abort();
  } else if(upload.status==UPLOAD_FILE_ABORTED) {
    if(otaBusy)Update.abort();otaBusy=false;otaUploadOk=false;otaResult="Upload aborted";
  }
}
static void runOtaFromUrl() {
  otaRequested=false;
  // Only use STA during OTA; do not expose the drive web server on home Wi-Fi.
  disableDrive();remoteHeld=false;
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(settings.otaSsid,settings.otaPassword);
  const uint32_t started=millis();
  while(WiFi.status()!=WL_CONNECTED && static_cast<uint32_t>(millis()-started)<15000) delay(50);
  if(WiFi.status()!=WL_CONNECTED) otaResult="OTA Wi-Fi connection failed";
  else {
    configTime(0,0,"pool.ntp.org","time.google.com");
    const uint32_t clockStart=millis();
    while(time(nullptr)<1700000000 && static_cast<uint32_t>(millis()-clockStart)<12000) delay(100);
    if(time(nullptr)<1700000000) otaResult="OTA TLS clock sync failed";
    else {
      BearSSL::X509List trustRoots(OTA_TRUST_ROOTS);
      BearSSL::WiFiClientSecure client;
      client.setTrustAnchors(&trustRoots);
      client.setBufferSizes(1024,1024);
      ESPhttpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      ESPhttpUpdate.setClientTimeout(15000);
      ESPhttpUpdate.rebootOnUpdate(true);
      const t_httpUpdate_return result=ESPhttpUpdate.update(client,OTA_URL);
      if(result==HTTP_UPDATE_FAILED) otaResult="OTA failed: "+ESPhttpUpdate.getLastErrorString();
      else if(result==HTTP_UPDATE_NO_UPDATES) otaResult="Already current";
      // HTTP_UPDATE_OK normally reboots automatically.
    }
  }
  Serial.println(otaResult);
  WiFi.disconnect(false);WiFi.mode(WIFI_AP);
  otaBusy=false;
}
static void startPortal() {
  WiFi.persistent(false);WiFi.mode(WIFI_AP);
  String ssid=apName();
  const IPAddress ip(192,168,4,1);
  WiFi.softAPConfig(ip,ip,IPAddress(255,255,255,0));
  WiFi.softAP(ssid.c_str(),settings.apPassword);
  dns.start(53,"*",ip);
  web.collectHeaders("X-Quad-Control");
  web.on("/",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send_P(200,"text/html; charset=utf-8",WEB_UI);});
  web.on("/api/status",HTTP_GET,[]{jsonResponse(statusJson());});
  web.on("/api/config",HTTP_GET,[]{jsonResponse(settingsJson());});
  web.on("/api/config",HTTP_POST,handleSettingsPost);
  web.on("/api/drive",HTTP_POST,handleDrivePost);
  web.on("/api/battery/reference",HTTP_POST,handleChargeReferencePost);
  web.on("/api/ota/url",HTTP_POST,handleOtaFromUrl);
  web.on("/api/ota/upload",HTTP_POST,handleOtaUploadDone,handleOtaUpload);
  // OS captive-portal detection URLs; unknown GETs redirect to local UI.
  web.on("/generate_204",HTTP_GET,portalRedirect);
  web.on("/gen_204",HTTP_GET,portalRedirect);
  web.on("/hotspot-detect.html",HTTP_GET,portalRedirect);
  web.on("/library/test/success.html",HTTP_GET,portalRedirect);
  web.on("/ncsi.txt",HTTP_GET,portalRedirect);
  web.on("/connecttest.txt",HTTP_GET,portalRedirect);
  web.on("/fwlink",HTTP_GET,portalRedirect);
  web.onNotFound(portalRedirect);
  web.begin();
  Serial.printf("Wi-Fi AP: %s / http://192.168.4.1/\n",ssid.c_str());
}
static void drawDisplay() {
  if(!oledAvailable)return;
  char line[32];display.clearBuffer();display.setFont(u8g2_font_6x10_tf);
  snprintf(line,sizeof(line),"QUAD %s",stateName(output.state));display.drawStr(0,10,line);
  snprintf(line,sizeof(line),"%s %u%% %s",directionName(output.direction),
    static_cast<unsigned>(output.duty*100UL/config::PWM_RANGE),sourceName(output.source));display.drawStr(0,22,line);
  if(inaAvailable)snprintf(line,sizeof(line),"%.2fV %.1fA",busVolts,motorAmps);
  else snprintf(line,sizeof(line),"INA missing (12V est)");
  display.drawStr(0,34,line);
  if(motorFound&&gearboxFound)snprintf(line,sizeof(line),"M%.0fC G%.0fC",motorTempC,gearboxTempC);
  else if(motorFound)snprintf(line,sizeof(line),"M%.0fC G---",motorTempC);
  else if(gearboxFound)snprintf(line,sizeof(line),"M--- G%.0fC",gearboxTempC);
  else snprintf(line,sizeof(line),"Temp probe(s) missing");
  display.drawStr(0,46,line);
  if(otaBusy)snprintf(line,sizeof(line),"OTA UPDATE STOPPED");
  else if(voltageGuard.inhibited())snprintf(line,sizeof(line),"HIGH VOLT / CHARGER?");
  else if(motorHot||gearboxHot)snprintf(line,sizeof(line),"HOT %s 5V LIMIT",motorHot?(gearboxHot?"M+G":"M"):"G");
  else if(output.fault==quad::Fault::EmergencyCurrent)snprintf(line,sizeof(line),"EMERGENCY CURRENT CUT");
  else if(output.fault==quad::Fault::Overcurrent)snprintf(line,sizeof(line),"STALL CURRENT CUT");
  else if(output.fault==quad::Fault::NoLoad)snprintf(line,sizeof(line),"POSSIBLE NO-LOAD CUT");
  else if(output.fault!=quad::Fault::None)snprintf(line,sizeof(line),"FAULT: %s",faultName(output.fault));
  else if(loadProtection.derating())snprintf(line,sizeof(line),"LOAD LIMIT %.1fV",currentDerateV);
  else {
    const auto recent=energy.recent(millis());
    const float remain=energy.minutesRemaining(millis(),settings.batteryCapacityAh);
    if(isfinite(recent.amps) && recent.observedSeconds>=30) {
      if(isfinite(remain))snprintf(line,sizeof(line),"5m %.1fA ~%.0fmin",recent.amps,remain);
      else snprintf(line,sizeof(line),"5m %.1fA ETA:--",recent.amps);
    } else snprintf(line,sizeof(line),"ADC:%u WIFI:ON",adc);
  }
  display.drawStr(0,58,line);display.sendBuffer();
}
void setup() {
  digitalWrite(config::ENABLE_PIN,LOW);pinMode(config::ENABLE_PIN,OUTPUT);
  digitalWrite(config::RPWM_PIN,LOW);pinMode(config::RPWM_PIN,OUTPUT);
  digitalWrite(config::LPWM_PIN,LOW);pinMode(config::LPWM_PIN,OUTPUT);
  analogWriteRange(config::PWM_RANGE);analogWriteFreq(config::PWM_FREQUENCY_HZ);
  disableDrive();
  pinMode(config::THROTTLE_PIN,INPUT); // GPIO16 requires external pull-up
  // D7 unused: 2-wire charging jack cannot sense plug insertion.
  Serial.begin(115200);Serial.println(F("Quad controller rev F v0.4"));
  loadSettings();
  Wire.begin(config::I2C_SDA_PIN,config::I2C_SCL_PIN);Wire.setClock(100000);
  inaAvailable=writeInaRegister(0x00,0x4127)&&pollIna();
  voltageGuard.step(millis(),inaAvailable,busVolts,settings.chargerCutoffV);
  chargerPresent=voltageGuard.inhibited();
  Wire.beginTransmission(config::OLED_ADDRESS);
  oledAvailable=Wire.endTransmission()==0;
  if(oledAvailable) {display.setI2CAddress(config::OLED_ADDRESS<<1);display.begin();}
  thermometers.begin();thermometers.setWaitForConversion(false);scanProbes();
  refreshThermal();
  startPortal();
  Serial.printf("INA=%d OLED=%d PWM supply=%.2fV\n",inaAvailable,oledAvailable,supplyForPwm());
}
void loop() {
  const uint32_t now=millis();
  if(rebootQueued && static_cast<int32_t>(now-rebootAt)>=0) {disableDrive();ESP.restart();}
  dns.processNextRequest();web.handleClient();
  if(otaRequested) runOtaFromUrl();
  if(static_cast<uint32_t>(now-lastIna)>=config::INA_PERIOD_MS) {
    lastIna=now;
    inaAvailable=pollIna();
    voltageGuard.step(now,inaAvailable,busVolts,settings.chargerCutoffV);
    chargerPresent=voltageGuard.inhibited();
    if(chargerPresent || otaBusy)disableDrive();
    energy.update(now,motorAmps,busVolts,inaAvailable);
    quad::LoadProtection::Settings thresholds;
    thresholds.emergencyA=settings.emergencyTripA;
    thresholds.derateA=settings.derateTripA;
    thresholds.derateMs=settings.derateTripMs;
    thresholds.derateMinimumV=settings.derateMinimumV;
    thresholds.stallA=settings.stallTripA;
    thresholds.stallMs=settings.stallTripMs;
    thresholds.noLoadBelowA=settings.noLoadBelowA;
    thresholds.noLoadMs=settings.noLoadTripMs;
    loadProtection.evaluate(now,output.enabled,output.duty,motorAmps,inaAvailable,thresholds);
    currentDerateV=loadProtection.derateCapVolts();
    if(loadProtection.trip()!=quad::LoadProtection::Trip::None) disableDrive(); // do not await next control tick
  }
  pollTemperatures(now);
  if(static_cast<uint32_t>(now-lastControl)>=config::INPUT_PERIOD_MS) {
    lastControl=now;
    const bool raw=digitalRead(config::THROTTLE_PIN)==LOW;
    if(raw&&!priorRawPress)throttlePressedSince=now;
    priorRawPress=raw;
    const bool pedal=raw&&static_cast<uint32_t>(now-throttlePressedSince)>=config::PRESS_DEBOUNCE_MS;
    const bool wifi=remoteHeld&&static_cast<uint32_t>(now-lastRemoteMs)<config::WIFI_DRIVE_TTL_MS&&WiFi.softAPgetStationNum()>0;
    if(!wifi)remoteHeld=false;

    chargerPresent=voltageGuard.inhibited();
    adc=readDirectionAdc();
    physicalDirection=quad::decodeDirection(adc,config::FORWARD_MIN,config::FORWARD_MAX,config::REVERSE_MIN,config::REVERSE_MAX);
    quad::Input input;
    input.physicalPressed=pedal;input.wifiPressed=wifi;input.charger=chargerPresent || otaBusy;
    input.physicalDirection=physicalDirection;input.wifiDirection=remoteDirection;
    input.overcurrent=loadProtection.trip()==quad::LoadProtection::Trip::HighCurrent;
    input.emergencyCurrent=loadProtection.trip()==quad::LoadProtection::Trip::EmergencyCurrent;
    input.noLoad=loadProtection.trip()==quad::LoadProtection::Trip::NoLoad;
    input.thermalLimit=motorHot||gearboxHot;
    input.currentLimitVolts=currentDerateV;
    input.supplyVolts=supplyForPwm();
    output=controller.step(now,input);
    if(otaBusy)disableDrive();else applyDrive(output);
    // Only clear a latched fault after all commands are released AND output
    // is off (including the physical pedal's 2-second release-hold period).
    if(!raw&&!wifi&&!output.enabled) loadProtection.resetAfterRelease();
  }
  if(static_cast<uint32_t>(now-lastDisplay)>=config::DISPLAY_PERIOD_MS) {
    lastDisplay=now;drawDisplay();
  }
  if(static_cast<uint32_t>(now-lastLog)>=config::LOG_PERIOD_MS) {
    lastLog=now;
    Serial.printf("%s %s %s duty=%u ADC=%u V=%.2f A=%.2f M=%.1f G=%.1f derate=%u\n",
      stateName(output.state),directionName(output.direction),sourceName(output.source),output.duty,adc,
      busVolts,motorAmps,motorTempC,gearboxTempC,motorHot||gearboxHot);
  }
  yield();
}

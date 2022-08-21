#if (!defined(ESP32))
#error This code is intended to run only on the ESP32 platform! Please check your Tools->Board setting.
#endif

#define ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET "ESPAsync_WiFiManager v1.12.2"
#define ESP_ASYNC_WIFIMANAGER_VERSION_MIN 1012002

// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _ESPASYNC_WIFIMGR_LOGLEVEL_ 3

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>

#include <AsyncElegantOTA.h>

// From v1.1.0
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
//////

// You only need to format the filesystem once
// #define FORMAT_FILESYSTEM true
#define FORMAT_FILESYSTEM false
// LittleFS has higher priority than SPIFFS
#if (defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2))
#define USE_LITTLEFS true
#define USE_SPIFFS false
#elif defined(ARDUINO_ESP32C3_DEV)
// For core v1.0.6-, ESP32-C3 only supporting SPIFFS and EEPROM. To use v2.0.0+ for LittleFS
#define USE_LITTLEFS false
#define USE_SPIFFS true
#endif

#if USE_LITTLEFS
// Use LittleFS
#include "FS.h"

// Check cores/esp32/esp_arduino_version.h and cores/esp32/core_version.h
//#if ( ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0) )  //(ESP_ARDUINO_VERSION_MAJOR >= 2)
#if (defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2))
#if (_ESPASYNC_WIFIMGR_LOGLEVEL_ > 3)
#warning Using ESP32 Core 1.0.6 or 2.0.0+
#endif

// The library has been merged into esp32 core from release 1.0.6
#include <LittleFS.h> // https://github.com/espressif/arduino-esp32/tree/master/libraries/LittleFS

FS *filesystem = &LittleFS;
#define FileFS LittleFS
#define FS_Name "LittleFS"
#else
#if (_ESPASYNC_WIFIMGR_LOGLEVEL_ > 3)
#warning Using ESP32 Core 1.0.5-. You must install LITTLEFS library
#endif

// The library has been merged into esp32 core from release 1.0.6
#include <LITTLEFS.h> // https://github.com/lorol/LITTLEFS

FS *filesystem = &LITTLEFS;
#define FileFS LITTLEFS
#define FS_Name "LittleFS"
#endif

#elif USE_SPIFFS
#include <SPIFFS.h>
FS *filesystem = &SPIFFS;
#define FileFS SPIFFS
#define FS_Name "SPIFFS"
#else
// Use FFat
#include <FFat.h>
FS *filesystem = &FFat;
#define FileFS FFat
#define FS_Name "FFat"
#endif

#define LED_BUILTIN 2
#define LED_ON HIGH
#define LED_OFF LOW

#include <SPIFFSEditor.h>

// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS)
// Otherwise, library will use default EEPROM storage
#if USE_LITTLEFS
#define ESP_DRD_USE_LITTLEFS true
#define ESP_DRD_USE_SPIFFS false
#define ESP_DRD_USE_EEPROM false
#elif USE_SPIFFS
#define ESP_DRD_USE_LITTLEFS false
#define ESP_DRD_USE_SPIFFS true
#define ESP_DRD_USE_EEPROM false
#else
#define ESP_DRD_USE_LITTLEFS false
#define ESP_DRD_USE_SPIFFS false
#define ESP_DRD_USE_EEPROM true
#endif

#define DEBUG true

#pragma region WifiConfig
// DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
// DoubleResetDetector *drd = NULL;
//////

// SSID and PW for Config Portal
String ssid = "ESP_" + String((uint32_t)ESP.getEfuseMac(), HEX);

const bool IS_TEST = ssid.compareTo("ESP_cfa3c9c8") == 0 ? true : false; // ESP_cfa3c9c8

String password;
// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

// From v1.1.0
#define MIN_AP_PASSWORD_SIZE 8

#define SSID_MAX_LEN 32
// From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN 64

typedef struct
{
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

typedef struct
{
  String wifi_ssid;
  String wifi_pw;
} WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS 2

// Assuming max 49 chars
#define TZNAME_MAX_LEN 50
#define TIMEZONE_MAX_LEN 50

typedef struct
{
  WiFi_Credentials WiFi_Creds[NUM_WIFI_CREDENTIALS];
  char TZ_Name[TZNAME_MAX_LEN]; // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];    // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} WM_Config;

WM_Config WM_config;

#define CONFIG_FILENAME F("/wifi_cred.dat")
//////

// Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
bool initialConfig = false;

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h>
#define USE_AVAILABLE_PAGES false

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
#define USE_STATIC_IP_CONFIG_IN_CP false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP true

// Just use enough to save memory. On ESP8266, can cause blank ConfigPortal screen
// if using too much memory
#define USING_AFRICA false
#define USING_AMERICA true
#define USING_ANTARCTICA false
#define USING_ASIA false
#define USING_ATLANTIC false
#define USING_AUSTRALIA false
#define USING_EUROPE false
#define USING_INDIAN false
#define USING_PACIFIC false
#define USING_ETC_GMT false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP false

#define USING_CORS_FEATURE true

////////////////////////////////////////////

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
// Force DHCP to be true
#if defined(USE_DHCP_IP)
#undef USE_DHCP_IP
#endif
#define USE_DHCP_IP true
#else
// You can select DHCP or Static IP here
#define USE_DHCP_IP true
//#define USE_DHCP_IP     false
#endif

#if (USE_DHCP_IP)
// Use DHCP

#if (_ESPASYNC_WIFIMGR_LOGLEVEL_ > 3)
#warning Using DHCP IP
#endif

IPAddress stationIP = IPAddress(0, 0, 0, 0);
IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
IPAddress netMask = IPAddress(255, 255, 255, 0);

#else
// Use static IP

#if (_ESPASYNC_WIFIMGR_LOGLEVEL_ > 3)
#warning Using static IP
#endif

#ifdef ESP32
IPAddress stationIP = IPAddress(192, 168, 2, 232);
#else
IPAddress stationIP = IPAddress(192, 168, 2, 186);
#endif

IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
IPAddress netMask = IPAddress(255, 255, 255, 0);
#endif

////////////////////////////////////////////

#define USE_CONFIGURABLE_DNS true

IPAddress dns1IP = gatewayIP;
IPAddress dns2IP = IPAddress(8, 8, 8, 8);

#define USE_CUSTOM_AP_IP false

IPAddress APStaticIP = IPAddress(192, 168, 100, 1);
IPAddress APStaticGW = IPAddress(192, 168, 100, 1);
IPAddress APStaticSN = IPAddress(255, 255, 255, 0);

#include <ESPAsync_WiFiManager.h> //https://github.com/khoih-prog/ESPAsync_WiFiManager

// Redundant, for v1.10.0 only
//#include <ESPAsync_WiFiManager-Impl.h>          //https://github.com/khoih-prog/ESPAsync_WiFiManager

const String host = IS_TEST ? "kombi2" : "kombi";
// const String host = "kombi";

#define HTTP_PORT 80

AsyncWebServer server(HTTP_PORT);
DNSServer dnsServer;

AsyncEventSource events("/events");
// AsyncWebSocket ws("/ws");

String http_username = "casakombiamora";
String http_password = "password";

String separatorLine = F("===============================================================");

///////////////////////////////////////////
// New in v1.4.0
/******************************************
 * // Defined in ESPAsync_WiFiManager.h
typedef struct
{
  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;

}  WiFi_AP_IPConfig;

typedef struct
{
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;
#if USE_CONFIGURABLE_DNS
  IPAddress _sta_static_dns1;
  IPAddress _sta_static_dns2;
#endif
}  WiFi_STA_IPConfig;
******************************************/

WiFi_AP_IPConfig WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig)
{
  in_WM_AP_IPconfig._ap_static_ip = APStaticIP;
  in_WM_AP_IPconfig._ap_static_gw = APStaticGW;
  in_WM_AP_IPconfig._ap_static_sn = APStaticSN;
}

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig)
{
  in_WM_STA_IPconfig._sta_static_ip = stationIP;
  in_WM_STA_IPconfig._sta_static_gw = gatewayIP;
  in_WM_STA_IPconfig._sta_static_sn = netMask;
#if USE_CONFIGURABLE_DNS
  in_WM_STA_IPconfig._sta_static_dns1 = dns1IP;
  in_WM_STA_IPconfig._sta_static_dns2 = dns2IP;
#endif
}

void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, F(", gatewayIP ="), in_WM_STA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
#if USE_CONFIGURABLE_DNS
  LOGERROR3(F("dns1IP ="), in_WM_STA_IPconfig._sta_static_dns1, F(", dns2IP ="), in_WM_STA_IPconfig._sta_static_dns2);
#endif
}

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
#if USE_CONFIGURABLE_DNS
  // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
  WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn, in_WM_STA_IPconfig._sta_static_dns1, in_WM_STA_IPconfig._sta_static_dns2);
#else
  // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
  WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn);
#endif
}

///////////////////////////////////////////

#pragma region WebSerial

#include <WebSerial.h>

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len)
{
  WebSerial.println("Received Data...");
  String d = "";
  for (int i = 0; i < len; i++)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);
}
#pragma endregion

///////////////////////////////////////////

uint8_t connectMultiWiFi()
{
#if ESP32
// For ESP32, this better be 0 to shorten the connect time.
// For ESP32-S2/C3, must be > 500
#if (USING_ESP32_S2 || USING_ESP32_C3)
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 500L
#else
// For ESP32 core v1.0.6, must be >= 500
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 800L
#endif
#else
// For ESP8266, this better be 2200 to enable connect the 1st time
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 2200L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS 500L

  uint8_t status;

  // WiFi.mode(WIFI_STA);

  LOGERROR(F("ConnectMultiWiFi with :"));

  if ((Router_SSID != "") && (Router_Pass != ""))
  {
    LOGERROR3(F("* Flash-stored Router_SSID = "), Router_SSID, F(", Router_Pass = "), Router_Pass);
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
  }

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
  {
    // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
    if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE))
    {
      LOGERROR3(F("* Additional SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw);
    }
  }

  LOGERROR(F("Connecting MultiWifi..."));

  // WiFi.mode(WIFI_STA);

#if !USE_DHCP_IP
  // New in v1.4.0
  configWiFi(WM_STA_IPconfig);
  //////
#endif

  WiFi.hostname(host);
  int i = 0;
  status = wifiMulti.run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ((i++ < 20) && (status != WL_CONNECTED))
  {
    status = WiFi.status();

    if (status == WL_CONNECTED)
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if (status == WL_CONNECTED)
  {
    LOGERROR1(F("WiFi connected after time: "), i);
    LOGERROR3(F("SSID:"), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
    LOGERROR3(F("Channel:"), WiFi.channel(), F(",IP address:"), WiFi.localIP());
  }
  else
  {
    LOGERROR(F("WiFi not connected"));

    // To avoid unnecessary DRD
    // drd->loop();

    ESP.restart();
  }

  return status;
}

// format bytes
String formatBytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + "B";
  }
  else if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0) + "KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
  else
  {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

void toggleLED()
{
  // toggle state
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

#if USE_ESP_WIFIMANAGER_NTP

void printLocalTime()
{
#if ESP8266
  static time_t now;

  now = time(nullptr);

  if (now > 1451602800)
  {
    Serial.print("Local Date/Time: ");
    Serial.print(ctime(&now));
  }
#else
  struct tm timeinfo;

  getLocalTime(&timeinfo);

  // Valid only if year > 2000.
  // You can get from timeinfo : tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec
  if (timeinfo.tm_year > 100)
  {
    Serial.print("Local Date/Time: ");
    const char *timeString = asctime(&timeinfo);
    Serial.print(timeString);
    WebSerial.println(timeString);
    events.send(timeString, "time", millis());
  }
#endif
}

#endif

void heartBeatPrint()
{
#if USE_ESP_WIFIMANAGER_NTP
  printLocalTime();
#else
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print(F("H")); // H means connected to WiFi
  else
    Serial.print(F("F")); // F means not connected to WiFi

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
#endif
}

void check_WiFi()
{
  if ((WiFi.status() != WL_CONNECTED))
  {
    Serial.println(F("\nWiFi lost. Call connectMultiWiFi in loop"));
    connectMultiWiFi();
  }
}

void check_status()
{
  static ulong checkstatus_timeout = 0;
  static ulong LEDstatus_timeout = 0;
  static ulong checkwifi_timeout = 0;

  static ulong current_millis;

#define WIFICHECK_INTERVAL 1000L

#if USE_ESP_WIFIMANAGER_NTP
#define HEARTBEAT_INTERVAL 60000L
#else
#define HEARTBEAT_INTERVAL 10000L
#endif

#define LED_INTERVAL 2000L

  current_millis = millis();

  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0))
  {
    check_WiFi();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  if ((current_millis > LEDstatus_timeout) || (LEDstatus_timeout == 0))
  {
    // Toggle LED at LED_INTERVAL = 2s
    toggleLED();
    LEDstatus_timeout = current_millis + LED_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}

int calcChecksum(uint8_t *address, uint16_t sizeToCalc)
{
  uint16_t checkSum = 0;

  for (uint16_t index = 0; index < sizeToCalc; index++)
  {
    checkSum += *(((byte *)address) + index);
  }

  return checkSum;
}

bool loadConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  memset((void *)&WM_config, 0, sizeof(WM_config));

  // New in v1.4.0
  memset((void *)&WM_STA_IPconfig, 0, sizeof(WM_STA_IPconfig));
  //////

  if (file)
  {
    file.readBytes((char *)&WM_config, sizeof(WM_config));

    // New in v1.4.0
    file.readBytes((char *)&WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));

    if (WM_config.checksum != calcChecksum((uint8_t *)&WM_config, sizeof(WM_config) - sizeof(WM_config.checksum)))
    {
      LOGERROR(F("WM_config checksum wrong"));

      return false;
    }

    // New in v1.4.0
    displayIPConfigStruct(WM_STA_IPconfig);
    //////

    return true;
  }
  else
  {
    LOGERROR(F("failed"));

    return false;
  }
}

void saveConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveWiFiCfgFile "));

  if (file)
  {
    WM_config.checksum = calcChecksum((uint8_t *)&WM_config, sizeof(WM_config) - sizeof(WM_config.checksum));

    file.write((uint8_t *)&WM_config, sizeof(WM_config));

    displayIPConfigStruct(WM_STA_IPconfig);

    // New in v1.4.0
    file.write((uint8_t *)&WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));
  }
  else
  {
    LOGERROR(F("failed"));
  }
}

#pragma endregion

#pragma region SetupSensors

int PIN_LEVEL_REF = 15;
int PIN_RELAY_SHOWER = 4;
int status_shower = 0;
int old_status_shower = 0;

// int cx1_status = B00000000; //Star with 0  14 = 0000 1111
// int cx1_pins[] = {9, 10, 11};
int cx1_pins[] = {13, 12, 14, 27};

const int NUM_PINS = sizeof(cx1_pins) / sizeof(cx1_pins[0]);

const unsigned long SPEED_SERIAL = 115200;
int INTERVAL_READ_PINS = 3000;

// void setGND(int pinOut, bool isGND)
// {
//   if (isGND)
//   {
//     pinMode(pinOut, OUTPUT);
//     digitalWrite(pinOut, LOW);
//   }
//   else
//   {
//     pinMode(pinOut, INPUT_PULLUP);
//   }
// }

void relayShower(bool activate)
{
  digitalWrite(PIN_RELAY_SHOWER, activate ? LOW : HIGH);
}

void setupPins(int arr_pins[])
{
  pinMode(PIN_LEVEL_REF, OUTPUT);
  digitalWrite(PIN_LEVEL_REF, LOW);

  pinMode(PIN_RELAY_SHOWER, OUTPUT);

  relayShower(false);

  for (int i = 0; i < NUM_PINS; i++)
  {
    // Transform all pins in GND
    // setGND(arr_pins[i], true);
    pinMode(arr_pins[i], INPUT_PULLUP);
  }
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, LOW);
}

int readLevelsPins(int arr_pins[])
{
  int cx1_status = B00000000; // Star with 0  14 = 0000 1111
  int level_max = 0;
  WebSerial.println("======Inicio=======");
  Serial.println("======Inicio=======");
  digitalWrite(PIN_LEVEL_REF, HIGH);
  delay(20);
  for (int i = NUM_PINS - 1; i >= 0; i--)
  {
    // setGND(arr_pins[i], false);
    int sta = digitalRead(arr_pins[i]);
    // setGND(arr_pins[i], true);
    // int status = sta;
    int status = sta ? 0 : 1;

    WebSerial.print("sta:");
    WebSerial.print(sta);
    Serial.print("sta:");
    Serial.print(sta);
    WebSerial.print(" i:");
    WebSerial.print(i);
    WebSerial.print(" s:");
    WebSerial.println(status);
    Serial.print(" s:");
    Serial.println(status);
    bitWrite(cx1_status, i, status);

    if (status == 1)
    {
      level_max = i;
      WebSerial.print(" level_max:");
      WebSerial.println(level_max);
      Serial.print(" level_max:");
      Serial.println(level_max);
      break;
    }
  }

  if (level_max > 0)
  {
    for (int i = level_max - 1; i >= 0; i--)
    {
      WebSerial.print("i:");
      WebSerial.println(i);
      Serial.print("i:");
      Serial.println(i);
      bitWrite(cx1_status, i, true);
    }
  }

  delay(20);
  digitalWrite(PIN_LEVEL_REF, LOW);
  WebSerial.println("======Fim==========");
  Serial.println("======Fim==========");

  return cx1_status;
}
#pragma endregion

#pragma region Utils

template <typename... Ts>
void wprintf(const char *format, Ts... ap)
{
  char *bla;
  // snprintf(buf,sizeof(buf),format,ap);
  asprintf(&bla, format, ap...);

  Serial.print(bla);
  WebSerial.print(bla);
}

#pragma endregion

#pragma region Endpoints

#include <ArduinoJson.h>

String levelToJson(int arr_pins[])
{
  int cx1_status = readLevelsPins(arr_pins);

  DynamicJsonDocument doc(1024);

  for (int i = NUM_PINS - 1; i >= 0; i--)
  {
    doc["cx1"][i] = bitRead(cx1_status, i);
  }

  String message = "";
  serializeJson(doc, message);
  return message;
}

void setupEndpoint(AsyncWebServer *server)
{

  server->on("/cx1", HTTP_GET, [](AsyncWebServerRequest *request)
             {
      String message = levelToJson(cx1_pins);
      WebSerial.println(message);
      request->send_P(200, F("application/json"), message.c_str()); });

  server->on("/shower", HTTP_GET, [](AsyncWebServerRequest *request)
             {
               DynamicJsonDocument doc(256);
               doc["status"] = false;

               if (request->hasParam("p"))
               {
                 AsyncWebParameter *p = request->getParam("p");
                 if (p->value().compareTo("1") == 0)
                 { // its equal
                   status_shower = 1;
                 }
                 else
                 {
                   status_shower = 0;
                 }
                 // delay(20);

                 doc["status"] = true;
                 doc["relay_shower"] = String(status_shower);
               }

               String message = "";
               serializeJson(doc, message);

               WebSerial.println(message);
               request->send_P(200, F("application/json"), message.c_str()); });

  // ws.onEvent(onWsEvent);
  // server->addHandler(&ws);
}

#pragma endregion

#pragma region Solar Controller

#define RXD2 16
#define TXD2 17
#define DEVICE_ID 1

#include <config.h>
#include <ModbusMaster.h>

ModbusMaster node;

#ifndef TRANSMIT_PERIOD
#define TRANSMIT_PERIOD 30000
#endif
unsigned long time_now = 0;

void debug_output()
{
#ifdef DEBUG
  // Output values to serial
  Serial.printf("\n\nTime:  20%02d-%02d-%02d   %02d:%02d:%02d   \n", rtc.r.y, rtc.r.M, rtc.r.d, rtc.r.h, rtc.r.m, rtc.r.s);

  Serial.print(F("\nLive-Data:           Volt        Amp       Watt  "));
  Serial.printf("\n  Panel:            %7.3f    %7.3f    %7.3f ", live.l.pV / 100.f, live.l.pI / 100.f, live.l.pP / 100.0f);
  Serial.printf("\n  Batt:             %7.3f    %7.3f    %7.3f ", live.l.bV / 100.f, live.l.bI / 100.f, live.l.bP / 100.0f);
  Serial.printf("\n  Load:             %7.3f    %7.3f    %7.3f \n", live.l.lV / 100.f, live.l.lI / 100.f, live.l.lP / 100.0f);
  Serial.printf("\n  Battery Current:  %7.3f  A ", batteryCurrent / 100.f);
  Serial.printf("\n  Battery SOC:      %7.0f  %% ", batterySOC / 1.0f);
  // Serial.printf( "\n  Load Switch:          %s   ",     (loadState==1?" On":"Off") );

  // Serial.print(F("\n\nStatistics:  "));
  // Serial.printf("\n  Panel:       min: %7.3f   max: %7.3f   V", stats.s.pVmin / 100.f, stats.s.pVmax / 100.f);
  // Serial.printf("\n  Battery:     min: %7.3f   max: %7.3f   V\n", stats.s.bVmin / 100.f, stats.s.bVmax / 100.f);

  // Serial.printf("\n  Consumed:    day: %7.3f   mon: %7.3f   year: %7.3f  total: %7.3f   kWh",
  //               stats.s.consEnerDay / 100.f, stats.s.consEnerMon / 100.f, stats.s.consEnerYear / 100.f, stats.s.consEnerTotal / 100.f);
  // Serial.printf("\n  Generated:   day: %7.3f   mon: %7.3f   year: %7.3f  total: %7.3f   kWh",
  //               stats.s.genEnerDay / 100.f, stats.s.genEnerMon / 100.f, stats.s.genEnerYear / 100.f, stats.s.genEnerTotal / 100.f);
  // Serial.printf("\n  CO2-Reduction:    %7.3f  t\n", stats.s.c02Reduction / 100.f);

  Serial.print(F("\nStatus:"));
  Serial.printf("\n    batt.volt:         %s   ", batt_volt_status[status_batt.volt]);
  Serial.printf("\n    batt.temp:         %s   ", batt_temp_status[status_batt.temp]);
  Serial.printf("\n    charger.charging:  %s   \n\n", charger_charging_status[charger_mode]);
#endif
}

void niceDelay(unsigned long delayTime)
{
  delay(delayTime);
}

void ReadValues()
{
  // clear old data
  //
  memset(rtc.buf, 0, sizeof(rtc.buf));
  memset(live.buf, 0, sizeof(live.buf));
  // memset(stats.buf, 0, sizeof(stats.buf));

  // Read registers for clock
  //
  niceDelay(50);
  node.clearResponseBuffer();
  uint8_t result = node.readHoldingRegisters(RTC_CLOCK, RTC_CLOCK_CNT);
  if (result == node.ku8MBSuccess)
  {

    rtc.buf[0] = node.getResponseBuffer(0);
    rtc.buf[1] = node.getResponseBuffer(1);
    rtc.buf[2] = node.getResponseBuffer(2);

    if (rtc.r.y == 14)
    {
      Serial.printf("\n\nDo Incorrect Date:  20%02d-%02d-%02d   %02d:%02d:%02d   \n", rtc.r.y, rtc.r.M, rtc.r.d, rtc.r.h, rtc.r.m, rtc.r.s);

      rtc.r.y = 22;
      rtc.r.M = 8;
      rtc.r.d = 21;
      rtc.r.h = 2;
      rtc.r.m = 8;
      rtc.r.s = 0;

      node.clearTransmitBuffer();
      node.send(rtc.buf[0]);
      node.send(rtc.buf[1]);
      node.send(rtc.buf[2]);
      uint8_t resultSend = node.writeMultipleRegisters(RTC_CLOCK, RTC_CLOCK_CNT);
      Serial.print(F("Result Write CLOCK"));
      Serial.println(resultSend, HEX);
    }
    else
    {
      Serial.printf("\n\nCorrect Date then:  20%02d-%02d-%02d   %02d:%02d:%02d   \n", rtc.r.y, rtc.r.M, rtc.r.d, rtc.r.h, rtc.r.m, rtc.r.s);
    }
  }
  else
  {
#ifdef DEBUG
    Serial.print(F("Miss read rtc-data, ret val:"));
    Serial.println(result, HEX);
#endif
  }
  if (result == 226)
    ErrorCounter++;

  // read LIVE-Data
  //
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(LIVE_DATA, LIVE_DATA_CNT);

  if (result == node.ku8MBSuccess)
  {

    for (uint8_t i = 0; i < LIVE_DATA_CNT; i++)
      live.buf[i] = node.getResponseBuffer(i);
  }
  else
  {
#ifdef DEBUG
    Serial.print(F("Miss read liva-data, ret val:"));
    Serial.println(result, HEX);
#endif
  }

  //   // Statistical Data
  //   niceDelay(50);
  //   node.clearResponseBuffer();
  //   result = node.readInputRegisters(STATISTICS, STATISTICS_CNT);

  //   if (result == node.ku8MBSuccess)
  //   {

  //     for (uint8_t i = 0; i < STATISTICS_CNT; i++)
  //       stats.buf[i] = node.getResponseBuffer(i);
  //   }
  //   else
  //   {
  // #ifdef DEBUG
  //     Serial.print(F("Miss read statistics, ret val:"));
  //     Serial.println(result, HEX);
  // #endif
  //   }

  // BATTERY_TYPE
  //   niceDelay(50);
  //   node.clearResponseBuffer();
  //   result = node.readInputRegisters(BATTERY_TYPE, 1);
  //   if (result == node.ku8MBSuccess)
  //   {

  //     // BatteryType = node.getResponseBuffer(0);
  // #ifdef DEBUG
  //     Serial.println(String(node.getResponseBuffer(0)));
  // #endif
  //   }
  //   else
  //   {
  // #ifdef DEBUG
  //     Serial.print(F("Miss read BATTERY_TYPE, ret val:"));
  //     Serial.println(result, HEX);
  // #endif
  //   }

  //   // EQ_CHARGE_VOLT
  //   niceDelay(50);
  //   node.clearResponseBuffer();
  //   result = node.readInputRegisters(EQ_CHARGE_VOLT, 1);
  //   if (result == node.ku8MBSuccess)
  //   {

  //     // EQChargeVoltValue = node.getResponseBuffer(0);
  // #ifdef DEBUG
  //     Serial.println(String(node.getResponseBuffer(0)));
  // #endif
  //   }
  //   else
  //   {
  // #ifdef DEBUG
  //     Serial.print(F("Miss read EQ_CHARGE_VOLT, ret val:"));
  //     Serial.println(result, HEX);
  // #endif
  //   }

  //   // CHARGING_LIMIT_VOLT
  //   niceDelay(50);
  //   node.clearResponseBuffer();
  //   result = node.readInputRegisters(CHARGING_LIMIT_VOLT, 1);
  //   if (result == node.ku8MBSuccess)
  //   {

  //     // ChargeLimitVolt = node.getResponseBuffer(0);
  // #ifdef DEBUG
  //     Serial.println(String(node.getResponseBuffer(0)));
  // #endif
  //   }
  //   else
  //   {
  // #ifdef DEBUG
  //     Serial.print(F("Miss read CHARGING_LIMIT_VOLT, ret val:"));
  //     Serial.println(result, HEX);
  // #endif
  //   }

  //   // Capacity
  //   niceDelay(50);
  //   node.clearResponseBuffer();
  //   result = node.readInputRegisters(BATTERY_CAPACITY, 1);
  //   if (result == node.ku8MBSuccess)
  //   {

  //     // BatteryCapactity = node.getResponseBuffer(0);
  // #ifdef DEBUG
  //     Serial.println(String(node.getResponseBuffer(0)));
  // #endif
  //   }
  //   else
  //   {
  // #ifdef DEBUG
  //     Serial.print(F("Miss read BATTERY_CAPACITY, ret val:"));
  //     Serial.println(result, HEX);
  // #endif
  //   }

  // Battery SOC
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(BATTERY_SOC, 1);
  if (result == node.ku8MBSuccess)
  {

    batterySOC = node.getResponseBuffer(0);
  }
  else
  {
#ifdef DEBUG
    Serial.print(F("Miss read batterySOC, ret val:"));
    Serial.println(result, HEX);
#endif
  }

  // Battery Net Current = Icharge - Iload
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(BATTERY_CURRENT_L, 2);
  if (result == node.ku8MBSuccess)
  {

    batteryCurrent = node.getResponseBuffer(0);
    batteryCurrent |= node.getResponseBuffer(1) << 16;
    // #ifdef DEBUG
    // Serial.println(String(batteryCurrent));
    // #endif
  }
  else
  {
#ifdef DEBUG
    Serial.print(F("Miss read batteryCurrent, ret val:"));
    Serial.println(result, HEX);
#endif
  }

  //   if (!switch_load) {
  //     // State of the Load Switch
  //     niceDelay(50);
  //     node.clearResponseBuffer();
  //     result = node.readCoils(  LOAD_STATE, 1 );
  //     if (result == node.ku8MBSuccess)  {

  //       loadState = node.getResponseBuffer(0);

  //     } else  {
  // #ifdef DEBUG
  //       Serial.print(F("Miss read loadState, ret val:"));
  //       Serial.println(result, HEX);
  //  #endif
  //     }
  //   }

  // Read Model
  //   niceDelay(50);
  //   node.clearResponseBuffer();
  //   result = node.readInputRegisters(CCMODEL, 1);
  //   if (result == node.ku8MBSuccess)
  //   {

  //     // CCModel = node.getResponseBuffer(0);
  //   }
  //   else
  //   {
  // #ifdef DEBUG
  //     Serial.print(F("Miss read Model, ret val:"));
  //     Serial.println(result, HEX);
  // #endif
  //   }

  // Read Status Flags
  niceDelay(50);
  node.clearResponseBuffer();
  result = node.readInputRegisters(0x3200, 2);
  if (result == node.ku8MBSuccess)
  {

    uint16_t temp = node.getResponseBuffer(0);
#ifdef DEBUG
    Serial.print(F("Batt Flags : "));
    Serial.println(temp);
#endif

    status_batt.volt = temp & 0b1111;
    status_batt.temp = (temp >> 4) & 0b1111;
    status_batt.resistance = (temp >> 8) & 0b1;
    status_batt.rated_volt = (temp >> 15) & 0b1;

    temp = node.getResponseBuffer(1);
#ifdef DEBUG
    Serial.print(F("Chrg Flags : "));
    Serial.println(temp, HEX);
#endif

    charger_mode = (temp & 0b0000000000001100) >> 2;
#ifdef DEBUG
    Serial.print(F("charger_mode  : "));
    Serial.println(charger_mode);
#endif
  }
  else
  {
#ifdef DEBUG
    Serial.print(F("Miss read ChargeState, ret val:"));
    Serial.println(result, HEX);
#endif
  }
}

#pragma endregion
// =======================================================================

// That region must be in the end.
#pragma region Setup and Loop
void setup()
{
  // set led pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  setupPins(cx1_pins);

  Serial.begin(115200);

  while (!Serial)
    ;

  delay(200);

  Serial.println("mac ssid: " + ssid);

  if (IS_TEST)
  {

    Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);
    // init modbus in receive mode
    // pinMode(MAX485_RE, OUTPUT);
    // pinMode(MAX485_DE, OUTPUT);
    // postTransmission();

    // EPEver Device ID and Baud Rate
    node.begin(DEVICE_ID, Serial1);
    // modbus callbacks
    // node.preTransmission(preTransmission);
    // node.postTransmission(postTransmission);
  }

#pragma region WifiConfigs
  Serial.print(F("\nStarting Async_ESP32_FSWebServer_DRD using "));
  Serial.print(FS_Name);
  Serial.print(F(" on "));
  Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION);
  // Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);

#if defined(ESP_ASYNC_WIFIMANAGER_VERSION_INT)
  if (ESP_ASYNC_WIFIMANAGER_VERSION_INT < ESP_ASYNC_WIFIMANAGER_VERSION_MIN)
  {
    Serial.print("Warning. Must use this example on Version later than : ");
    Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET);
  }
#endif

  Serial.setDebugOutput(false);

  if (FORMAT_FILESYSTEM)
    FileFS.format();

  // Format FileFS if not yet
  if (!FileFS.begin(true))
  {
    Serial.println(F("SPIFFS/LittleFS failed! Already tried formatting."));

    if (!FileFS.begin())
    {
      // prevents debug info from the library to hide err message.
      delay(100);

#if USE_LITTLEFS
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));
#else
      Serial.println(F("SPIFFS failed!. Please use LittleFS or EEPROM. Stay forever"));
#endif

      while (true)
      {
        delay(1);
      }
    }
  }

  File root = FileFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    String fileName = file.name();
    size_t fileSize = file.size();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    file = root.openNextFile();
  }

  Serial.println();

  // drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  // if (!drd)
  //   Serial.println(F("Can't instantiate. Disable DRD feature"));

  unsigned long startedAt = millis();

  // New in v1.4.0
  initAPIPConfigStruct(WM_AP_IPconfig);
  initSTAIPConfigStruct(WM_STA_IPconfig);
  //////

  digitalWrite(LED_BUILTIN, LED_ON);

  // Local intialization. Once its business is done, there is no need to keep it around
  //  Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  // ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer);
  //  Use this to personalize DHCP hostname (RFC952 conformed)0
#if (USING_ESP32_S2 || USING_ESP32_C3)
  ESPAsync_WiFiManager ESPAsync_wifiManager(&server, NULL, "AsyncESP32-FSWebServer");
#else
  DNSServer dnsServer;

  ESPAsync_WiFiManager ESPAsync_wifiManager(&server, &dnsServer, "kombi");
#endif

#if USE_CUSTOM_AP_IP
  // set custom ip for portal
  //  New in v1.4.0
  ESPAsync_wifiManager.setAPStaticIPConfig(WM_AP_IPconfig);
  //////
#endif

  ESPAsync_wifiManager.setMinimumSignalQuality(-1);

  // Set config portal channel, default = 1. Use 0 => random channel from 1-13
  ESPAsync_wifiManager.setConfigPortalChannel(0);
  //////

#if !USE_DHCP_IP
  // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway, Subnetmask). New in v1.0.5
  // New in v1.4.0
  ESPAsync_wifiManager.setSTAStaticIPConfig(WM_STA_IPconfig);
  //////
#endif

  // New from v1.1.1
#if USING_CORS_FEATURE
  ESPAsync_wifiManager.setCORSHeader("Access-Control-Allow-Origin: *");
#endif

  // We can't use WiFi.SSID() in ESP32as it's only valid after connected.
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
  // Have to create a new function to store in EEPROM/SPIFFS for this purpose
  Router_SSID = ESPAsync_wifiManager.WiFi_SSID();
  Router_Pass = ESPAsync_wifiManager.WiFi_Pass();

  // Remove this line if you do not want to see WiFi password printed
  Serial.println("ESP Self-Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  // SSID to uppercase
  ssid.toUpperCase();
  password = ssid;

  bool configDataLoaded = false;

  // From v1.1.0, Don't permit NULL password
  if ((Router_SSID != "") && (Router_Pass != ""))
  {
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());

    ESPAsync_wifiManager.setConfigPortalTimeout(120); // If no access point name has been previously entered disable timeout.
    Serial.println(F("Got ESP Self-Stored Credentials. Timeout 120s for Config Portal"));
  }

  if (loadConfigData())
  {
    configDataLoaded = true;

    ESPAsync_wifiManager.setConfigPortalTimeout(120); // If no access point name has been previously entered disable timeout.
    Serial.println(F("Got stored Credentials. Timeout 120s for Config Portal"));

#if USE_ESP_WIFIMANAGER_NTP
    if (strlen(WM_config.TZ_Name) > 0)
    {
      LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

#if ESP8266
      configTime(WM_config.TZ, "pool.ntp.org");
#else
      // configTzTime(WM_config.TZ, "pool.ntp.org" );
      configTzTime(WM_config.TZ, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
#endif
    }
    else
    {
      Serial.println(F("Current Timezone is not set. Enter Config Portal to set."));
    }
#endif
  }
  else
  {
    // Enter CP only if no stored SSID on flash and file
    Serial.println(F("Open Config Portal without Timeout: No stored Credentials."));
    initialConfig = true;
  }

  // if (drd->detectDoubleReset())
  // {
  //   // DRD, disable timeout.
  //   ESPAsync_wifiManager.setConfigPortalTimeout(0);

  //   Serial.println(F("Open Config Portal without Timeout: Double Reset Detected"));
  //   initialConfig = true;
  // }

  if (initialConfig)
  {
    Serial.print(F("Starting configuration portal @ "));

#if USE_CUSTOM_AP_IP
    Serial.print(APStaticIP);
#else
    Serial.print(F("192.168.4.1"));
#endif

    Serial.print(F(", SSID = "));
    Serial.print(ssid);
    Serial.print(F(", PWD = "));
    Serial.println(password);

    // Starts an access point
    if (!ESPAsync_wifiManager.startConfigPortal((const char *)ssid.c_str(), password.c_str()))
      Serial.println(F("Not connected to WiFi but continuing anyway."));
    else
    {
      Serial.println(F("WiFi connected...yeey :)"));
    }

    // Stored  for later usage, from v1.1.0, but clear first
    memset(&WM_config, 0, sizeof(WM_config));

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      String tempSSID = ESPAsync_wifiManager.getSSID(i);
      String tempPW = ESPAsync_wifiManager.getPW(i);

      if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE))
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw);
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

#if USE_ESP_WIFIMANAGER_NTP
    String tempTZ = ESPAsync_wifiManager.getTimezoneName();

    if (strlen(tempTZ.c_str()) < sizeof(WM_config.TZ_Name) - 1)
      strcpy(WM_config.TZ_Name, tempTZ.c_str());
    else
      strncpy(WM_config.TZ_Name, tempTZ.c_str(), sizeof(WM_config.TZ_Name) - 1);

    const char *TZ_Result = ESPAsync_wifiManager.getTZ(WM_config.TZ_Name);

    if (strlen(TZ_Result) < sizeof(WM_config.TZ) - 1)
      strcpy(WM_config.TZ, TZ_Result);
    else
      strncpy(WM_config.TZ, TZ_Result, sizeof(WM_config.TZ_Name) - 1);

    if (strlen(WM_config.TZ_Name) > 0)
    {
      LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

#if ESP8266
      configTime(WM_config.TZ, "pool.ntp.org");
#else
      // configTzTime(WM_config.TZ, "pool.ntp.org" );
      configTzTime(WM_config.TZ, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
#endif
    }
    else
    {
      LOGERROR(F("Current Timezone Name is not set. Enter Config Portal to set."));
    }
#endif

    // New in v1.4.0
    ESPAsync_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);
    //////

    saveConfigData();
  }

  startedAt = millis();

  if (!initialConfig)
  {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    if (!configDataLoaded)
      loadConfigData();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE))
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw);
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println(F("ConnectMultiWiFi in setup"));

      connectMultiWiFi();
    }
  }

  Serial.print(F("After waiting "));
  Serial.print((float)(millis() - startedAt) / 1000);
  Serial.print(F(" secs more in setup(), connection result is "));

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(WiFi.localIP());
  }
  else
    Serial.println(ESPAsync_wifiManager.getStatus(WiFi.status()));

  if (!MDNS.begin(host.c_str()))
  {
    Serial.println(F("Error starting MDNS responder!"));
  }
#pragma endregion

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", HTTP_PORT);

  // SERVER INIT
  // events.onConnect([](AsyncEventSourceClient *client)
  //                  { client->send("", NULL, millis(), 1000); });
  //                   // Handle Web Server Events

  events.onConnect([](AsyncEventSourceClient *client)
                   {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("oie", NULL, millis(), 10000); });

  server.addHandler(&events);

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(ESP.getFreeHeap())); });

  server.addHandler(new SPIFFSEditor(FileFS, http_username, http_password));
  server.serveStatic("/", FileFS, "/").setDefaultFile("index.html");

  setupEndpoint(&server);

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    Serial.print(F("NOT_FOUND: "));
    
    if (request->method() == HTTP_GET)
      Serial.print(F("GET"));
    else if (request->method() == HTTP_POST)
      Serial.print(F("POST"));
    else if (request->method() == HTTP_DELETE)
      Serial.print(F("DELETE"));
    else if (request->method() == HTTP_PUT)
      Serial.print(F("PUT"));
    else if (request->method() == HTTP_PATCH)
      Serial.print(F("PATCH"));
    else if (request->method() == HTTP_HEAD)
      Serial.print(F("HEAD"));
    else if (request->method() == HTTP_OPTIONS)
      Serial.print(F("OPTIONS"));
    else
      Serial.print(F("UNKNOWN"));
      
    Serial.println(" http://" + request->host() + request->url());

    if (request->contentLength())
    {
      Serial.println("_CONTENT_TYPE: " + request->contentType());
      Serial.println("_CONTENT_LENGTH: " + request->contentLength());
    }

    int headers = request->headers();
    int i;

    for (i = 0; i < headers; i++)
    {
      AsyncWebHeader* h = request->getHeader(i);
      Serial.println("_HEADER[" + h->name() + "]: " + h->value());
    }

    int params = request->params();

    for (i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);

      if (p->isFile())
      {
        Serial.println("_FILE[" + p->name() + "]: " + p->value() + ", size: " + p->size());
      }
      else if (p->isPost())
      {
        Serial.println("_POST[" + p->name() + "]: " + p->value());
      }
      else
      {
        Serial.println("_GET[" + p->name() + "]: " + p->value());
      }
    }

    request->send(404); });

  server.onFileUpload([](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
                      {
    (void) request;
    
    if (!index)
      Serial.println("UploadStart: " + filename);

    Serial.print((const char*)data);

    if (final)
      Serial.println("UploadEnd: " + filename + "(" + String(index + len) + ")" ); });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                       {
    (void) request;
    
    if (!index)
      Serial.println("BodyStart: " + total);

    Serial.print((const char*)data);

    if (index + len == total)
      Serial.println("BodyEnd: " + total); });

  AsyncElegantOTA.begin(&server, http_username.c_str(), http_password.c_str()); // Start AsyncElegantOTA

  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);

  server.begin();

  //////

  Serial.print(F("HTTP server started @ "));
  Serial.println(WiFi.localIP());

  Serial.println(separatorLine);
  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/edit to see the file browser");
  Serial.println("Using username = " + http_username + " and password = " + http_password);
  Serial.println(separatorLine);

  digitalWrite(LED_BUILTIN, LED_OFF);
}

unsigned long SHOWER_INTERVAL = 250;
unsigned long CONTROLLER_INTERVAL = 30000;

static ulong check_water_timeout = 0;
static ulong shower_timeout = 0;
static ulong controller_timeout = 0;

static ulong current_millis_loop;

void loop()
{
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  // if (drd)
  // drd->loop();

  check_status();

  current_millis_loop = millis();

  if ((current_millis_loop > shower_timeout) || (shower_timeout == 0))
  {
    if (old_status_shower != status_shower)
    {
      relayShower(status_shower == 1);
      old_status_shower = status_shower;
    }

    events.send(String(status_shower).c_str(), "shower_status", millis());
    shower_timeout = current_millis_loop + SHOWER_INTERVAL;
  }

  if ((current_millis_loop > check_water_timeout) || (check_water_timeout == 0))
  {

    String message = levelToJson(cx1_pins);
    // Send Events to the Web Server with the Sensor Readings
    events.send("ping", NULL, millis());
    events.send(message.c_str(), "shower_box", millis());

    check_water_timeout = current_millis_loop + INTERVAL_READ_PINS;
  }

  if (IS_TEST)
  {

    if ((current_millis_loop > controller_timeout) || (controller_timeout == 0))
    {

      ReadValues();

            debug_output();

      Serial.println("Start sending...");
      DynamicJsonDocument doc(1024);
      char *time = NULL;
      asprintf(&time, "20%02d-%02d-%02d   %02d:%02d:%02d", rtc.r.y, rtc.r.M, rtc.r.d, rtc.r.h, rtc.r.m, rtc.r.s);
      doc["time"] = time;

      // Serial.print(F("\nLive-Data:           Volt        Amp       Watt  "));
      // Serial.printf("\n  Panel:            %7.3f    %7.3f    %7.3f ", live.l.pV / 100.f, live.l.pI / 100.f, live.l.pP / 100.0f);
      // Serial.printf("\n  Batt:             %7.3f    %7.3f    %7.3f ", live.l.bV / 100.f, live.l.bI / 100.f, live.l.bP / 100.0f);
      // Serial.printf("\n  Load:             %7.3f    %7.3f    %7.3f \n", live.l.lV / 100.f, live.l.lI / 100.f, live.l.lP / 100.0f);
      // Serial.printf("\n  Battery Current:  %7.3f  A ", batteryCurrent / 100.f);
      // Serial.printf("\n  Battery SOC:      %7.0f  %% ", batterySOC / 1.0f);

      String message = "";
      serializeJson(doc, message);

      // free(time);
      events.send(message.c_str(), "controller_status", millis());

      controller_timeout = current_millis_loop + CONTROLLER_INTERVAL;
      // Read Values from Charge Controller



      Serial.print(F("Error count = "));
      Serial.println(ErrorCounter);

      if (ErrorCounter > 5)
      {
        // init modbus in receive mode
        // pinMode(MAX485_RE, OUTPUT);
        // pinMode(MAX485_DE, OUTPUT);
        // postTransmission();

        // EPEver Device ID and Baud Rate
        node.begin(DEVICE_ID, Serial1);

        // modbus callbacks
        // node.preTransmission(preTransmission);
        // node.postTransmission(postTransmission);
        ErrorCounter = 0;
      }

      // power down MAX485_DE
      // postTransmission()/;
    }
  }
}
#pragma endregion

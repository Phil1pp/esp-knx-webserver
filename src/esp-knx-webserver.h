#pragma once

#include <Arduino.h>
#include <ArduinoOTA.h>
#if defined(ESP32)
    #pragma message "Building KnxWebserver for ESP32"
    #include <WebServer.h>
#elif defined(ESP8266)
    #pragma message "Building KnxWebserver for ESP8266"
    #include <ESP8266WebServer.h>
#else
    #error "Wrong hardware. Not ESP8266 or ESP32"
#endif

typedef enum __knxModeOptions {
    KNX_MODE_OFF    = 0,
    KNX_MODE_NORMAL = 1,
    KNX_MODE_PROG   = 2,
} knxModeOptions_t;

typedef void callbackSetKnxMode(knxModeOptions_t mode);
typedef knxModeOptions_t callbackGetKnxMode();

class KnxWebserver
{
public:
    void startWeb(const char* username, const char* password);
    void startOta();
    void endOta();
    void setHostname(String newName);
    void setKnxDetail(String physAddr, bool configOk);
    void loop();

    void registerSetKnxModeCallback(callbackSetKnxMode* fctn);
    void registerGetKnxModeCallback(callbackGetKnxMode* fctn);

private:
    #if defined(ESP32)
        WebServer *server;
    #elif defined(ESP8266)
        ESP8266WebServer *server;
    #endif
    String hostname = "ESP-KNX-Device";
    String knxPhysAddr = "0.0.0";
    bool knxConfigOk = false;
    const char* username;
    const char* password;
    bool authRequired = false;
    bool otaActive = false;
    bool otaIntialized = false;
    unsigned long otaStartTime = 0;

    void handleRoot();
    void handleProgMode();
    void handleNormalMode();
    void handleKnxOff();
    void handleOtaOn();
    void handleOtaOff();
    void handleNotFound();
    void otaSetup();
    int getRSSIasQuality(int RSSI);

    callbackSetKnxMode* setKnxModeFctn;
    callbackGetKnxMode* getKnxModeFctn;
};

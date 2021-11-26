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

typedef void callbackSetProgmode(bool);
typedef bool callbackGetProgmode();

class KnxWebserver
{
public:
    void startWeb(const char* username, const char* password);
    void startOta();
    void endOta();
    void setHostname(String newName);
    void setKnxDetail(String physAddr, bool configOk);
    void loop();

    void registerSetProgmodeCallback(callbackSetProgmode* fctn);
    void registerGetProgmodeCallback(callbackGetProgmode* fctn);

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
    void handleProgmodeOn();
    void handleProgmodeOff();
    void handleNotFound();
    void handleOtaOn();
    void handleOtaOff();
    void otaSetup();
    int getRSSIasQuality(int RSSI);

    callbackSetProgmode* setProgmodeFctn;
    callbackGetProgmode* getProgmodeFctn;
};

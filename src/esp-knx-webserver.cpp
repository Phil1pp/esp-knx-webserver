#include "esp-knx-webserver.h"

void KnxWebserver::startWeb(const char *www_username, const char *www_password)
{
#if defined(ESP32)
    server = new WebServer(80);
#elif defined(ESP8266)
    server = new ESP8266WebServer(80);
#endif
    username = www_username;
    password = www_password;
    authRequired = username != nullptr && username[0] != 0;

    server->on("/", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleRoot(); });
    server->on("/progmode", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleProgMode(); });
    server->on("/normalmode", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleNormalMode(); });
    server->on("/knxoff", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleKnxOff(); });
    server->on("/otaon", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleOtaOn(); });
    server->on("/otaoff", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleOtaOff(); });
    server->on("/restart", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleRestart(); });
    server->on("/tftupdate", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleTftUpdate(); });
    server->on("/tftdebug", [this]()
               { if (authRequired && !server->authenticate(username, password)) { return server->requestAuthentication(); } handleTftDebug(); });
    server->on("/favicon.ico", [this]()
               { server->send_P(200, "image/png", favicon, sizeof(favicon)); });
    server->onNotFound([this]()
                       { handleNotFound(); });
    server->begin();
}

void KnxWebserver::loop()
{
    if (otaActive)
    {
        ArduinoOTA.handle();
        // Disable OTA 5 minutes after startup to reduce WIFI load
        if (millis() - otaStartTime > 5 * 60 * 1000UL)
        {
            endOta();
        }
    }
    server->handleClient();
}

void KnxWebserver::setHostname(String newName)
{
    hostname = newName;
    ArduinoOTA.setHostname(hostname.c_str());
}

void KnxWebserver::setKnxDetail(String physAddr, bool configOk)
{
    knxPhysAddr = physAddr;
    knxConfigOk = configOk;
}

void KnxWebserver::registerSetKnxModeCallback(callbackSetKnxMode *fctn)
{
    setKnxModeFctn = fctn;
}

void KnxWebserver::registerGetKnxModeCallback(callbackGetKnxMode *fctn)
{
    getKnxModeFctn = fctn;
}

void KnxWebserver::registerTftUpdateCallback(callbackStartTftUpdate *fctn)
{
    startTftUpdateFctn = fctn;
}

void KnxWebserver::registerTftDebugCallback(callbackStartTftDebug *fctn)
{
    startTftDebugFctn = fctn;
}

void KnxWebserver::handleRoot()
{
    String msg = "<!DOCTYPE html><html>\n";
    msg += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    msg += "<title>" + hostname + "</title>\n";
    msg += "<style>html {font-family: Helvetica; display: inline-block; color: #444444; text-align: center;}\n";
    msg += "h1 {margin: 50px auto 30px;}\n";
    msg += ".button {display: inline-block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px 5px 35px 5px;cursor: pointer;border-radius: 4px;}\n";
    msg += ".button-blue {background-color: #3498db; cursor: not-allowed ;}\n";
    msg += ".button-dark {background-color: #34495e;}\n";
    msg += ".button-dark:active {background-color: #2c3e50;}\n";
    msg += ".warning {color: #a93226;}\n";
    msg += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
    msg += "</style>\n";
    msg += "</head>\n";
    msg += "<body>\n";
    msg += "<h1>ESP based KNX device</h1>";
    msg += "<h3>Name: " + hostname + "</h3>";
    msg += "<h3>Physical address: " + knxPhysAddr + "</h3>\n";
    if (!knxConfigOk)
    {
        msg += "<h3 class=\"warning\">KNX configuration incomplete!</h3>\n";
    }

    if (getKnxModeFctn != nullptr)
    {
        msg += "<p>KNX Mode:</p>";
        switch (getKnxModeFctn())
        {
        case KNX_MODE_OFF:
            msg += "<a class=\"button button-dark\" href=\"/progmode\">PROG</a><a class=\"button button-dark\" href=\"/normalmode\">Normal</a><a class=\"button button-blue\">OFF</a>\n";
            break;
        case KNX_MODE_NORMAL:
            msg += "<a class=\"button button-dark\" href=\"/progmode\">PROG</a><a class=\"button button-blue\">Normal</a><a class=\"button button-dark\" href=\"/knxoff\">OFF</a>\n";
            break;
        case KNX_MODE_PROG:
            msg += "<a class=\"button button-blue\">PROG</a><a class=\"button button-dark\" href=\"/normalmode\">Normal</a><a class=\"button button-dark\" href=\"/knxoff\">OFF</a>\n";
            break;
        }
    }

    if (otaActive)
    {
        int remainingTime = 5 * 60 - (millis() - otaStartTime) / 1000;
        msg += "<script>var t=" + String(remainingTime) + ";var x=setInterval(function(){var m=Math.floor(t/60);var s=t%60;document.getElementById(\"timer\").innerHTML=m+\"m \"+s+\"s\";t--;if(t<0){clearInterval(x);location.reload();}},1000);</script>";
        msg += "<p>OTA: <span id=\"timer\"></span></p><a class=\"button button-blue\">ON</a><a class=\"button button-dark\" href=\"/otaoff\">OFF</a>\n";
    }
    else
    {
        msg += "<p>OTA:</p><a class=\"button button-dark\" href=\"/otaon\">ON</a><a class=\"button button-blue\">OFF</a>\n";
    }

    msg += "<p>System:</p><a class=\"button button-dark\" href=\"/restart\">Restart</a>";

    if (startTftUpdateFctn != nullptr)
    {
        msg += "<a class=\"button button-dark\" href=\"/tftupdate\">TFT Update</a>";
    }

    if (startTftDebugFctn != nullptr)
    {
        msg += "<a class=\"button button-dark\" href=\"/tftdebug\">TFT Debug</a>";
    }

    if (authRequired)
    {
        msg += "<a class=\"button button-dark\" onclick=\"window.open('http://logout@'+window.location.host,'_self');\">Logout</a>";
    }
    msg += "\n";

#if defined(ESP32)
    char strBuffer[50];
    msg += "<h3>ESP32 Chip Info</h3>";
    sprintf(strBuffer, "<p>Flash size: %.1gMB<br>", spi_flash_get_chip_size() / 1024.0 / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "PSRAM size: %.1gMB<br>", ESP.getPsramSize() / 1024.0 / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "Free PSRAM: %.1gMB<br>", ESP.getFreePsram() / 1024.0 / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "Heap size: %.3gKB<br>", ESP.getHeapSize() / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "Free heap: %.3gKB<br>", ESP.getFreeHeap() / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "Chip temperature: %.1f&deg;C<br>", temperatureRead());
    msg += String(strBuffer);
    sprintf(strBuffer, "CPU frequency: %dMHz<br>", ESP.getCpuFreqMHz());
    msg += String(strBuffer);
    msg += "WIFI MAC: " + String(WiFi.macAddress()) + "<br>";
    sprintf(strBuffer, "WIFI Signal: %d&percnt;</p>", getRSSIasQuality(WiFi.RSSI()));
    msg += String(strBuffer);
#elif defined(ESP8266)
    char strBuffer[100];
    msg += "<h3>ESP8266 Chip Info</h3>";
    sprintf(strBuffer, "<p>Flash size: %.1gMB<br>", ESP.getFlashChipRealSize() / 1024.0 / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "Free heap: %.3gKB<br>", ESP.getFreeHeap() / 1024.0);
    msg += String(strBuffer);
    sprintf(strBuffer, "CPU frequency: %dMHz<br>", ESP.getCpuFreqMHz());
    msg += String(strBuffer);
    msg += "WIFI MAC: " + String(WiFi.macAddress()) + "<br>";
    sprintf(strBuffer, "WIFI Signal: %d&percnt;<br>", getRSSIasQuality(WiFi.RSSI()));
    msg += String(strBuffer);
    msg += "Last restart reason: " + ESP.getResetInfo() + "</p>";
#endif
    msg += "</body>\n";
    msg += "</html>\n";
    server->send(200, "text/html", msg);
}

void KnxWebserver::handleProgMode()
{
    if (setKnxModeFctn != nullptr)
    {
        setKnxModeFctn(KNX_MODE_PROG);
    }
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
}

void KnxWebserver::handleNormalMode()
{
    if (setKnxModeFctn != nullptr)
    {
        setKnxModeFctn(KNX_MODE_NORMAL);
    }
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
}

void KnxWebserver::handleKnxOff()
{
    if (setKnxModeFctn != nullptr)
    {
        setKnxModeFctn(KNX_MODE_OFF);
    }
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
}

void KnxWebserver::handleOtaOn()
{
    startOta();
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
}

void KnxWebserver::handleOtaOff()
{
    endOta();
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
}

void KnxWebserver::handleRestart()
{
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
    server->handleClient();
    ESP.restart();
}

void KnxWebserver::handleTftUpdate()
{
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
    if (startTftUpdateFctn != nullptr)
    {
        startTftUpdateFctn();
    }
}

void KnxWebserver::handleTftDebug()
{
    server->sendHeader("Location", String("/"), true);
    server->send(302, "text/plain", "");
    if (startTftDebugFctn != nullptr)
    {
        startTftDebugFctn();
    }
}

void KnxWebserver::handleNotFound()
{
    server->send(404);
}

void KnxWebserver::otaSetup()
{
    if (!otaIntialized)
    {
        ArduinoOTA.onStart([]()
                           { Serial.println("Start"); });
        ArduinoOTA.onEnd([]()
                         { Serial.println("\nEnd"); });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                              { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
        ArduinoOTA.onError([](ota_error_t error)
                           {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

        otaIntialized = true;
    }
}

void KnxWebserver::startOta()
{
    otaSetup();

    otaActive = true;
    otaStartTime = millis();
    ArduinoOTA.begin();
}

void KnxWebserver::endOta()
{
    otaActive = false;
    ArduinoOTA.end();
}

int KnxWebserver::getRSSIasQuality(int RSSI)
{
    int quality = 0;

    if (RSSI <= -100)
    {
        quality = 0;
    }
    else if (RSSI >= -50)
    {
        quality = 100;
    }
    else
    {
        quality = 2 * (RSSI + 100);
    }
    return quality;
}
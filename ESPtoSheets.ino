#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define PROJECT_ID ""
#define CLIENT_EMAIL ""

const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\n\n-----END PRIVATE KEY-----\n";
const char spreadsheetId[] = "";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000; // 5 seconds (or more)
unsigned long lastPrintTime = 0;
unsigned long printInterval = 5000;

void tokenStatusCallback(TokenInfo info);
const char *ntpServer = "pool.ntp.org";

unsigned long getTime()
{
    time_t now;
    time(&now);
    return now;
}

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    configTime(0, 0, ntpServer);
    delay(5000);

    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop()
{
    bool ready = GSheet.ready();

    if (millis() - lastPrintTime > printInterval)
    {
        lastPrintTime = millis();
        unsigned long epochTime = getTime();
        int randomValue = random(1, 101);
        Serial.printf("Time: %lu, Random Number: %d\n", epochTime, randomValue);
    }

    if (ready && millis() - lastTime > timerDelay)
    {
        lastTime = millis();
        FirebaseJson response;

        Serial.println("\nAppending data to Google Sheets...");

        FirebaseJson valueRange;
        unsigned long epochTime = getTime();
        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", epochTime);

        bool success = GSheet.values.append(&response, spreadsheetId, "Sheet1!A:A", &valueRange);
        if (success)
        {
            response.toString(Serial, true);
            valueRange.clear();
        }
        else
        {
            Serial.println(GSheet.errorReason());
        }
        Serial.println();
    }
}

void tokenStatusCallback(TokenInfo info)
{
    Serial.printf("Token status: %s\n", GSheet.getTokenStatus(info).c_str());
}

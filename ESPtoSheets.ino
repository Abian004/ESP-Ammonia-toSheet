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
unsigned long timerDelay = 5000; // 5 seconds
unsigned long lastPrintTime = 0;
unsigned long printInterval = 5000;

void tokenStatusCallback(TokenInfo info);
const char *ntpServer = "pool.ntp.org";

// Function to get formatted time as "YYYYMMDD-HH:MM:SS" in GMT+7
String getFormattedTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "N/A"; // Return "N/A" if time sync fails
    }
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d-%H:%M:%S", &timeinfo);
    return String(buffer);
}

void setup()
{
    Serial.begin(115200);
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

    configTime(25200, 0, ntpServer); // GMT+7
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
        String formattedTime = getFormattedTime();
        int randomValue = random(1, 101);
        Serial.printf("Time: %s, Random Number: %d\n", formattedTime.c_str(), randomValue);
    }

    if (ready && millis() - lastTime > timerDelay)
    {
        lastTime = millis();
        FirebaseJson response;

        Serial.println("\nAppending data to Google Sheets...");

        FirebaseJson valueRange;
        String formattedTime = getFormattedTime();
        int randomValue = random(1, 101);

        valueRange.add("majorDimension", "ROWS");
        valueRange.set("values/[0]/[0]", formattedTime); // Column A
        valueRange.set("values/[0]/[1]", randomValue);   // Column B

        bool success = GSheet.values.append(&response, spreadsheetId, "Sheet1!A:B", &valueRange);
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
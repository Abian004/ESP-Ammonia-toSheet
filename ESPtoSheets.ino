#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>

// WiFi and Google Sheet credentials
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define PROJECT_ID ""
#define CLIENT_EMAIL ""

const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\n\n-----END PRIVATE KEY-----\n";

const char spreadsheetId[] = "";

// MQ2 Sensor definitions
#define MQ2_ANALOG_PIN A0
#define RL_VALUE 5
#define RO_CLEAN_AIR 9.83

// Calibration curves
float LPGCurve[3] = {2.3, 0.21, -0.47};
float COCurve[3] = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};

float Ro = 10;

// Timing variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // Send data every 10 seconds
const char *ntpServer = "pool.ntp.org";

void tokenStatusCallback(TokenInfo info);

// Get formatted time function
String getFormattedTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "N/A";
    }
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d-%H:%M:%S", &timeinfo);
    return String(buffer);
}

void setup()
{
    Serial.begin(115200);
    pinMode(MQ2_ANALOG_PIN, INPUT);

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

    // Initialize time
    configTime(25200, 0, ntpServer); // GMT+7

    // Initialize Google Sheets
    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    // Warm up and calibrate MQ2
    Serial.println("MQ2 warming up...");
    delay(20000);
    Ro = calibrateSensor();
    Serial.println("Ro = " + String(Ro));
}

void loop()
{
    if (GSheet.ready() && millis() - lastTime > timerDelay)
    {
        lastTime = millis();

        // Read sensor values
        float sensorValue = analogRead(MQ2_ANALOG_PIN);
        float rs = ((1024.0 / sensorValue) - 1.0) * RL_VALUE;
        float ratio = rs / Ro;

        // Calculate gas concentrations
        float lpg = calculatePPM(ratio, LPGCurve);
        float co = calculatePPM(ratio, COCurve);
        float smoke = calculatePPM(ratio, SmokeCurve);

        // Prepare data for Google Sheets
        FirebaseJson response;
        FirebaseJson valueRange;
        String formattedTime = getFormattedTime();

        valueRange.add("majorDimension", "ROWS");
        valueRange.set("values/[0]/[0]", formattedTime); // Time
        valueRange.set("values/[0]/[1]", lpg);           // LPG
        valueRange.set("values/[0]/[2]", co);            // CO
        valueRange.set("values/[0]/[3]", smoke);         // Smoke

        // Send to Google Sheets
        Serial.println("\nSending data to Google Sheets...");
        bool success = GSheet.values.append(&response, spreadsheetId, "Sheet1!A:D", &valueRange);

        if (success)
        {
            Serial.println("Data sent successfully!");
            Serial.printf("Time: %s\nLPG: %.2f ppm\nCO: %.2f ppm\nSmoke: %.2f ppm\n",
                          formattedTime.c_str(), lpg, co, smoke);
        }
        else
        {
            Serial.println("Failed to send data: " + String(GSheet.errorReason()));
        }
    }
}

float calibrateSensor()
{
    float val = 0;
    for (int i = 0; i < 100; i++)
    {
        val += analogRead(MQ2_ANALOG_PIN);
        delay(10);
    }
    val = val / 100.0;
    return ((1024.0 / val) - 1.0) * RL_VALUE / RO_CLEAN_AIR;
}

float calculatePPM(float ratio, float *curve)
{
    return pow(10, ((log10(ratio) - curve[1]) / curve[2]) + curve[0]);
}

void tokenStatusCallback(TokenInfo info)
{
    Serial.printf("Token status: %s\n", GSheet.getTokenStatus(info).c_str());
}
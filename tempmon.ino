#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define A0 17

extern const char *webhook_url;

HTTPClient http;
WiFiClientSecure client;

struct Message {
    float thermocoupleVoltage;
    String toJSON();
};

String Message::toJSON() {
    return String(R"JSON({"content": "<@272727593881567242> **Pilot light fault**```Thermocouple voltage: )JSON") + String(this->thermocoupleVoltage) + String(R"JSON(```"})JSON");
}

bool sendMessage(Message message) {
    // Setup ssl
    client.setInsecure();
    client.connect(webhook_url, 443);

    // Connect to webhook
    http.begin(client, webhook_url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(message.toJSON());

    // Close connection
    http.end();

    return httpCode == 204;
}

void setup() {
    pinMode(A0, INPUT_PULLDOWN_16);

    // Start the serial connection
    Serial.begin(115200);
    Serial.println();

    // Connect to the wifi
    WiFi.begin("HomeofSkipper", "skipperisthebestdog");

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    // Wait for the serial port to be avaliable
    while (!Serial.available()) {
    }

    // Print wifi info
    WiFi.printDiag(Serial);
}

void loop() {
    double thermocoupleVoltage = ((double) analogRead(A0) / 1024.0) * 3.3;
    Serial.println(thermocoupleVoltage);

    // bool success = sendMessage({.thermocoupleVoltage = 10});
}

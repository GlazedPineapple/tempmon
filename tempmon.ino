#include "secrets.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define ON_THRESHOLD .08

const PROGMEM char webhook_url[] = WEBHOOK_URL;
const PROGMEM char ssid[] = STASSID;
const PROGMEM char password[] = STAPSK;

HTTPClient http;
WiFiClientSecure client;

struct Message {
    float thermocoupleVoltage;
    String toJSON();
};

String Message::toJSON() {
    String json;

    json.concat(F("{\"content\": \"<@272727593881567242> **Pilot light fault**```yml\\nThreshold Voltage: "));
    json.concat(String(ON_THRESHOLD * 1000));
    json.concat(F(" mv\\nThermocouple Voltage: "));
    json.concat(String(this->thermocoupleVoltage * 1000));
    json.concat(F(" mv```\"}"));

    return json;
}

bool sendMessage(Message message) {
    // Setup ssl
    client.setInsecure();
    client.connect(webhook_url, 443);

    // Connect to webhook
    http.begin(client, webhook_url);
    http.addHeader(F("Content-Type"), F("application/json"));
    int httpCode = http.POST(message.toJSON());

    // Close connection
    http.end();

    return httpCode == 204;
}

void setup() {
    pinMode(A0, INPUT_PULLDOWN_16);
    pinMode(D5, OUTPUT);
    pinMode(D6, OUTPUT);
    pinMode(D7, OUTPUT);
    pinMode(D8, OUTPUT);

    digitalWrite(D5, LOW);
    digitalWrite(D6, LOW);
    digitalWrite(D7, LOW);
    digitalWrite(D8, LOW);

    // Start the serial connection
    Serial.begin(115200);
    Serial.println();

    // Connect to the wifi
    WiFi.begin(STASSID, STAPSK);

    Serial.print(F("Connecting"));
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
    }
    Serial.println();

    // Wait for the serial port to be avaliable
    // while (!Serial.available()) {
    // }

    // Print wifi info
    WiFi.printDiag(Serial);
}

unsigned long triggerStart = NULL;
unsigned long lastSent = NULL;
unsigned long sendLimit = 60 * 1000;    // 1 minute
unsigned long triggerDelay = 10 * 1000; // 10 seconds

void loop() {
    int raw_adc = analogRead(A0);

    double thermocoupleVoltage = ((double)raw_adc / 1024.0) * 3.3;

    Serial.println(String(thermocoupleVoltage * 1000) + F(" mv"));

    if (thermocoupleVoltage > ON_THRESHOLD + .100) {
        digitalWrite(D5, HIGH); // Green
        digitalWrite(D6, LOW);
        digitalWrite(D7, LOW);
        digitalWrite(D8, LOW);
    } else if (thermocoupleVoltage > ON_THRESHOLD + .050) {
        digitalWrite(D5, LOW);
        digitalWrite(D6, HIGH); // Yellow
        digitalWrite(D7, LOW);
        digitalWrite(D8, LOW);
    } else if (thermocoupleVoltage > ON_THRESHOLD) {
        digitalWrite(D5, LOW);
        digitalWrite(D6, LOW);
        digitalWrite(D7, HIGH); // Yellow
        digitalWrite(D8, LOW);
    } else {
        digitalWrite(D5, LOW);
        digitalWrite(D6, LOW);
        digitalWrite(D7, LOW);
        digitalWrite(D8, HIGH); // Red
    }

    unsigned long currentMillis = millis();
    if (thermocoupleVoltage < ON_THRESHOLD) {
        if (triggerStart == NULL)
            triggerStart = currentMillis;
        else if (currentMillis - triggerStart > triggerDelay) {
            if (lastSent == NULL || currentMillis - lastSent > sendLimit) {
                Serial.print(F("Sending message..."));

                bool success = sendMessage({.thermocoupleVoltage = thermocoupleVoltage});

                if (success) {
                    Serial.println(F("Success"));
                } else {
                    Serial.println(F("Failure"));
                }

                lastSent = currentMillis;
            }
        }
    } else {
        triggerStart = NULL;
    }
}
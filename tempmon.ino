#include "secrets.hpp"
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define ON_THRESHOLD .08

class Secrets {
  public:
    static const __FlashStringHelper *webhook_url() {
        return F(WEBHOOK_URL);
    }
    static const __FlashStringHelper *ssid() {
        return F(STA_SSID);
    }
    static const __FlashStringHelper *password() {
        return F(STA_PSK);
    }
    static const char *ota_pass() {
        return STA_PSK;
    }
};

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
    client.connect(Secrets::webhook_url(), 443);

    // Connect to webhook
    http.begin(client, Secrets::webhook_url());
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
    WiFi.begin(Secrets::ssid(), Secrets::password());

    Serial.print(F("Connecting..."));
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    Serial.println(F("Poggers!"));

    // Wait for the serial port to be avaliable
    // while (!Serial.available()) {
    // }

    // Print wifi info
    WiFi.printDiag(Serial);

    // Setup OTA
    ArduinoOTA.setHostname("tempmon");
    ArduinoOTA.setPassword(Secrets::ota_pass());

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
}

unsigned long triggerStart = NULL;
unsigned long lastSent = NULL;
unsigned long sendLimit = 10 * 60 * 1000; // 10 minutes
unsigned long triggerDelay = 10 * 1000;   // 10 seconds

void loop() {
    // Handle possible OTA update
    ArduinoOTA.handle();

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
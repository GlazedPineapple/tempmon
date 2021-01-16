#include "secrets.hpp"
#include "twilio.hpp"
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define ON_THRESHOLD .08

Twilio *twilio = new Twilio(ACCOUNT_SID, AUTH_TOKEN);
ESP8266WebServer twilio_server(8000);

HTTPClient http;
WiFiClientSecure client;

void handle_message();

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
    WiFi.begin(STA_SSID, STA_PSK);

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

    twilio_server.on("/message", handle_message);
    twilio_server.begin();

    Serial.println("Ready");
}

unsigned long triggerStart = NULL;
unsigned long lastSent = NULL;
unsigned long sendLimit = 10 * 60 * 1000; // 10 minutes
unsigned long triggerDelay = 10 * 1000;   // 10 seconds
unsigned long delayUntil = NULL;

void loop() {
    twilio_server.handleClient();

    unsigned long currentMillis = millis();

    if (delayUntil != NULL) {
        if (delayUntil - currentMillis > 0) {
            delayUntil = NULL;
        } else {
            return;
        }
    }

    int raw_adc = analogRead(A0);

    double thermocoupleVoltage = ((double)raw_adc / 1024.0) * 3.3;

    // Serial.println(String(thermocoupleVoltage * 1000) + F(" mv"));

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

    if (thermocoupleVoltage < ON_THRESHOLD) {
        if (triggerStart == NULL)
            triggerStart = currentMillis;
        else if (currentMillis - triggerStart > triggerDelay) {
            if (lastSent == NULL || currentMillis - lastSent > sendLimit) {
                Serial.print(F("Sending message..."));

                String message;
                message.concat(F("**Pilot light fault**\nThreshold Voltage: "));
                message.concat(String(ON_THRESHOLD * 1000));
                message.concat(F(" mv\nThermocouple Voltage: "));
                message.concat(String(thermocoupleVoltage * 1000));
                message.concat(F(" mv"));

                String response;
                bool success = twilio->send_message(
                    TO_NUM,
                    FROM_NUM,
                    message,
                    response,
                    "");

                Serial.println(response);

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

void handle_message() {
    Serial.println("Incoming connection!");

    bool auth = false;
    String command = NULL;

    for (int i = 0; i < twilio_server.args(); i++) {
        String arg_name = twilio_server.argName(i);
        String arg = twilio_server.arg(i);

        if (arg_name == "From" && arg == TO_NUM) {
            auth = true;
        } else if (arg_name == "Body") {
            command = arg;
        }
    }

    String response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    if (command != NULL && auth) {
        switch (command) {
            case "ack":
                delayUntil = millis() + (30 * 60 * 1000);
                response += "<Response><Message>"
                            "Delaying for 30 minutes"
                            "</Message></Response>";
                break;
            case "volt":
                response += "Threshold Voltage: ";
                response += String(ON_THRESHOLD * 1000);
                response    += " mv\nThermocouple Voltage: ";
                response += String(thermocoupleVoltage * 1000);
                response += " mv";
                break;
            default:
                response += "<Response><Message>"
                            "Avaliable Commands:"
                            "ack - Acknowledge the warning and suppress messages for 30 minutes"
                            "volt - Get the current voltage of the thermocouple"
                            "</Message></Response>";
                break;
        }
    }
}
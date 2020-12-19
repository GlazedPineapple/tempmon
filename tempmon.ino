#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

extern const char* webhook_url;

HTTPClient http;

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(115200);
  while (!Serial.available()) {}
  Serial.println();

  WiFi.begin("HomeofSkipper", "skipperisthebestdog");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  WiFi.printDiag(Serial);

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println(webhook_url); 
  // TODO: SSL support >:(

  http.begin(webhook_url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(R"JSON({"content": "<@272727593881567242> **Pilot light fault**\n```Thermocouple voltage: {}```"})JSON");
  String payload = http.getString();

  Serial.println(httpCode);
  Serial.println(payload);
  http.end();
}

// the loop routine runs over and over again forever:
void loop() { 
}

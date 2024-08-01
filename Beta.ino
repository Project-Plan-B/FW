#include <ESP8266WiFi.h>
#include <DHTesp.h>

#define DHTPIN 4
DHTesp dht;

const char* ssid = "SSID";
const char* password = "PW";
const char* server = "IP";
const int port = 1234; // '1234' is test number.

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");

  dht.setup(DHTPIN, DHTesp::DHT22);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void loop() {
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);

  String jsonPayload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";
  Serial.println("Sending data: " + jsonPayload);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    if (client.connect(server, port)) {
      client.println("POST /info/other HTTP/1.1");
      client.println("Host: " + String(server));
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(jsonPayload.length());
      client.println();
      client.println(jsonPayload);
      client.stop();
    } else {
      Serial.println("Failed to connect to server");
    }
  } else {
    Serial.println("WiFi not connected");
  }

  delay(60000);
}

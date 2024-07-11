#include <DHT.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define CO2PIN A7

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial espSerial(16, 17);

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  dht.begin();

  espSerial.println("AT");
  delay(1000);
  while (espSerial.available()) {
    Serial.write(espSerial.read());
  }
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int co2Value = analogRead(CO2PIN);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C");
  Serial.print("CO2: ");
  Serial.println(co2Value);

  String jsonData = createJSONData(t, h, co2Value);
  sendToServer(jsonData);

  delay(2000);
}

String createJSONData(float temperature, float humidity, int co2) {
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["co2"] = co2;

  String jsonData;
  serializeJson(doc, jsonData);
  return jsonData;
}

void sendToServer(String jsonData) {
  espSerial.println("AT+CIPSTART=\"TCP\",\"<서버 IP>\",<포트>");
  delay(2000);

  String httpRequest = "POST /update HTTP/1.1\r\n";
  httpRequest += "Host: <서버 IP>\r\n";
  httpRequest += "Content-Type: application/json\r\n";
  httpRequest += "Content-Length: " + String(jsonData.length()) + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
  httpRequest += jsonData;

  espSerial.print("AT+CIPSEND=");
  espSerial.println(httpRequest.length());
  delay(1000);

  espSerial.print(httpRequest);
  delay(2000);

  espSerial.println("AT+CIPCLOSE");
  delay(1000);
}

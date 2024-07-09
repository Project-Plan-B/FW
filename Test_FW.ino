#include <DHT.h>
#include <SoftwareSerial.h>

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

  String data = "temperature=" + String(t) + "&humidity=" + String(h) + "&co2=" + String(co2Value);
  sendToServer(data);

  delay(2000);
}

void sendToServer(String data) {
  espSerial.println("AT+CIPSTART=\"TCP\",\"<Node.js 서버 IP>\",<포트>");
  delay(2000);

  String httpRequest = "GET /update?" + data + " HTTP/1.1\r\nHost: <Node.js 서버 IP>\r\nConnection: close\r\n\r\n";
  espSerial.print("AT+CIPSEND=");
  espSerial.println(httpRequest.length());
  delay(1000);

  espSerial.print(httpRequest);
  delay(2000);

  espSerial.println("AT+CIPCLOSE");
  delay(1000);
}

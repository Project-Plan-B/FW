#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHTesp.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define DHTPIN 2
DHTesp dht;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ESP8266WebServer server(80);

const int EEPROM_SSID_ADDR = 0;
const int EEPROM_PASSWORD_ADDR = 32;
const int EEPROM_SIZE = 64;

void setup() {
  Serial.begin(115200);
  Serial.println();

  dht.setup(DHTPIN, DHTesp::DHT22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  EEPROM.begin(EEPROM_SIZE);

  String ssid = readEEPROMString(EEPROM_SSID_ADDR);
  String password = readEEPROMString(EEPROM_PASSWORD_ADDR);

  if (ssid.length() > 0 && password.length() > 0) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi");
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.print("WiFi: Connected");
      display.setCursor(0, 10);
      display.print("DHT: ");
      display.println(dht.getStatusString());
      display.display();
    } else {
      Serial.println("Failed to connect to WiFi");
      startAPMode();
    }
  } else {
    startAPMode();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    delay(dht.getMinimumSamplingPeriod());

    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();

    bool sensorConnected = !isnan(humidity) && !isnan(temperature);

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

    bool serverConnected = false;
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      if (client.connect("IP", 1234)) {
        client.println("POST /info/other HTTP/1.1");
        client.println("Host: IP");
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(jsonPayload.length());
        client.println();
        client.println(jsonPayload);
        client.stop();
        serverConnected = true;
      } else {
        Serial.println("Failed to connect to server");
      }
    } else {
      Serial.println("WiFi not connected");
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("WiFi: ");
    display.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    display.setCursor(0, 10);
    display.print("DHT: ");
    display.println(sensorConnected ? "OK" : "Disconnected");

    display.setCursor(0, 20);
    display.print("Server: ");
    display.println(serverConnected ? "Connected" : "Failed");

    display.display();

    delay(60000);
  } else {
    server.handleClient();
  }
}

void startAPMode() {
  WiFi.softAP("ESP8266_Config", "12345678");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.begin();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("AP Mode Started");
  display.display();
}

void handleRoot() {
  String html = "<html><body><h1>ESP8266 Configuration</h1>";
  html += "<form action=\"/submit\" method=\"post\">";
  html += "SSID: <input type=\"text\" name=\"ssid\"><br>";
  html += "Password: <input type=\"text\" name=\"password\"><br>";
  html += "<input type=\"submit\" value=\"Submit\">";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleSubmit() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  writeEEPROMString(EEPROM_SSID_ADDR, ssid);
  writeEEPROMString(EEPROM_PASSWORD_ADDR, password);

  String response = "<html><body><h1>Credentials Saved</h1>";
  response += "<p>SSID: " + ssid + "</p>";
  response += "<p>Password: " + password + "</p>";
  response += "<p>Restarting in 3 seconds...</p>";
  response += "</body></html>";
  server.send(200, "text/html", response);

  delay(3000);
  ESP.restart();
}

void writeEEPROMString(int addr, const String &str) {
  for (int i = 0; i < str.length(); i++) {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + str.length(), '\0');
  EEPROM.commit();
}

String readEEPROMString(int addr) {
  char data[32];
  int len = 0;
  unsigned char k;
  k = EEPROM.read(addr);
  while (k != '\0' && len < 32) {
    k = EEPROM.read(addr + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}

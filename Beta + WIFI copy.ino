#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <DHTesp.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C  // I2C 주소

#define DHTPIN 2    // DHT 센서 핀 (D4)
DHTesp dht;

// Initialize the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "SSID";
const char* password = "PW";
const char* server = "IP";
const int port = 1234; // '1234' is test number.

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  dht.setup(DHTPIN, DHTesp::DHT22);

  // Initialize I2C and OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  bool wifiConnected = false;
  unsigned long startTime = millis();
  
  // Check WiFi connection status
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) { // Wait for 10 seconds
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
  }

  // Update OLED with WiFi status
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("WiFi: ");
  display.println(wifiConnected ? "Connected" : "Failed");

  // Display DHT sensor connection status
  display.setCursor(0, 10);
  display.print("DHT: ");
  display.println(dht.getStatusString());

  display.display();
  
  delay(2000);  // Pause for 2 seconds
}

void loop() {
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
    if (client.connect(server, port)) {
      client.println("POST /info/other HTTP/1.1");
      client.println("Host: " + String(server));
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

  // Update OLED display with sensor data and connection status
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

  delay(60000);  // Delay between readings and updates
}

#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP8266WiFi.h>      // For WiFi connectivity
#include <ThingSpeak.h>       // For ThingSpeak API

// Soil Moisture Sensor Configuration
#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN D6
#define MOISTURE_THRESHOLD 30

// DHT11 Sensor Configuration
#define DHT_PIN D7
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_BMP085 bmp;

// WiFi and ThingSpeak Configuration
const char* ssid = "Swathy";         // Replace with your WiFi SSID
const char* password = "00000123";       // Replace with your WiFi Password
unsigned long myChannelNumber = 2758479; // Replace with your ThingSpeak Channel ID
const char* myWriteAPIKey = "Q7QTZ1EYUX6NT6QB"; // Replace with your ThingSpeak Write API Key

WiFiClient client;

void setup() {
  Serial.begin(115200);

  // Initialize BMP085
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1);
  }

  // Initialize DHT11
  dht.begin();

  // Initialize Soil Moisture and Relay Pins
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // Read Soil Moisture
  int sensorValue = analogRead(SOIL_MOISTURE_PIN);
  float moisturePercentage = (100 - ((sensorValue / 1023.0) * 100));

  if (moisturePercentage < MOISTURE_THRESHOLD) {
    digitalWrite(RELAY_PIN, HIGH); // Turn ON pump
  } else {
    digitalWrite(RELAY_PIN, LOW); // Turn OFF pump
  }

  // Read BMP085 Data
  float temperatureBMP = bmp.readTemperature();
  int32_t pressure = bmp.readPressure();

  // Read DHT11 Data with Retry
  float temperatureDHT = NAN;
  float humidity = NAN;
  for (int i = 0; i < 3; i++) { // Retry up to 3 times
    temperatureDHT = dht.readTemperature();
    humidity = dht.readHumidity();
    if (!isnan(temperatureDHT) && !isnan(humidity)) {
      break;
    }
    delay(2000); // Wait before retrying
  }

  if (isnan(temperatureDHT) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor after retries!");
    temperatureDHT = 0; // Set default or error values
    humidity = 0;
  }

  // Display Data
  Serial.print("Soil Moisture: "); Serial.print(moisturePercentage); Serial.println("%");
  Serial.print("BMP085 Temperature: "); Serial.print(temperatureBMP); Serial.println(" *C");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.println(" Pa");
  Serial.print("DHT11 Temperature: "); Serial.print(temperatureDHT); Serial.println(" *C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");

  // Send Data to ThingSpeak
  ThingSpeak.setField(1, moisturePercentage);
  ThingSpeak.setField(2, temperatureBMP);
  ThingSpeak.setField(3, pressure / 100); // Convert to hPa
  ThingSpeak.setField(4, temperatureDHT);
  ThingSpeak.setField(5, humidity);

  int statusCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (statusCode == 200) {
    Serial.println("Data sent to ThingSpeak successfully!");
  } else {
    Serial.print("Error sending data to ThingSpeak. HTTP code: ");
    Serial.println(statusCode);
  }

  // Delay before next loop iteration
  delay(20000); // ThingSpeak accepts updates every 15 seconds
}

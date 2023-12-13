#include <WiFi.h>
#include <SPIFFS.h>
#include "ThingSpeak.h"
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Define Variable
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Motor DC
#define MOTOR_PIN 12
#define MOTOR_THRESHOLD 34 // Ambang batas suhu untuk menghidupkan motor

// Connect to Internet
const char* ssid = "Alpin";
const char* password = "Alfin110702";

// Linked to ThingSpeak
unsigned long myChannelID = 2128579;
const char * myWriteAPIKey = "D5LO6IF7ULS66NIT";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
WiFiClient client;

String readDHTTemperature() {
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return "0";
  } else {
    Serial.println(temperature);
    return String(temperature);
  }
}

String readDHTHumidity() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return "0";
  } else {
    Serial.println(humidity);
    return String(humidity);
  }
}

void controlDCMotor(float temperature) {
  if (temperature >= MOTOR_THRESHOLD) {
    // Hidupkan motor DC jika suhu lebih dari atau sama dengan 34°C
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    // Matikan motor DC jika suhu kurang dari 34°C
    digitalWrite(MOTOR_PIN, LOW);
  }
}

void setup() {
  // Starting
  Serial.begin(115200); // Start Serial
  dht.begin(); // Start DHT Sensor
  ThingSpeak.begin(client); // Initialize ThingSpeak

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Motor DC
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // SPIFFS
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    String temperature = readDHTTemperature();
    controlDCMotor(temperature.toFloat()); // Kontrol motor berdasarkan suhu
    request->send_P(200, "text/plain", temperature.c_str());
    int x = ThingSpeak.writeField(myChannelID, 1, temperature, myWriteAPIKey);
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
    int y = ThingSpeak.writeField(myChannelID, 2, readDHTHumidity(), myWriteAPIKey);
  });

  server.begin(); // Start server
}

void loop() {
  delay(1000); // Add a delay to avoid sending data too frequently
}

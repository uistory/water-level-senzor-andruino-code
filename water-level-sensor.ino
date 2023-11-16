#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include <ArduinoJson.h>

// Define connections to sensor
const int TRIG_PIN = 10;
const int ECHO_PIN = 11;
// Speed of sound in air (meters per second)
const float SPEED_OF_SOUND = 343.0;
char measureUnit[] = "mm";
// WiFi settings
char ssid[] = "T-593990";
char pass[] = "b4btuhb3hep7";
// Server settings
char serverAddress[] = "uistory-water-lvl.azurewebsites.net";  // remote server we will connect to
const int serverPort = 443;
int status = WL_IDLE_STATUS;
char server[] = "uistory-water-lvl.azurewebsites.net";
WiFiSSLClient client;

// Function to measure distance using the ultrasonic sensor
int measureDistance() {
  // Set the trigger pin LOW for 2 microseconds
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Set the trigger pin HIGH for 10 microseconds to send pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  // Return the trigger pin to LOW
  digitalWrite(TRIG_PIN, LOW);

  // Measure the duration of the incoming pulse
  float duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate distance in centimeters
  // Divide by 2 to account for the round trip of the sound wave
  int distance = round((duration * SPEED_OF_SOUND) * 10 / 20000);

  return distance;
}

// Function to send a POST request to the server
void sendPostRequest(float distance) {
  if (client.connect(server, 443)) {
    Serial.println("Connected to server");

    // Make a HTTP POST request with JSON body
    client.println("POST /api/SensorStatus HTTP/1.1");
    client.println("Host: uistory-water-lvl.azurewebsites.net");
    client.println("Content-Type: application/json");
    client.println("Connection: close");

    // Calculate the length of JSON data and send it
    // Create a JSON document
    DynamicJsonDocument doc(100);  // Adjust the size according to your data
    // Add data to the JSON document
    doc["distance"] = distance;
    doc["unit"] = measureUnit;
    String jsonData;
    serializeJson(doc, jsonData);
    Serial.println("JSON DATA " + jsonData);
    client.print("Content-Length: ");
    client.println(jsonData.length());
    client.println();
    client.println(jsonData);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }

  printWifiStatus();

  Serial.println("\nStarting connection to server...");
  // Set pin modes for sensor connections
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
}

void read_response() {
  uint32_t received_data_num = 0;
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
    received_data_num++;
    if (received_data_num % 80 == 0) {
      Serial.println();
    }
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void waitForResponse() {
  Serial.println("Waiting for response...");
  unsigned long startMillis = millis();
  while (!client.available()) {
    // Check if the maximum wait time (10 seconds) has elapsed
    if (millis() - startMillis > 10000) {
      Serial.println("Timeout: No response received from the server.");
      return;  // Exit the function if no response is received within the timeout
    }

    delay(100);  // Wait a short time before checking again
  }
  // If the code reaches here, it means a response has been received
  Serial.println("Response received from the server.");
  // Read and print the server response
  read_response();
}

void handleConnectionLoss() {
  Serial.println();
  Serial.println("Connection lost. Reconnecting...");
  // Attempt to reconnect to WiFi if not connected
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.begin(ssid, pass);
    delay(1000);
  }
  printWifiStatus();
  // Attempt to reconnect to the server
  while (!client.connect(server, 443)) {
    Serial.println("Retrying connection to server...");
    delay(1000);
  }
  Serial.println("Reconnected to server.");
}

void loop() {
  int distance = measureDistance();
  sendPostRequest(distance);  // Send POST request
  waitForResponse();
  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnecting from server.");
    client.stop();
  }
  delay(10000);  // Delay before repeating measurement
}

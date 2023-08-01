#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <HttpClient.h>

const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";

ESP8266WebServer server(80);

LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Server settings
const char* serverAddress = "127.0.0.1"; 
const int serverPort = 80;
const char* endpoint = "/send_status"; 

const int numParkingSpaces = 5;
const int triggerPins[] = {D1, D2, D3, D4, D5}; 
const int echoPins[] = {D6, D7, D8, D9, D10};   
bool isSpaceOccupied[numParkingSpaces] = {false};

void setup() {
  lcd.begin(16, 2);
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print("System");
  delay(2000);

  lcd.clear();
  lcd.print("Initializing...");

  for (int i = 0; i < numParkingSpaces; i++) {
    pinMode(triggerPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  for (int i = 0; i < numParkingSpaces; i++) {
    // Measure the distance using the ultrasonic sensor
    float distance = measureDistance(triggerPins[i], echoPins[i]);

    // Convert the distance to centimeters (change this based on your sensor's specifications)
    int distanceInCm = (int)(distance * 100);

    // Determine if a car is present based on the measured distance
    bool isCarPresent = (distanceInCm < 30); // Adjust this threshold based on your setup

    // Update the parking space status
    if (isSpaceOccupied[i] != isCarPresent) {
      isSpaceOccupied[i] = isCarPresent;
      sendParkingStatus(i, isCarPresent);
    }

    // Display the parking status on the LCD
    lcd.clear();
    lcd.print("Space ");
    lcd.print(i + 1);
    lcd.setCursor(0, 1);
    lcd.print(isCarPresent ? "Occupied" : "Vacant");

    delay(1000);
  }
}

// Function to measure distance using the ultrasonic sensor
float measureDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH);
  float distance = (duration * 0.0343) / 2;
  return distance;
}


void sendParkingStatus(int parkingSpace, bool isOccupied) {
    WiFiClient wifiClient;
    HttpClient client(wifiClient, serverAddress, serverPort);

    String requestBody = "parkingSpace=" + String(parkingSpace) + "&isOccupied=" + String(isOccupied);

    client.beginRequest();
    client.post(endpoint);
    client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
    client.sendHeader("Content-Length", requestBody.length());
    client.beginBody();
    client.print(requestBody);
    client.endRequest();

    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

    // Check for a successful response
    if (statusCode == 200) {
        Serial.println("Status sent successfully!");
        Serial.print("Server Response: ");
        Serial.println(response);
    } else {
        Serial.print("Error sending status. HTTP status code: ");
        Serial.println(statusCode);
        Serial.print("Server Response: ");
        Serial.println(response);
    }
}

void handleRoot() {
    String htmlContent = "<!DOCTYPE html><html><head>";
    htmlContent += "<title>Parking Space Status</title>";
    htmlContent += "<style>";
    htmlContent += "body { font-family: Arial, sans-serif; text-align: center; }";
    htmlContent += "h1 { color: #008080; }";
    htmlContent += "p { font-size: 18px; margin: 10px 0; }";
    htmlContent += ".occupied { color: #FF0000; font-weight: bold; }";
    htmlContent += ".vacant { color: #00CC00; font-weight: bold; }";
    htmlContent += "</style>";
    htmlContent += "</head><body>";
    htmlContent += "<h1>Parking Space Status</h1>";

    // Loop through all parking spaces and add their status to the HTML content
    for (int i = 0; i < numParkingSpaces; i++) {
        htmlContent += "<p>Space " + String(i + 1) + ": <span class=\"" + (isSpaceOccupied[i] ? "occupied" : "vacant") + "\">" + (isSpaceOccupied[i] ? "Occupied" : "Vacant") + "</span></p>";
    }

    htmlContent += "</body></html>";

    // Send the HTML content as a response to the client
    server.send(200, "text/html", htmlContent);
}
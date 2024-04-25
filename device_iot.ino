#include <WiFi.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define AWS_IOT_PUBLISH_TOPIC   "Omeliukh-Lab4/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "Omeliukh-Lab4/sub"

#define CLK 35
#define DT 34
#define SW 32

WiFiClientSecure espClient = WiFiClientSecure();
PubSubClient client(espClient);

String direction;
String values;

const unsigned long postingInterval = 3L * 1000L; // Post data every 3 seconds.
unsigned long lastUploadedTime = 0;

int counter = 0;
int currentStateCLK;
int previousStateCLK;

int flag = 0;
int buttonState = 0;

String encodir = "";
 
void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  espClient.setCACert(AWS_CERT_CA);
  espClient.setCertificate(AWS_CERT_CRT);
  espClient.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}
 
void publishMessage() {
  StaticJsonDocument<200> doc;
  doc["direction"] = direction;
  doc["values"] = values;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 
void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup() {
  Serial.begin(115200);
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  pinMode (CLK, INPUT);
  pinMode (DT, INPUT);
  pinMode (SW, INPUT);

  previousStateCLK = digitalRead(CLK);
  buttonState = LOW;

  connectAWS();
}

void loop() {

  buttonState = digitalRead(SW);

  if (buttonState == HIGH) {
    if (flag == 0) {
      readHW040();
      if (millis() - lastUploadedTime > postingInterval) {
        Serial.println("Diraction" + direction);
        Serial.println("Values" + values);

        publishMessage();    
        lastUploadedTime = millis();
      }
      flag = 1;
    } else {
      flag = 0;
    }
  }

  client.loop();
}

void readHW040() {
  currentStateCLK = digitalRead(CLK);

  if (currentStateCLK != previousStateCLK)  {
      if (digitalRead(DT) != currentStateCLK) {
        direction = "0";
        counter --;
        encodir = "Right side";

      } else {
        direction = "1";
        counter ++;
        encodir = "Left side";
      }

      values = counter;
  }

  previousStateCLK = currentStateCLK;
}
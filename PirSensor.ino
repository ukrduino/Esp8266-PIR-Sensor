#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


const char* ssid = "";
const char* password = "";

const String arm = "zff343";
const String disarm = "45sfdg";
const String getStatus = "fsf255";
const byte greenLedPin = 12;
const byte redLedPin = 15;
const byte blueLedPin = 13;
const byte sensorPin = 14;
const byte boardLedPin = 2;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(3333);
bool motionDetected = false; //0 - OK , 1 - Alarm
bool sensorArmed = false;
byte pirState = LOW;             // we start, assuming no motion detected
byte val = LOW;                    // variable for reading the pin status


void setup() {
  Serial.begin(115200);
  // prepare GPIO
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(boardLedPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(blueLedPin, HIGH);
  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print(WiFi.localIP());
  Serial.println(":3333");
  delay(60000); //sensor calibration period
}

void loop() {
  askSensor();
  showSensorStatus();
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  if (req.indexOf(arm) != -1) {
    sensorArmed = true;
    sendResponse(client, getStatusResponse());
  }

  else if (req.indexOf(disarm) != -1) {
    sensorArmed = false;
    resetSensor();
    sendResponse(client, getStatusResponse());
  }
  else if (req.indexOf(getStatus) != -1) {
    sendResponse(client, getStatusResponse());
  }
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
  client.flush();
  Serial.println("Client disonnected");
  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}

String getStatusResponse() {
  String response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["motionDetected"] = motionDetected;
  root["sensorArmed"] = sensorArmed;
  root["activeTimeMilis"] = millis();
  String output;
  root.printTo(output);
  Serial.println("------------ SensorStatus response-----------");
  Serial.println(output);
  return response += output;
}

// about motion detection http://henrysbench.capnfatz.com/henrys-bench/arduino-sensors-and-input/arduino-hc-sr501-motion-sensor-tutorial/
void askSensor() {
  val = digitalRead(sensorPin);
  if (val == HIGH) {            // check if the input is HIGH
    digitalWrite(boardLedPin, LOW); // turn LED ON
    if (pirState == LOW) {
      Serial.println("Motion detected!");
      // We only want to print on the output change, not state
      if (sensorArmed) {
        if (!motionDetected) {
          motionDetected = true;
          return;
        }
      }
      pirState = HIGH;
    }
  } else {
    digitalWrite(boardLedPin, HIGH); // turn LED OFF
    if (pirState == HIGH) {
      // we have just turned of
      Serial.println("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
  }
}

void resetSensor() {
  motionDetected = false;
}

void showSensorStatus() {
  if (sensorArmed && motionDetected) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);
    digitalWrite(blueLedPin, HIGH);
  }
  else if (sensorArmed && !motionDetected) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
    digitalWrite(blueLedPin, LOW);
  }
  else if (!sensorArmed) {
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
    digitalWrite(blueLedPin, LOW);
  }
}

void sendResponse(WiFiClient client, String response) {
  client.print(response);
  Serial.println("------------ response sent-----------");
}


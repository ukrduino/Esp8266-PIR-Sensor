#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


const char* ssid = "";
const char* password = "";

const byte greenLedPin = 12;
const byte redLedPin = 15;
const byte blueLedPin = 13;
const byte sensorPin = 14;
const byte boardLedPin = 2;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(3333);
bool motionDetected = false; //0 - OK , 1 - Alarm
bool sensorEnabled = false;
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
  digitalWrite(boardLedPin, HIGH);
  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
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
  sendSensorIp();
  delay(30000); //sensor calibration period
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
  if (req.indexOf("enable") != -1) {
    sensorEnabled = true;
    sendResponse(client, getStatusResponse());
  }

  else if (req.indexOf("disable") != -1) {
    sensorEnabled = false;
    resetSensor();
    sendResponse(client, getStatusResponse());
  }
  else if (req.indexOf("getStatus") != -1) {
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
  root["sensorEnabled"] = sensorEnabled;
  root["activeTimeSeconds"] = millis() / 1000;
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
      if (sensorEnabled) {
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
  if (sensorEnabled && motionDetected) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, LOW);
    digitalWrite(blueLedPin, HIGH);
  }
  else if (sensorEnabled && !motionDetected) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
    digitalWrite(blueLedPin, LOW);
  }
  else if (!sensorEnabled) {
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
    digitalWrite(blueLedPin, LOW);
  }
}

void sendResponse(WiFiClient client, String response) {
  client.print(response);
  Serial.println("------------ response sent-----------");
}

void sendSensorIp() {
  // Remote site information
  const char main_board_http_site[] = "192.168.1.38";
  const int main_board_http_port = 3000;
  WiFiClient client;
  if (!client.connect(main_board_http_site, main_board_http_port)) {
    Serial.println("connection failed");
    return;
  }
  // We now create a URI for the request
  String ipaddress = WiFi.localIP().toString();
  String url = "/register";
  url += "?ip=";
  url += ipaddress;
  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + main_board_http_site + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}

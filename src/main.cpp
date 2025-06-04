// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// wifi credentials and site for posting
#include "setup.h"

const char compileTime[] = " Compiled on " __DATE__ " " __TIME__;

const long utcOffsetInSeconds = UTC_OFFSET * 3600;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiClient client;
HTTPClient http;

const int BUFFER_SIZE = 300;
 
String s;
int numChars = 1000;
boolean newData = false;
int ndx = 0;
char receivedChars[1000];
char rc;

// stuff for blink-while-connected
int ledState = LOW;
unsigned long previousMillis = 0;
const long conn_blink = 2000;
const long dis_blink = 500;
bool connected = 0;

// Set your access point network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;

void blink(unsigned long interval);
void rcvSerial();
void handleSerial();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.printf("\n\nGEMS ESP %s \n", compileTime);
}

void loop(){
    rcvSerial();
    handleSerial();
    if (WiFi.status() == WL_CONNECTED) {
      blink(conn_blink);
    } else {
      blink(dis_blink);
    }
}


void sendSerial(Stream& serial, const char* data) {
  serial.write('<');
  serial.print(data);
  serial.write('>');
  serial.println();
}

void rcvSerial() {
  static bool recvInProgress = false;
  static int ndx = 0;
  const char START_MARKER = '<';
  const char END_MARKER = '>';

  while (Serial.available() > 0 && !newData) {
    char rc = Serial.read();

    // Check for start marker if not already collecting
    if (!recvInProgress) {
      if (rc == START_MARKER) {
        recvInProgress = true;
        ndx = 0;
      }
      continue;  // Skip rest of loop until start marker found
    }

    // Add character to buffer if not end marker
    if (rc != END_MARKER) {
      if (ndx < numChars - 1) {  // Leave space for null terminator
        receivedChars[ndx] = rc;
        ndx++;
      }
    } else {
      // End marker found, terminate string
      receivedChars[ndx] = '\0';
      recvInProgress = false;
      newData = true;
    }
  }
}

void handleTimeRequest() {
  timeClient.begin();
  timeClient.update();
  if (timeClient.isTimeSet())
  {
    Serial.print("<T");
    Serial.print(timeClient.getEpochTime());
    Serial.print(">");
  }
  else
  {
    Serial.println("T0");
  }
  timeClient.end();
}

void handleCommandCheck() {
  http.begin(client, GET_URL);
  int httpCode = http.GET(); // Make the request
  if (httpCode > 0) {
    String payload = http.getString();
    // Serial.println(httpCode);
    // Serial.println(payload);
    if (payload == "Start") {
      sendSerial(Serial, "C1");
    } else if (payload == "Stop") {
      sendSerial(Serial, "C2");
    } else {
      //Serial.println('0');
      sendSerial(Serial, "C0");
    }
  } else {
    // Serial.println("Error on HTTP request");
    sendSerial(Serial, "C0");
  }
  http.end(); // Free the resources
}

void handleDataPacket() {
  http.begin(client, POST_URL);
  http.addHeader("Content-Type", "text/plain");
  int httpCode  = http.POST(receivedChars);
  // http.getString();
  http.end();
  if (httpCode == 200) {
    sendSerial(Serial, "Da");
  } else {
    sendSerial(Serial, "D0");
  }
}

void handleSerial() {
  if (!newData) return;
  // if not connected, send 0 in all cases
  if (WiFi.status() != WL_CONNECTED) {
    sendSerial(Serial, "0");
    newData = false;
    return;
  }

  switch (receivedChars[0]) {
    case '^':
      sendSerial(Serial, "1");
      break;
    case '$':
      handleTimeRequest();
      break;
    case '?':
      handleCommandCheck();
      break;
    case '*': // GPS request for compatibility with cell module
      sendSerial(Serial, "G0");
      break;
    default:
      handleDataPacket();
      break;
  }

  newData = false;
}

void blink(unsigned long interval) {
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}

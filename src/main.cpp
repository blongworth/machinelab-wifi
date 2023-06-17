/*!
 * @file main.cpp
 *
 * Configure ESP8266 for use with MachineLab teensys
 *
 * Written by Brett Longworth
 *
 * BSD license, all text above must be included in any redistribution
 */

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// flasher library
#include "Flasher.h"

// wifi credentials and site for posting
#include "setup.h"

// use offset from setup
const long utcOffsetInSeconds = UTC_OFFSET * 3600;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// HTTP endpoint for POSTing data
String site= POST_URL;

const int BUFFER_SIZE = 300;
WiFiClient client;
 
String s;
int numChars = 1000;
boolean newData = false;
int ndx = 0;
char receivedChars[1000];
char rc;

// stuff for flashing led
const int conOn = 2000;
const int conOff = 2000;
const int disOn = 100;
const int disOff = 900;
Flasher flasher(LED_BUILTIN, disOff, disOn); //NodeMCU LED's inverted, LOW = on

// Set your access point network credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PWD;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);  //Connect to the WiFi network 
  timeClient.begin();
  flasher.begin();
}

void loop(){
  // check for connection, answer appropriately
  if (WiFi.status() != WL_CONNECTED) { 
    flasher.update(disOff, disOn);
    // if asked, say we have no connection
    if (Serial.available() > 0) {
      char query;
      query = Serial.read();
      if (query == '^') Serial.print(0);
    }
  } else {
    flasher.update(conOff, conOn);
    if (Serial.available() > 0){
      int rlen = Serial.readBytesUntil('#', receivedChars, BUFFER_SIZE);
      rlen++;
      receivedChars[rlen] = '\0'; 
      s=String(receivedChars);

      if (s.substring(0,1) == "^"){
        // tell teensy we have wifi connection and are ready for data
        Serial.print(1);
      } else if (s.substring(0,1) == "$") {
        timeClient.update();
        Serial.print("T");
        Serial.println(timeClient.getEpochTime());
      } else {
        // setup connection and POST data
        HTTPClient http;    //Declare object of class HTTPClient
        http.begin(client,site);      //Specify request destination
        http.addHeader("Content-Type", "text/plain");  //Specify content-type header
        int httpCode = http.POST(s);   //Send the request
        String payload = http.getString(); 
        http.end();
        // send 'a' to let teensy know data has been posted and move to next data packet
        Serial.print('a');
      }
    }
  }
  flasher.run();
}
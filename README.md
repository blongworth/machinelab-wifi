# machinelab-wifi

Firmware for ESP8266 NodeMCU 1.c0 clones. Connects to SSID using credentials in setup.h. Communicates with Teensy via serial and POSTs data to an endpoint.

## Setup

Edit setup.h to include correct SSID and wifi password. Edit POST_URL to be the correct data upload endpoint for the deployment.

Setup for deployment with PlatformIO. If deploying from ArduinoIDE, change `main.cpp` to `main.ino`.

## Features

Establish a wifi connection when started. LED will blink quickly (1Hz, .25s on) while connecting.

Once connected blink led slowly (.5Hz, 1s on). 

While connected, check for serial data from Teensy. If `^` received, reply with `1` to tell teensy that connection is active.

If a string not starting with `^` is recieved, send it to the POST_URL via HTTP POST.

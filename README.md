# SMARTHOME

This repository contains a collection of smart home modules including firmware, diagrams and models for 3D printer. You can reproduce this modules whitout 3D printer, but for curtain control module 3D printer is required.

Most of modules based on ESP8266 chip, like NodeMCU and ESP01. All modules are controlled from server using mqtt. Firmware of all modules adapted for on-the-air updates.

The head of smarthome system based on Raspberry Pi.

## module with RGB LED strip

[firmware](https://github.com/AndroiDjo/smarthome/tree/master/lua/nodemcu_rgb_strip)

This module based on NodeMCU board. Firmware written on Lua.

Features:
- switch LED on movement
- customize fade time
- color pulsating mode

Box 3D model: [link](https://github.com/AndroiDjo/smarthome/tree/master/lua/nodemcu_rgb_strip/box%203d%20model)

Schematic:
![RGB LED strip](https://github.com/AndroiDjo/smarthome/blob/master/lua/nodemcu_rgb_strip/rgb_led_with_pir_bb.png)

## module with double clap control

[firmware](https://github.com/AndroiDjo/smarthome/tree/master/irsend_soundsensor_temp)

This module based on NodeMCU board. Firmware written on C++.

Features:
- switch lamp on double clap
- controlling air conditioning using IR LED
- measuring temperature and humidity

Schematic:
![RGB LED strip](https://github.com/AndroiDjo/smarthome/blob/master/irsend_soundsensor_temp/Scheme/irsend_soundsensor_temp_scheme_bb.png)

## ESP01 + Arduino module

This module consist of [ESP01](https://github.com/AndroiDjo/smarthome/tree/master/esp01_arduino/esp01_relay_arduino) and [Arduino](https://github.com/AndroiDjo/smarthome/tree/master/esp01_arduino/arduino_esp01) bundle.

Features:
- switch lamp on double clap
- controlling air conditioning using IR LED
- measuring temperature and humidity

Box 3D model: [link](https://github.com/AndroiDjo/smarthome/tree/master/esp01_arduino/3dmodel)

Schematic:
![ESP01 + Arduino](https://github.com/AndroiDjo/smarthome/blob/master/esp01_arduino/scheme/esp01_arduino_bb.png)

## high-power LED module

[firmware](https://github.com/AndroiDjo/smarthome/tree/master/esp_bathroom)

This module based on NodeMCU board.

Features:
- turn on LED for short period when you come closer to the door
- turn on LED for long period when you enter room
- configuring LED brighntess
- turn on LED and fan when door is closed
- measuring temperature and humidity

Box 3D model: [link](https://github.com/AndroiDjo/smarthome/tree/master/esp_bathroom/3Dmodels)

Schematic:
![RGB LED strip](https://github.com/AndroiDjo/smarthome/blob/master/esp_bathroom/scheme/led_bathroom_bb.png)

## curtain control module

[firmware](https://github.com/AndroiDjo/smarthome/tree/master/EspCurtainControl)

This module explained on [video](https://www.youtube.com/watch?v=JtYdPwO65WI) (not mine). 
It based on NodeMCU board and Nema17 stepper motor. 3D printer is required for this module!

Features:
- multistage adjustment of the curtain opening speed
- calibrating by mqtt
- protection against double opening
- power outage protection (save position to flash memory)

3D model: [link](https://github.com/AndroiDjo/smarthome/tree/master/EspCurtainControl/3dmodels)

Schematic:
![RGB LED strip](https://github.com/AndroiDjo/smarthome/blob/master/EspCurtainControl/curtain_control_scheme_bb.jpg)

## magic RGB lamp

[firmware](https://github.com/AndroiDjo/smarthome/tree/master/GyverLamp)

This is not my development, i am just integrate it with my smart home system.

Original project [link](https://alexgyver.ru/gyverlamp). 
It based on NodeMCU board.

## SERVER

[source code](https://github.com/AndroiDjo/smarthome/tree/master/websrv)

Server part consists of:
- web server (python+flask)
- frontend (html+javascript)
- message broker (mqtt)
- scheduler (python)
# SMARTHOME

This repository contains a collection of smart home modules including firmware, diagrams and 3D models.
Most of modules based on ESP8266 chip, like NodeMCU and ESP01.
The head of smarthome system based on Raspberry Pi.

## [SERVER](https://github.com/AndroiDjo/smarthome/tree/master/websrv)

Server part consists of:
- web server (python+flask)
- frontend (html+javascript)
- message broker (mqtt)
- scheduler (python)

## [MODULE with RGB LED strip](https://github.com/AndroiDjo/smarthome/tree/master/lua/nodemcu_rgb_strip)

This module based on NodeMCU board. Firmware written on Lua.
Features:
- switch LED on movement
- customize fade time
- color pulsating mode
Schematic:
![RGB LED strip](https://github.com/AndroiDjo/smarthome/blob/master/lua/nodemcu_rgb_strip/rgb_led_with_pir_bb.png)
3D model of case: [link](https://github.com/AndroiDjo/smarthome/tree/master/lua/nodemcu_rgb_strip/box%203d%20model)
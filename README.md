# Automated-Pump
This is an automatic pump controller that is used to turn on and off a pump(or actually any device connected to the relay) for a specific time interval everyday. 

## Overview of the system
At the heart of the system is an ESP8266 that is used to controll all the devices. There is an RTC to take care of the timings. An OLED display shows all the information. A rotary encoder is used to interact with the device. 

## Features
A main feature to highlight is:
> what if the power goes off while the device is still running?

Well, once the power is restored the time is automatically extended to compansate for the time the device was off due to the power loss.

<img src="https://user-images.githubusercontent.com/75741446/232307124-497eed18-80cd-4949-9e7c-44af2c107d43.jpeg" width="750" height="562"> 

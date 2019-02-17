# Temp Hacking for ESP8266

## Goals:

# Detect the temperature, and publish to LCD
# Query the temperature via HTTP
# Post the temperature to the web
# Update wifi SSID and Password (use AP mode?)

## User Experiences

# allow for AP credential reset by holding a pin low/high on boot
# scan SSIDs for good connections, and publish to webpage
# accept SSID/password for network connection, and initate connection to network
#

### Connect to WiFi

# Starting state: device is reset, and has not connected to WiFi network
# Scan for WiFi SSIDs for 30s, and store in array
# Wait for connection from device
# Use web browser to connect in AP mode to WeMos device
# set SSID and password, saved on device
# restart system, connecting to wifi network
# verify by connecting to device on wifi network

## Software Build Environment Setup

# Install Arduino IDE 1.8.8
# Install MacOS USB to UART driver (CH340)
# Install ESP8266 Arduino hardware libs:
```
mkdir -p ~/Documents/dev/Arduino/hardware/esp8266com/esp8266 && cd ~/Documents/dev/Arduino/hardware/esp8266com/esp8266
git clone https://github.com/esp8266/Arduino.git
git submodule update --recursive --remote

```

## References

https://tttapa.github.io/ESP8266/Chap01%20-%20ESP8266.html


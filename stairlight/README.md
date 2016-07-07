Stair Light
===========

This project supplies soft lighting to a particularly dark stairway in my house.  Two 12 volt soft-white LED light strips are mounted under the stair-lip, on landings, each 1/3 of the way up the rotating stairwell.  A photo cell CdS photoresistor attneuates the LED's intensity, depending on the lighting conditions.  A button, hidden under the stair lip, allows for three modes:

- auto: turning on the light when it's dark enough, and dimming the light as conditions change
- on: turn the LEDs on, set to full brightness, regardless of ambient conditions
- off: turn the LEDs off, regardless of ambient conditions

## The auto mode logic is as follows

- if it's dark enough, turn the light on, otherwise turn the light off (note: the threshold to trigger on is found by trial and error)
- within the range of darkness (dusk to no-starlight), dim the lights to match the conditions
- at the darkest condition, the LEDs will be on, at their lowest level
- linearly scale accourding to conditions within the "on" range

## Temperature and Humidity

Currently there's a 4th mode, to take a sensor reading and post it to a web location.  This feature is broken because the power requirements of the Arduino WiFi board cause it to overheat after ~4 hours of runtime, and thus lockup the Arduino loop(), causing the device to stop functioning.  Fixing this is on the backlog!

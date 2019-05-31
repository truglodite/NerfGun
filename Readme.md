# Nerftec... deeper down the rabbit hole.
<img src="https://github.com/truglodite/NerfGun/blob/master/images/nerfTecStryfe1.jpg?raw=true" width="600"><img src="https://github.com/truglodite/NerfGun/blob/master/images/nerfTecStryfe2.jpg?raw=true" width="600">

## Overview
This is a Nerf gun chronograph/computer project, that uses an Arduino Pro-mini (328p/5V/16mhz), a single IR emitter/detector, battery voltage divider, 3 buttons, and an SSD1306 OLED display to show remaining rounds, voltage, fps, and rpm. The code also displays averages and dart data tables with highlighted max and min values for each clip after it is emptied. Code is fairly optimized for performance; it is non-blocking and uses direct port access (versus digitalRead/Write) with Timer1 input capture methods, ported for a 16MHz m328 target. An OPL550 IR sensor with an OP240a IR emitter were tested by author of this code (100ohm on the emitter, ~1" from sensor). The pair are mechanically matched and have sub clock (at 16mhz) response times, which goes well with the +/-65ns input capture methods used (any measurement error is due to dart length variances). Other displays and/or targets may be used... just be mindful of your portD and Timer1 pinouts.

## Operation

### Setup
Boots in to setup mode, where the screen guides the user to adjust clip size and dart length. Upon entering setup, the clip size value is highlighted, and pressing the right or left buttons will adjust capacity. A short click of select will highlight the dart length, which can then be adjusted with the R/L buttons.  Holding the R/L buttons in dart length mode will jump between Mega and Elite defined lengths as a convenience. Live voltage is displayed on the bottom. Holding Select will go to fire mode.
<img src="https://github.com/truglodite/NerfGun/blob/master/images/nerfTecStryfe3.jpg?raw=true" width="600">
### Fire
When entering fire mode, the LED turns off, and live voltage, FPS, and RPM are displayed under remaining rounds. When there is one round remaining, the LED will
turn on and the beeper will emit 1 short beep. When the clip is empty, the beeper emits 3 short beeps and the code enters "Empty Mode".
<img src="https://github.com/truglodite/NerfGun/blob/master/images/nerfTecStryfe4.jpg?raw=true" width="600">
### Empty
Upon entering empty mode, a statistics page is displayed showing Min, Avg, and Max values for FPS, RPM, and Voltage. Live voltage is also show on the bottom of the page. After a few seconds, dart data tables are shown. The tables list round #, volts, FPS, and RPM for all rounds fired, with highlighted max and min values. Hitting the select button returns to fire mode using the previously selected clip size and dart length. Either the right or left button will return to setup mode.
<img src="https://github.com/truglodite/NerfGun/blob/master/images/nerfTecStryfe5.jpg?raw=true" width="600">
### Low Battery
After boot up, voltage is checked, and the low voltage threshold is set to 6.4 or 9.6V, depending the the inferred source (2s lithium or 3s lithium). On
screens where live voltage is available, the value will be highlighted when it drops below the threshold. In all modes, if the battery is low, the beeper will emit one long beep. If voltage remains low, the long beep will repeat after the defined warning delay.

## Pinout
Pin | Connected to
---- | ----------
A0 | Vbatt divider *(default: 5k6:3k3 divider for 13.4V max)*
A4 | OLED SDA
A5 | OLED SCL
D3 | Select button
D4 | Right button
D5 | Left button
D6 | LED indicator
D7 | 5V Buzzer
D8 | IR Sensor signal

*D0-8 are ported for m328 (Uno, etc). Edit code as needed to match portD & Timer1 pinouts for other targets.*

## Installation
The file structure is intended for direct use with PlatformIO. Simply download the zip, extract, open in PIO, and build/upload. The code is compatible with Arduino IDE if you wish to compile with that instead; you must create the necessary file structure for Arduino (sketch name = parent directory name).

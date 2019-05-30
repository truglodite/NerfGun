# Nerftec... deeper down the rabbit hole.

## Overview
A Nerf gun chronograph/computer, which uses an IR emitter/detector and OLED display
to show remaining rounds, voltage, fps, and rpm. This is taken to the next level with
post clip averages and dart data tables with highlighted max and min values. The code
is all non-blocking, uses direct port access (versus digitalRead/Write), and Timer1
input capture methods, ported for a 16MHz m328 target. An OPL550 IR sensor with an
OP240a IR emitter work well for the applications used by author of this code (100ohm
on the emitter >1" from sensor). The pair are mechanically matched and posess a sub
clock response times, which goes well with the +/-65ns input capture methods used
(with a 550/240 set, any measurement error will stem from dart length variances). The
code is also written for use with a Banggood 0.96" 128x64 i2c monochrome OLED screen
(SSD1306). Other displays and/or targets may be used... just be mindful of your portD
and Timer1 pinouts.

## Operation

### Setup
Boots in to setup mode, where the screen guides the user to adjust clip size
and dart length. Upon entering setup, the clip size value is highlighted, and pressing
the right or left buttons will adjust capacity. A short click of select will highlight
the dart length, which can then be adjusted with the R/L buttons.  Holding the R/L buttons
in dart length mode will jump between Mega and Elite defined lengths as a convenience.
Live voltage is displayed on the bottom. Holding Select will go to fire mode.

### Fire
When entering fire mode, the LED turns off, and live voltage, FPS, and RPM
are displayed under remaining rounds. When there is one round remaining, the LED will
turn on and the beeper will emit 1 short beep. When the clip is empty, the beeper emits
3 short beeps and the code enters "Empty Mode".

### Empty
Upon entering empty mode, a statistics page is displayed showing Min, Avg,
and Max values for FPS, RPM, and Voltage. Live voltage is also show on the bottom of the
page. After a few seconds, dart data tables are shown. The tables list round #, volts,
FPS, and RPM for all rounds fired, with highlighted max and min values. Hitting the
select button returns to fire mode using the previously selected clip size and dart
length. Either the right or left button will return to setup mode.

### Low Battery
After boot up, voltage is checked, and the low voltage threshold is
set to 6.4 or 9.6V, depending the the inferred source (2s lithium or 3s lithium). On
screens where live voltage is available, the value will be highlighted when it drops
below the threshold. In all modes, if the battery is low, the beeper will emit one long
beep. If voltage remains low, the long beep will repeat after the defined warning delay.

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

* D0-8 are ported for m328 (Uno, etc). *
* Edit code as needed to match portD & Timer1 pinouts for other targets. *

#define versionNo   "1.0"
// nerfGun_.ino
// by: Truglodite
// 5/29/2019
// Nerftec... deeper down the rabbit hole.
//
// Note: U=T, with a workaround for input capture interrupts resulting in
// failure of the 'last buzzer off' command. Version ID is also #defined for
// convenience.

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>
#include "logo.h"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//Vin = ADCoutput / vbattRatio (theoretically, vbattRatio = 1023*R2/(5*(R1+R2))
//calibrate vbattRatio... first upload with vbattRatio = 100.0. Then using a DMM,
//find the calibrated value: vbattRatio = 100.0 * [DMM Volts]/[Displayed Volts]

const float vbattRatio =          75.7;        //theoretically 83.2 for a 5k6:3k3 divider
unsigned int dartLength =         2850;        //Default = elite darts [1/1000 inch]
uint8_t clipSize =                10;          //Default clip capacity
#define maxClipSize               35           //Controls size of dart data arrays (uses up memory!!!)
#define vbattPin                  A0           //vbatt divider (A0)
#define vbattLow2s                7.0          //Low battery voltage threshold for 2s lipo
#define vbattLow3s                10.5         //Low battery threshold for 3s lipo
#define lipo2sHigh                8.4          //Voltage above which vbattLow = vbattLow3s
#define buttonDelayNorm           250          //mSec before registering a button as held (also a debounce)
#define buttonDelayShort          80           //mSec between held button repeats
#define screenDelayLong           4000         //mSec to display a detailed density screen
#define screenDelayMed            3000         //mSec to display a standard density screen
#define voltCheckDelay            500          //mSec between live voltage updates
#define beepDelayShort            50           //mSec for short beeps
#define beepDelayLong             1000         //mSec for long beeps (low voltage)
#define warningDelay              10000        //msec between low voltage beeps
#define dartLengthMega            3700         //Jumps to when below this value with long press
#define dartLengthElite           2850         //Jumps to when above this value with long press
#define splashScreen                           //Uncomment to enable splash screen (~15% of progmem)

//Globals--------------------------------------
bool lowBattWarning = 0;                       //Low battery alarm state, off
bool beepState = 0;                            //Beeper state, off
bool longSelect = 0;                           //Saves previous status of select button for long/short presses
bool longLeft = 0;                             //Saves previous status of left button for long/short presses
bool longRight = 0;                            //Saves previous status of right button for long/short presses

uint8_t mode = 0;                              //Mode switch, start in setup mode (0)
uint8_t setupItem = 0;                         //Switch for selected items in setup mode (0)
uint8_t currentScreen = 0;                     //Index for switching pages in empty mode
uint8_t currentRound = 0;                      //Index for rotating data tables in empty mode
uint8_t remaining = 0;                         //Remaining rounds in fire mode
uint8_t beepCount = 0;                         //Beep counter
uint8_t fastestRound = 0;                      //Saves max FPS round #
uint8_t slowestRound = 0;                      //...   min FPS...
uint8_t fastestRate = 0;                       //...   max RPM...
uint8_t slowestRate = 0;                       //...   min RPM...
uint8_t highestVolt = 0;                       //...   max Volt...
uint8_t lowestVolt = 0;                        //...   min Volt...

unsigned int dartSpeedData[maxClipSize];       //Dart data storage
unsigned int dartRPMdata[maxClipSize];         //Dart data storage
unsigned int dartVoltData[maxClipSize];        //Dart data storage
unsigned int liveVoltage = 0;                  //Raw ADC reading
unsigned int vBattLow = 0;                     //Holder for low voltage threshold (raw adc)

unsigned long previousWarning = 0;             //Time saver for low battery warnings
unsigned long previousVoltCheck = 0;           //Time saver for battery checks
unsigned long previousScreenTime = 0;          //Time saver for empty screens
unsigned long screenDelay = 0;                 //Time to show automated pages (empty mode)
unsigned long dartTime = 0;                    //Saves micros() for RPM
unsigned long previousDartTime = 0;            //Saves micros() for RPM
unsigned long clipStartTime = 0;               //Saves micros() for average RPM
unsigned long beepTime = 0;                    //Beep time saver
unsigned long beepDelay = 0;                   //Beep length saver
unsigned long buttonDelay = buttonDelayNorm;
unsigned long previousButtonTime = 0;          //Time saver for buttons

volatile boolean first;                        //Flag for ISR modes (startTime or finishTime)
volatile boolean triggered;                    //Flag for ISR to ignore additional interrupts until the code saves them
volatile unsigned long overflowCount;          //Timer1 overflow count
volatile unsigned long startTime;              //Timer1 count when the dart head blocks the sensor
volatile unsigned long finishTime;             //Timer1 count when the dart tail clears the sensor

//Setup Mode Display update..............................................
void updateSetupScreen()  {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(F("Clip Size:  "));    //1-11,(1)

  display.setTextSize(2);             //11-16,(1-2)
  if(clipSize < 10) {
    display.print(F("   "));
  }
  else if(clipSize < 100) {
    display.print(F("  "));
  }
  if(!setupItem)  {                   //Clip size selected
    display.setTextColor(BLACK,WHITE);
  }
  display.println(clipSize);

  float dartLengthy = float(dartLength)/1000.0;
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.print(F("Dart Length:"));    //1-12,(3)
  display.setTextSize(2);              //12-16,(4-5)
  if(setupItem == 1)  {                //Clip size selected
    display.setTextColor(BLACK,WHITE);
  }
  display.println(dartLengthy,2);

  display.setTextSize(1);
  display.setTextColor(BLACK,WHITE);
  display.println();
  //               12345678901234567890
  display.println(F("<[-]  [Next]  [+]>"));//(6)
  display.println();                   //(7)
  display.setTextColor(WHITE);
  display.print(F("  Battery: "));     //(8)
  if(liveVoltage < vBattLow) {
    display.setTextColor(BLACK,WHITE);
  }
  float voltFloat = (float)liveVoltage / vbattRatio;
  display.print(voltFloat,2);
  display.println(F(" V"));
  display.display();
}

//Fire Mode Display update...................................................
void updateFireScreen(unsigned int rpm, unsigned int fps)  {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  display.setTextSize(6);             //Print remaining rounds
  display.print(F(" "));              //6 "rows"
  if(remaining < 10) {
    display.print(F(" "));
  }
  display.println(remaining);

  display.setTextSize(1);             //21x8
  display.setTextColor(BLACK, WHITE); //Print Header Row
                   //123456789012345678901
  display.println(F(" BATT    RPM     FPS "));

                                      //Convert int*100's to float's
  float voltFloat = (float)liveVoltage / vbattRatio;
  float fpsFloat = (float)fps / 100.0;
  float rpmFloat = (float)rpm / 100.0;

  if(liveVoltage > vBattLow)  {       //Print voltage non-inverted if OK
    display.setTextColor(WHITE);
  }                                   //...otherwise it stays highlighted
  display.print(F("  "));             //1-2
  if(voltFloat < 10)  {
   display.print(voltFloat,2);        //3-6
  }
  else  {
    display.print(voltFloat,1);       //3-6
  }
  display.setTextColor(WHITE);        //Set to non-inverted again, in case of low voltage

  display.print(F("   "));            //Print fire rate, 7-9
  if(rpmFloat < 10) {                 //10-13
    display.print(rpmFloat,2);
  }
  else if(rpmFloat < 100) {
    display.print(rpmFloat,1);
  }
  else if(rpmFloat < 1000) {
    display.print(F(" "));
    display.print((int)rpmFloat);
  }
  else if(rpmFloat >= 1000) {
    display.print((int)rpmFloat);
  }

  display.print(F("   "));            //14-16
                                      //Print velocity
  if(fpsFloat < 10) {                 //17-20
    display.print(fpsFloat,2);
  }
  else if(fpsFloat < 100) {
    display.print(fpsFloat,1);
  }
  else if(fpsFloat < 1000) {
    display.print(F(" "));
    display.print((int)fpsFloat);
  }
  else if(fpsFloat >= 1000) {
    display.print((int)fpsFloat);
  }
  display.display();
}

//Read digital buttons & return a value.........................................
uint8_t readButtons(void) {
  if(!(PIND & 0b00001000))  {         //Select button
    return 1;
  }
  else if(!(PIND & 0b00010000))  {    //Right button
    return 2;
  }
  else if(!(PIND & 0b00100000))  {    //Left button
    return 3;
  }
  else  {                             //No buttons
    return 0;
  }
}

//Update beeper: reads global beep variables, and toggles the beeper accordingly
//set beepCount = 2 * desired#ofBeeps
//set beepDelay = desired length of beep in mSec
//note: length of beep = length of silence between repeated beeps
void beeper(void) {
  //beepCount = 0 and buzzer is still on...
  if(!beepCount && (PIND & 0b10000000))  {//For the rare case when 'beep off' is 'skipped' during a timer interrupt
    PORTD = PORTD & 0b01111111;        //Buzzer off again
  }
  //We have beeps to do, and it's time to act on them
  if(beepCount && millis() - beepTime > beepDelay) {
    if(beepState == 0) {
      PORTD = PORTD | 0b10000000;      //Buzzer on
      beepState = 1;
    }
    else  {
      PORTD = PORTD & 0b01111111;      //Buzzer off
      beepState = 0;
    }
    beepCount--;
    beepTime = millis();
  }
}

//Timer overflow counter ISR (ICR1 overflows every 65536 counts).....................
ISR (TIMER1_OVF_vect) {
  overflowCount++;
}

//Timer capture ISR.............................................................
ISR (TIMER1_CAPT_vect)  {
                                      //Get counter value before it changes any more
  unsigned int timer1CounterValue;
  timer1CounterValue = ICR1;          //Timer1 input capture count register
  unsigned long overflowCopy = overflowCount;

                                      //Just missed an overflow...
  if ((TIFR1 & bit (TOV1)) && timer1CounterValue < 0x7FFF)
    overflowCopy++;

  if (triggered)  return;             //Wait until the last capture is processed

  if (first)    {                     //This is the falling edge
    startTime = (overflowCopy << 16) + timer1CounterValue;
    TIFR1 |= bit (ICF1);              //Clear Timer1 input capture count flag
    TCCR1B =  bit (CS10) | bit (ICES1);//No prescaler + capture rising edge
    first = false;
    return;
  }
                                      //This must be the rising edge
  finishTime = (overflowCopy << 16) + timer1CounterValue;
  triggered = true;
  TIMSK1 = 0;                         //Disable Timer1 interrupts
}

//Reset timer1 registers for the next capture...............................................
void prepareForInterrupts ()  {
  noInterrupts ();                    //Protected code
  first = true;                       //Re-arm for next time
  triggered = false;
  TCCR1A = 0;                         //Reset Timer1
  TCCR1B = 0;
  TIFR1 = bit (ICF1) | bit (TOV1);    //Clear Timer1 input capture and overflow flags
  TCNT1 = 0;                          //Reset counter
  overflowCount = 0;                  //Reset overflows
                                      //Timer1 - counts clock pulses
  TIMSK1 = bit (TOIE1) | bit (ICIE1); //Enable Timer1 overflow and input capture interrupts
  TCCR1B =  bit (CS10);               //Start Timer1, with no prescaler + capture falling edge
  interrupts ();
}

//SETUP----------------------------------------
void setup() {
  analogReference(DEFAULT);                    //5V reference for the default divider & input range

  //Atmega328 pin config & init (there may be more work below... ;-) )
  DDRD = DDRD & 0b11000111;                    //PortD- pins 3-5 as inputs (buttons)
  DDRD = DDRD | 0b11000000;                    //PortD- pins 6&7 as outputs (led and beeper)
  PORTD= PORTD & 0b01111111;                   //turn off beeper
  PORTD= PORTD | 0b01111000;                   //pins 3-5 activate pullups (buttons), and turn on pin 7 (LED)

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);   //Banggood Monochrome i2c OLED

  #ifdef splashScreen
    //display.display();                         //Adafruit props...
    //delay(1000);

    #define h 64
    #define w 128


    //Flashing Nerftec logo
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(200);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(200);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(150);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(150);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(100);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(100);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(50);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(50);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(25);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(25);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(10);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(10);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(5);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(5);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftec_bmp, w, h, WHITE);
    display.display();
    delay(1);
    display.clearDisplay();
    display.drawBitmap( 0, 0, nerftecInvert_bmp, w, h, WHITE);
    display.display();
    delay(1);

    //Author's autobot logo
    display.clearDisplay();
    display.drawBitmap( 0, 0, bykev_bmp, w, h, WHITE);
    display.display();
    delay(2000);

    //Some text infos
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.setTextColor(WHITE);
    display.print(F(" "));                    //(1,2)
    display.setTextColor(BLACK,WHITE);
                     //1234567890
    display.println(F("Nerftec"));
    display.setTextSize(1);
                     //123456789012345678901
    display.setTextColor(WHITE);
    display.println();                        //(3)
    display.print(F("   Version - "));        //(4)
    display.println(F(versionNo));
    display.println();                        //(5)
    display.println();                        //(6)
    display.println(F("For the nerf"));       //(7)
                   //123456789012345678901
    display.print(F("     connoisseur! ;-)"));//(8)
    display.display();
    delay(2000);
  #endif

  //Read the ADC and set vBattLow appropriately...
  float volts = float(analogRead(vbattPin)) / vbattRatio;
  if(volts < 5.3)  vBattLow = 0.0;            //<5.3V = usb power... battery may be disconnected
  else if(volts > lipo2sHigh)  vBattLow = vbattLow3s * vbattRatio;//More than 2s, must be 3s
  else  vBattLow = vbattLow2s * vbattRatio;   //If it's 3s, it must be 2s

  updateSetupScreen();                        //Initialize setup mode display
}//END OF SETUP

//LOOP----------------------------------------
void loop() {
  switch (mode)  {
      case 0:  {  //'setup' Mode...
                                              //Time to reset the battery warning
          if(lowBattWarning && millis() - previousWarning > warningDelay)  {
            lowBattWarning = 0;               //Ready to beep again if battery reads low
          }
                                              //No active beepers, and it's time to check voltage
          if(!beepCount && millis() - previousVoltCheck > voltCheckDelay)  {
            liveVoltage = analogRead(vbattPin);//Read Vold adc
            previousVoltCheck = millis();     //Update timer
                                              //Battery is low and we haven't warned yet
            if(!lowBattWarning && liveVoltage <= vBattLow)  {
              beepCount = 2;                  //1 long beep
              beepDelay = beepDelayLong;
              previousWarning = millis();
              lowBattWarning = 1;
            }
            updateSetupScreen();              //Update voltage on screen
          }

          beeper();                           //Update the beeper
          uint8_t buttonValue = readButtons();//Read buttons

                                              //Button debounce & hold/repeat timer
          if(millis() - previousButtonTime > buttonDelay) {
            if(!buttonValue) {                //No buttons...
              longSelect = 0;                 //Reset long press variables
              longLeft = 0;
              longRight = 0;
              buttonDelay = buttonDelayNorm;
            }
            else  {                             //We have a button
              previousButtonTime = millis();    //Save the time

              if(buttonValue <= 1) {            //Select button
                if(longSelect)  {               //We have a long press... go to fire mode
                  remaining = clipSize;         //Clip should be loaded
                  PORTD = PORTD & 0b10111111;   //Turn off LED
                  mode = 1;                     //Switch to fire mode
                  updateFireScreen(0.0,0.0);    //Initialized fire mode screen
                  prepareForInterrupts ();      //Get interrupts ready
                  longSelect = 0;               //Reset for a possible next setup mode
                  break;
                }
                else  {                         //Every time we se select...
                  longSelect = 1;               //Flag for a long press
                  if(setupItem < 1)  {          //Cycle to next item
                    setupItem++;
                  }
                  else  setupItem = 0;
                }
              }

              else if(buttonValue <= 2 && setupItem == 0) {//Right button, item 0...
                if(longRight)  {
                  buttonDelay = buttonDelayShort;
                }
                clipSize++;                     //Increase clip size
                longRight = 1;
                if(clipSize > maxClipSize) {    //Limit to maxClipSize
                  clipSize = maxClipSize;       //Enforce
                  beepCount = 2;                //1 short beep
                  beepDelay = beepDelayShort;
                }
                remaining = clipSize;
              }

              else if(buttonValue <= 2 && setupItem == 1) {//Right button, item 1...
                if(longRight && dartLength < dartLengthMega)  {
                  dartLength = dartLengthMega-1;   //Long right, jump to bigger size if smaller
                }
                else if(longRight)  {
                  buttonDelay = buttonDelayShort;
                }
                dartLength=dartLength+10;
                if(dartLength > 9994) {
                  dartLength = 9994;
                }
                longRight = 1;
              }

              else if(buttonValue <= 3 && setupItem == 0) {//Left button, item 0
                if(longLeft)  {
                  buttonDelay = buttonDelayShort;
                }
                clipSize--;                     //Decrease clip size
                longLeft = 1;
                if(clipSize < 1) {              //Limit to smallest clip
                  clipSize = 1;                 //Enforce
                  beepCount = 2;                //1 short beep
                  beepDelay = beepDelayShort;
                }
                remaining = clipSize;
              }
              else if(buttonValue <= 3 && setupItem == 1) {//Left button, item 1
                if(longLeft && dartLength > dartLengthElite)  {
                  dartLength = dartLengthElite+1;//Long left, jump to smaller size if bigger
                }
                else if(longLeft)  {
                  buttonDelay = buttonDelayShort;
                }
                dartLength=dartLength-10;
                if(dartLength < 10) {
                  dartLength = 10;
                }
                longLeft = 1;
              }
              updateSetupScreen();
            }//end of button processing
          }//end button timer
        }//end of setup mode
        break;

      case 1:  {//'Fire' mode... capture and display each dart
                                               //It's been a while since we warned of a low battery...
          if(millis() - previousWarning > warningDelay)  {
            lowBattWarning = 0;                //Ready to beep again if battery reads low
          }
                                               //It's time to update live voltage, and no other beepers are running
          if(millis() - previousVoltCheck > voltCheckDelay && !beepCount)  {
            liveVoltage = analogRead(vbattPin);//Read adc
                                               //We have a low battery...
            if(liveVoltage <= vBattLow && !lowBattWarning)  {
              beepCount = 2;
              beepDelay = beepDelayLong;
              previousWarning = millis();
              lowBattWarning = 1;
            }
            updateFireScreen(dartRPMdata[clipSize - remaining-1],dartSpeedData[clipSize - remaining-1]);
            previousVoltCheck = millis();
          }

          beeper();                            //Update the beeper

          if (triggered) {                     //We have new data from the ISR...
            dartTime = micros();               //Save Timer0 for RPM related stuff
                                               //Read adc and store voltage
            dartVoltData[clipSize - remaining] = analogRead(vbattPin);
            unsigned long elapsedTime = finishTime - startTime;//Calculate # of ticks it took for dart to go by
            prepareForInterrupts ();           //Done with the volatiles, get interrupts ready again

            //elapsedTime = (1000000 * elapsedTime) / F_CPU;//Calculate ET in microseconds
            float microsecs = (float)elapsedTime * 62.5e-9 * 1.0e6;//Calculate ET in microseconds

                                                //Calculate and save FPS * 100
            dartSpeedData[clipSize - remaining] = dartLength * 100000 / (12 * microsecs);
                                                //Calc RPM*100
            if(remaining == clipSize) {         //On the first shot only...
              clipStartTime = dartTime;         //Save clip timer
              dartRPMdata[clipSize - remaining] = 0;//Zero RPM for the first round
            }
            else {                              //The rest of the shots...
              dartRPMdata[clipSize - remaining] = 6000000000 / (dartTime - previousDartTime);//Calc and save instant RPM
            }
            remaining--;                        //Decrease rounds remaining
                                                //Update fire screen now (with updated remaining, thus the -1
            updateFireScreen(dartRPMdata[clipSize - remaining-1],dartSpeedData[clipSize - remaining-1]);
            previousDartTime = dartTime;        //Save dart timer

            if(remaining == 1) {                //One round left...
              beepCount = 2;                    //1 short beep
              beepDelay = beepDelayShort;
              PORTD = PORTD | 0b01000000;       //LED on
            }

            if(remaining == 0) {                //Clip empty...
              beepCount = 6;                    //3 short beeps
              beepDelay = beepDelayShort;
              previousScreenTime = millis();          //So we show the last fire mode screen
              screenDelay = screenDelayMed;     //...for a bit
              mode = 2;                         //Switch to 'Empty' mode
            }
          }//done processing new data
        }//end of fire mode... cases {} wrapped to control variable scope (and SRAM usage)
        break;

      case 2:  { //"Empty" mode... Scroll dart data
                                               //Ready to beep again if battery reads low
          if(millis() - previousWarning > warningDelay && lowBattWarning)  {
            lowBattWarning = 0;
          }
                                               //It's time to update live voltage, and no other beepers are running
          if(millis() - previousVoltCheck > voltCheckDelay && !beepCount)  {
            liveVoltage = analogRead(vbattPin);//Read adc
                                               //We have a low battery & haven't warned...
            if(liveVoltage <= vBattLow && !lowBattWarning)  {
              beepCount = 2;
              beepDelay = beepDelayLong;
              previousWarning = millis();
              lowBattWarning = 1;
            }
            previousVoltCheck = millis();
          }

          beeper();                           //Update the beeper

          uint8_t buttonValue = readButtons();//Read the buttons
          if(buttonValue == 1) {              //Select button
            remaining = clipSize;             //Refill the clip
            screenDelay = screenDelayMed;
            currentScreen = 0;                //Reset empty mode indexes
            currentRound = 0;
            updateFireScreen(0,0);            //Show initialized fire screen
            PORTD = PORTD & 0b10111111;       //Turn off LED
            prepareForInterrupts ();          //...in case something triggered it while reloading
            mode = 1;                         //Switch to fire mode
            break;                            //Break early
          }
          if(buttonValue == 2 || buttonValue == 3) {//Right or Left buttons
            screenDelay = screenDelayMed;
            currentScreen = 0;                //Reset empty mode indexes
            currentRound = 0;
            mode = 0;                         //Switch to setup mode
            previousButtonTime = millis();    //Update timer so we don't change values upon entrering setup mode
            break;                            //Break early
          }
                                              //If it is time, show the next screen
          if(millis() - previousScreenTime > screenDelay)  {
            if(currentScreen <= 0)  {         //First screen: min/avg/max V, FPS, & RPM, and live voltage
              float voltFloat = (float)liveVoltage / vbattRatio;//Convert to float Volts
              float avgVolts = 0.0;           //Calculate average Voltage for clip
              float avgFps = 0.0;             //Calculate average FPS for clip
              for(uint8_t i=0; i<clipSize; i++) {
                avgVolts += (float)dartVoltData[i]/vbattRatio;
                avgFps += (float)dartSpeedData[i]/100.0;
              }
              avgVolts = avgVolts / (float)clipSize;
              avgFps = avgFps / (float)clipSize;
                                              //Calculate average RPM for clip
              float avgRpm = 60000000.0 * (float)(clipSize - 1) / (float)(dartTime - clipStartTime);

              float maxFps = 0.0;             //Find max FPS and save it
              float minFps = 1000000.0;       //...  min FPS...
              float maxRpm = 0.0;             //...  max RPM...
              float minRpm = 1000000.0;       //...  min RPM...
              float maxVolt = 0.0;            //...  max V...
              float minVolt = 1000000.0;      //...  min V...
              for(uint8_t i=0; i<clipSize; i++) {
                if((float)dartSpeedData[i] > maxFps) {
                  maxFps = (float)dartSpeedData[i];
                  fastestRound = i;
                }
                if((float)dartSpeedData[i] < minFps) {
                  minFps = (float)dartSpeedData[i];
                  slowestRound = i;
                }
                if((float)dartRPMdata[i] > maxRpm) {
                  maxRpm = (float)dartRPMdata[i];
                  fastestRate = i;
                }
                if((float)dartRPMdata[i] < minRpm && i > 0) {
                  minRpm = (float)dartRPMdata[i];
                  slowestRate = i;
                }
                if((float)dartVoltData[i] > maxVolt) {
                  maxVolt = (float)dartVoltData[i];
                  highestVolt = i;
                }
                if((float)dartVoltData[i] < minVolt) {
                  minVolt = (float)dartVoltData[i];
                  lowestVolt = i;
                }
              }
              maxFps = maxFps / 100.0;
              minFps = minFps / 100.0;
              maxRpm = maxRpm / 100.0;
              minRpm = minRpm / 100.0;
              maxVolt = maxVolt / vbattRatio;
              minVolt = minVolt / vbattRatio;

              display.clearDisplay();
              display.setTextSize(2);         //10x5?
              display.setTextColor(WHITE);
              display.setCursor(0,0);
              display.println(F("Statistics"));//Title row (1-2)
              display.setTextSize(1);

              display.setTextColor(BLACK,WHITE);//Header row (3)
                               //123456789012345678901
              display.println(F("Desc MIN   AVG   MAX "));

              display.print(F("FPS "));       //FPS line (4), 1-4
              display.setTextColor(WHITE);
              display.print(F(" "));          //padding, 4-5
              if(minFps < 10) {               //Mininum FPS, 6-9
                display.print(minFps,2);
              }
              else if(minFps < 100) {
                display.print(minFps,1);
              }
              else if(minFps < 1000) {
                display.print(F(" "));
                display.print((int)minFps);
              }
              else if(minFps >= 1000) {
                display.print((int)minFps);
              }

              display.print(F("  "));         //Padding, 10-11
              if(avgFps < 10) {               //Average FPS, 12-15
                display.print(avgFps,2);
              }
              else if(avgFps < 100) {
                display.print(avgFps,1);
              }
              else if(avgFps < 1000) {
                display.print(F(" "));
                display.print((int)avgFps);
              }
              else if(avgFps >= 1000) {
                display.print((int)avgFps);
              }

              display.print(F("  "));         //Padding, 15-16
               if(maxFps < 10) {              //Maximum FPS, 18-21
                display.println(maxFps,2);
              }
              else if(maxFps < 100) {
                display.println(maxFps,1);
              }
              else if(maxFps < 1000) {
                display.print(F(" "));
                display.println((int)maxFps);
              }
              else if(maxFps >= 1000) {
                display.println((int)maxFps);
              }

              display.setTextColor(BLACK,WHITE);
              display.print(F("RPM "));       //RPM line (5), 1-4
              display.setTextColor(WHITE);
              display.print(F(" "));          //padding, 4-5
              if(minRpm < 10) {               //Minimum RPM, 6-9
                display.print(minRpm,2);
              }
              else if(minRpm < 100) {
                display.print(minRpm,1);
              }
              else if(minRpm < 1000) {
                display.print(F(" "));
                display.print((int)minRpm);
              }
              else if(minRpm >= 1000) {
                display.print((int)minRpm);
              }

              display.print(F("  "));         //Padding 10-11
              if(avgRpm < 10) {               //Average RPM, 12-15
                display.print(avgRpm,2);
              }
              else if(avgRpm < 100) {
                display.print(avgRpm,1);
              }
              else if(avgRpm < 1000) {
                display.print(F(" "));
                display.print((int)avgRpm);
              }
              else if(avgRpm >= 1000) {
                display.print((int)avgRpm);
              }

              display.print(F("  "));         //Padding 16-17
              if(maxRpm < 10) {               //Maximum RPM, 18-21
                display.println(maxRpm,2);
              }
              else if(maxRpm < 100) {
                display.println(maxRpm,1);
              }
              else if(maxRpm < 1000) {
                display.print(F(" "));
                display.println((int)maxRpm);
              }
              else if(maxRpm >= 1000) {
                display.println((int)maxRpm);
              }

              display.setTextColor(BLACK,WHITE);
              display.print(F("BATT"));       //Voltage line (6), 1-4
              display.setTextColor(WHITE);
              display.print(F(" "));          //padding, 4-5
              if(minVolt < 10) {              //Minimum voltage, 6-9
                display.print(minVolt,2);
              }
              else  {
                display.print(minVolt,1);
              }
              display.print(F("  "));         //Padding 10-11
              if(avgVolts < 10) {             //Average voltage, 12-15
                display.print(avgVolts,2);
              }
              else  {
                display.print(avgVolts,1);
              }
              display.print(F("  "));         //Padding 16-17
              if(maxVolt < 10) {             //Maximum voltage, 18-21
                display.println(maxVolt,2);
              }
              else  {
                display.println(maxVolt,1);
              }

              display.println();              //line (7)
              display.print(F("Live Batt:     "));//Live voltage line (8), 1-15
              if(liveVoltage <= vBattLow)  {  //Print voltage inverted if low
                  display.setTextColor(BLACK,WHITE);
                }
              if(voltFloat < 10) {          //Live voltage, 16-19
                display.print(voltFloat,2);
              }
              else  {
                display.print(voltFloat,1);
              }
              display.setTextColor(WHITE);    //In case of low battery
              display.print(F(" V"));         //20-21

              display.display();
              screenDelay = screenDelayLong;   //Show stats for a short time
              currentScreen = 1;
              previousScreenTime = millis();
            }//end screen 0

            else if(currentScreen <= 1)  {     //2nd+ screen(s)... dart data
              display.clearDisplay();
              display.setTextSize(1);          //21x8
              display.setTextColor(BLACK, WHITE);
              display.setCursor(0,0);
              display.println(F(" RND VOLT  FPS   RPM "));//Header row
              //                 123456789012345678901
              display.setTextColor(WHITE);
              for(int i=0 ; i<7 ; i++) {       //7 rows of data per screen (under the header row)
                if(currentRound < clipSize) {  //Check that we only print a row when data exists
                  if(currentRound+1 < 10) {    //Left side padding
                    display.print(F("  "));
                  }
                  else {
                    display.print(F(" "));
                  }
                  display.print(currentRound+1);//Print Round #, 1-3
                  display.print(F("  "));      //Round-Volts padding, 4-5

                                               //Convert int*100's to float's
                  float voltFloat = (float)dartVoltData[currentRound] / vbattRatio;
                  float fpsFloat = (float)dartSpeedData[currentRound] / 100.0;
                  float rpmFloat = (float)dartRPMdata[currentRound] / 100.0;

                  if(currentRound == lowestVolt || currentRound == highestVolt) {//Highlight max & min volts
                    display.setTextColor(BLACK,WHITE);
                  }
                  if(voltFloat < 10) {         //Print Volts, 6-9
                    display.print(voltFloat,2);
                  }
                  else {
                    display.print(voltFloat,1);
                  }
                  display.setTextColor(WHITE);

                  display.print(F("  "));      //Volts-FPS padding, 10-11
                  if(currentRound == fastestRound || currentRound == slowestRound) {//Highlight max & min velocities
                    display.setTextColor(BLACK,WHITE);
                  }
                  if(fpsFloat < 10) {          //Print Velocity
                    display.print(fpsFloat,2);//12-15
                  }
                  else if(fpsFloat < 100) {
                    display.print(fpsFloat,1);
                  }
                  else if(fpsFloat < 1000) {
                    display.print((int)fpsFloat);
                    display.print(F("."));
                  }
                  else {
                    display.print((int)fpsFloat);
                  }
                  display.setTextColor(WHITE);

                  display.print(F("  "));      //FPS-RPM padding, 16-17
                  if(currentRound == fastestRate || currentRound == slowestRate) {//Highlight max & min rates
                    display.setTextColor(BLACK,WHITE);
                  }
                  if(rpmFloat < 10) {          //Print RPM, 18-21
                    display.println(rpmFloat,2);
                  }
                  else if(rpmFloat < 100) {
                    display.println(rpmFloat,1);
                  }
                  else if(rpmFloat < 1000) {
                    display.print((int)rpmFloat);
                    display.println(F("."));
                  }
                  else if(rpmFloat >= 1000) {
                    display.println((int)rpmFloat);
                  }
                  display.setTextColor(WHITE);
                }
                currentRound++;
              }
              if(currentRound >= clipSize) {   //No more dart data
                currentScreen = 0;             //Go back to screen 0
                currentRound = 0;              //Reset line index for next time around
              }
              screenDelay = screenDelayLong;   //Show dart data screens a longer time
              display.display();
              previousScreenTime = millis();
            }//end screen 2
          }//end screen timer
        }//end empty mode
        break;
      default:  //This should never happen...
          mode = 0;
        break;
  }
}//END OF LOOP

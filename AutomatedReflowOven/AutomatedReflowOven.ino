    /***************************************************
 * Title:      Reflow Oven
 * Version:    v1.0
 * Date:       2018-10-21
 * Author:     Taavet Kangur <taavetk@gmail.com>
 * Website:     
 * Licence:    LGPL
 * 
 * Designed working on a Super Delice Oven
 * With this low tempreture soldering paste : 
 * http://www.chipquik.com/datasheets/SMDLTLFP.pdf
 * Obtained through Digikey :
 * https://www.digikey.ch/product-detail/en/chip-quik-inc/SMDLTLFP/SMDLTLFP-ND/2682721
 * Designed to work with the Adafruit Thermocouple Sensor:  
 * https://www.adafruit.com/products/269
    
 ****************************************************/
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <prismino.h>


// default temperature with an offset to meet the mentionned soldering paste
// heating curvature
#define TEMPTOREACH         180
#define TEMPCOOL            40
#define HEATING             1
#define FAN                 4  
#define START_BUTTON        6
#define EMERGENCY_BUTTON    7
#define START_LIGHT         A1
#define EMERGENCY_LIGHT     A2
#define LIMIT_SWITCH        0    

// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO        A5  
#define MAXCS        A4  
#define MAXCLK       A3  

#define OvenOn          digitalWrite(HEATING, LOW)
#define OvenOff         digitalWrite(HEATING, HIGH)

#define FanOn           digitalWrite(FAN, LOW)
#define FanOff          digitalWrite(FAN, HIGH)

// Default connection is using software SPI, but comment and uncomment one of
// the two examples below to switch between software SPI and hardware SPI:

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display


// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

int status = 0;
bool limitswitch = !digitalRead(LIMIT_SWITCH);

//Emergency stop button linked to the Shield's button for fastest stop
void button()
{
  // toggle EMERGENCY STOP : stop the heating and the fan
    FanOff;
    OvenOff;
    status = 6;
}

Stepper myStepper;

void setlimitswitch()
{
    limitswitch = !digitalRead(LIMIT_SWITCH);
}
    
void openOven()
{
    myStepper.moveSteps(-2000, 500);
    while(myStepper.isBusy());          ///Never remove
} 

void closeOven()
{   
    while(limitswitch == 0)
    {
        myStepper.moveSteps(200, 500);
        while(myStepper.isBusy());      ///Never remove
    }
} 
void changeLimitSwitchDown()
{
    limitswitch = !digitalRead(LIMIT_SWITCH);
}

void setup() {

    Serial.begin(9600);

    lcd.init();                      // initialize the lcd 
    lcd.backlight();
    
    //start button
    pinMode(START_BUTTON,           INPUT_PULLUP);
    buttonCallback(button);
    
    pinMode(LIMIT_SWITCH,           INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LIMIT_SWITCH), changeLimitSwitchDown, CHANGE);
        
    //Pin 1 and 2 respectively active the heating and the oven's fan
    pinMode(HEATING,                OUTPUT);
    pinMode(FAN,                    OUTPUT);
    
    //Pin A1 and A2 are used to light up the start (Green) and stop (Red) buttons
    pinMode(START_LIGHT,            OUTPUT);
    pinMode(EMERGENCY_LIGHT,        OUTPUT);
    
    //starts the button lit up
    digitalWrite(START_LIGHT,       LOW);
    digitalWrite(EMERGENCY_LIGHT,   LOW);
    

    
    Serial.println("Automated Reflow Oven");
    // wait for MAX chip to stabilize
    myStepper.setPosition(0);
    delay(1000);
}

void loop() 
{
    // basic readout test, just print the current temp
    //Serial.print("Internal Temp = ");
    //Serial.println(thermocouple.readInternal());  
    double temperature = thermocouple.readCelsius();
    if (isnan(temperature)) 
    {
        Serial.println("Something wrong with thermocouple!");
        lcd.print("Something wrong with thermocouple!");
    } 
    else 
    {
        /*
        lcd.print("C = ");
        lcd.println(temperature);
        Serial.print("C = ");
        Serial.println(temperature);*/
    }
    
    switch(status) 
    {
        case 0:             ///check if oven is closed
            OvenOff;
            FanOff;
            Serial.println("Checking is closed");
            lcd.clear();
            if(limitswitch == 1){
                status = 1;
                }
            else{
                lcd.setCursor(2,0);
                lcd.print("Closing oven");
                lcd.setCursor(1, 1);
                lcd.print("Wait a bit...");
                closeOven();
                }
            break;
        case 1:
            Serial.println("Press green button to open oven");
            lcd.clear();
            lcd.setCursor(2,0);
            lcd.print("Press green");
            lcd.setCursor(1, 1);
            lcd.print("button to open");
            if(digitalRead(START_BUTTON) == LOW && limitswitch == 1) {
                openOven();
                status = 2;
                delay(500);
                }
            break;
        case 2:
            Serial.println("Waiting on button press to start reflow process");
            lcd.clear();
            lcd.setCursor(1,0);
            lcd.println("Press green to ");
            lcd.setCursor(2,1);
            lcd.println("start reflow  ");
            if(digitalRead(START_BUTTON) == LOW) {
                    closeOven();
                    delay(1000);
                    status = 3;
                }
            
            break;
        case 3:
            Serial.println("Heating oven to");
            Serial.print(TEMPTOREACH);
            Serial.println(" °C");
            lcd.clear();
            if (temperature > TEMPTOREACH)
                {   
                    OvenOff;
                    lcd.setCursor(0,0);
                    lcd.println("Temperature");
                    lcd.setCursor(0,1);
                    lcd.println("reached");
                    openOven();
                    delay(2000);
                    status = 4;
                }
            lcd.setCursor(0,0);
            lcd.print("Heating oven to");
            lcd.setCursor(0,1);
            lcd.print(TEMPTOREACH);
            lcd.print("C");
            lcd.print(" T =");
            lcd.print(temperature);
            lcd.print("C");
            OvenOn;
            FanOn;
            break;
        case 4:
            Serial.println("Temperature reached");
            Serial.println("Cooling started");
            Serial.println("Openning oven");
            lcd.clear();
            OvenOff;
            FanOn;
            lcd.setCursor(0,0);
            lcd.println("Cooling to ");
            lcd.print(TEMPCOOL);
            lcd.print("C");
            lcd.setCursor(0,1);
            lcd.print("Actual T = ");
            lcd.print(temperature);
            lcd.print("C");
            if(temperature < TEMPCOOL){
                status = 5;
                }
            break;
        case 5:
            Serial.println("Oven cooled");
            Serial.print(TEMPCOOL," °C");
            Serial.println("Reflow Soldering completed");
            lcd.clear();
            OvenOff;
            FanOff;
            lcd.setCursor(2,0);
            lcd.print("Take your PCB");
            lcd.print(TEMPCOOL," C");
            lcd.setCursor(0,1);
            lcd.print("Green to restart");
            if(digitalRead(START_BUTTON) == LOW) {status = 0;}
            delay(500);
            break;
        case 6:
            Serial.println("Emergency STOP");
            OvenOff;
            FanOff;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.println("Emergency STOP");
            lcd.setCursor(0,1);
            lcd.println("Green to open");
            if(digitalRead(START_BUTTON)==0 && limitswitch == 1)
            {
                openOven(); 
                delay(1000);
                status = 5;
            }
            if(digitalRead(START_BUTTON)==0 && limitswitch == 0)
            {
                closeOven(); 
            }
            delay(100);
            break;
    }
Serial.println(temperature);
Serial.println(limitswitch);
Serial.println(digitalRead(LIMIT_SWITCH));
Serial.println(digitalRead(START_BUTTON));
Serial.println(digitalRead(EMERGENCY_BUTTON));

}

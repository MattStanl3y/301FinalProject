
// these should be all the libraries we need
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
#include <DHT.h>

// need to code this in arduino for it to work, but put it in here so it is accessible in repository

/* pin #'s

Fan Motor:
motorPin1: 3
motorPin2: 4
enablePin: 5
Stepper Motor:
Pin 1: 8
Pin 2: 9
Pin 3: 10
Pin 4: 11
Water Level Sensor:
POWER_PIN: 13
SIGNAL_PIN: A0
DHT sensor
DHT PIN 53:
DHTTYPE DHT: 11
LCD Display
RS: 51
EN: 49
D4: 45
D5: 43
D6: 41
D7: 39
Buttons and LEDs:
Button1 (on/off button): 35
button2(reset button): 34
led1Pin: 25
led2Pin: 27
led3Pin: 29
led4Pin: 31

*/

void setup()
{
    Serial.begin(9600);
}

void loop()
{
    currentState = determineState();
    switch (currentState)
    {
    case IDLE:
        handleIdleState();
        break;
    case RUNNING:
        handleRunningState();
        break;
    case ERROR:
        handleErrorState();
        break;
    case DISABLED:
        handleDisabledState();
        break;
    }
    delay(1000); // Loop delay for debouncing and stability
}

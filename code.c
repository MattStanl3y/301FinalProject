

// need to code this in arduino for it to work, but put it in here so it is accessible in repository

#include "config.h"
#include "states.h"
#include "sensors.h"
#include "display.h"
#include "motor.h"
#include "logging.h"
#include "utils.h"

void setup()
{
    Serial.begin(9600);
    initSensors();
    initDisplay();
    initMotor();
    initLogging();
    pinMode(LED_BUILTIN, OUTPUT);
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

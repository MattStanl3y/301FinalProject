// these should be all the libraries we need


// changed to old code since this is old I think

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
#include <DHT.h>

enum State
{
    IDLE,
    RUNNING,
    ERROR,
    DISABLED
};

// Fan
const int MOTOR_PIN1 = 3;
const int ENABLE_PIN = 5;

// Stepper
const int STEPS_PER_REV = 2038;
Stepper stepper(STEPS_PER_REV, 8, 9, 10, 11);

// Water Level Sensor
const int POWER_PIN = 13;
const int SIGNAL_PIN = A0;

// DHT
const int DHT_PIN = 53;
#define DHT_TYPE DHT11

// LCD Display
const int RS_PIN = 51;
const int EN_PIN = 49;
const int D4_PIN = 45;
const int D5_PIN = 43;
const int D6_PIN = 41;
const int D7_PIN = 39;

// Buttons and LEDs
const int ON_OFF_BUTTON = 2;
const int RESET_BUTTON = 34;
const int YELLOW_LED = 25;
const int GREEN_LED = 27;
const int RED_LED = 29;
const int BLUE_LED = 31;

// volatile buttonStates
volatile bool onOffButtonPressed = false;
volatile bool resetButtonPressed = false;

// Initialize LCD
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

// Initialize DHT Sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Initialize RTC
RTC_DS1307 rtc;

// Variables
State currentState = IDLE;
float temperature;
float humidity;
int waterLevel;
bool fanStatus;
DateTime lastTransitionTime;
unsigned long lastUpdateTime = 0;
bool errorStateEntered = false;

/*-------------------------------------------------------*/

void setup()
{
    // Initialize serial communication
    Serial.begin(9600);

    // Pin modes
    pinMode(MOTOR_PIN1, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(POWER_PIN, OUTPUT);
    pinMode(SIGNAL_PIN, INPUT);
    pinMode(DHT_PIN, INPUT);
    pinMode(ON_OFF_BUTTON, INPUT_PULLUP);
    pinMode(RESET_BUTTON, INPUT_PULLUP);
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);

    // states of LED's
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);

    // Initialize LCD
    lcd.begin(16, 2);
    lcd.print("System Initializing");

    // Initialize DHT Sensor
    dht.begin();

    // Initialize RTC
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    // Configure interrupts for buttons
    attachInterrupt(digitalPinToInterrupt(ON_OFF_BUTTON), onOffButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetButtonISR, FALLING);

    stepper.setSpeed(5);
}

void loop()
{

    if (onOffButtonPressed)
    {
        onOffButtonPressed = false;
        handleOnOffButton();
    }

    if (resetButtonPressed)
    {
        resetButtonPressed = false;
        handleResetButton();
    }

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

/*--------------------------------------------------------------*/

float readTemperature()
{
    float temperature = dht.readTemperature();
    if (isnan(temperature))
    {
        Serial.println("Failed to read temperature from DHT sensor!");
        return -999.0;
    }
    return temperature;
}

float readHumidity()
{
    float humidity = dht.readHumidity();
    if (isnan(humidity))
    {
        Serial.println("Failed to read humidity from DHT sensor!");
        return -999.0;
    }
    return humidity;
}

int readWaterLevel()
{
    digitalWrite(POWER_PIN, HIGH);
    delay(10);
    int waterLevel = analogRead(SIGNAL_PIN);
    digitalWrite(POWER_PIN, LOW);
    return waterLevel;
}

void controlFanMotor(bool state)
{
    if (state)
    {
        digitalWrite(ENABLE_PIN, HIGH);
        digitalWrite(MOTOR_PIN1, HIGH);
    }
    else
    {
        digitalWrite(MOTOR_PIN1, LOW);
        digitalWrite(ENABLE_PIN, LOW);
    }
}

void updateLCD(float temperature, float humidity, int waterLevel)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humidity);
    lcd.print("%% WL: ");
    lcd.print(waterLevel);
}

State determineState()
{
    // Read sensor values
    temperature = readTemperature(); // Ensure these variables are globally declared
    humidity = readHumidity();
    waterLevel = readWaterLevel();

    // Check if the system is manually disabled via a button
    if (currentState == DISABLED) // Use a global variable updated by an ISR
    {
        return DISABLED;
    }

    // Check for any error conditions in sensor readings
    if (temperature == -999.0 || humidity == -999.0 || waterLevel < 100) // Use standardized error handling
    {
        return ERROR;
    }

    // Check if conditions to run the system are met
    if (temperature > 20) // Threshold for activating the system, adjust as necessary
    {
        return RUNNING;
    }

    // If none of the above conditions are met, return IDLE
    return IDLE;
}

void handleErrorState()
{
    // Turn off the fan motor
    controlFanMotor(false);

    // Display error message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error: Low Water");

    // Set LED indicators
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);

    if (!errorStateEntered)
    {
        recordTransitionTime("ERROR");
        errorStateEntered = true;
    }
}

void handleIdleState()
{
    digitalWrite(GREEN_LED, HIGH); // GREEN LED ON to indicate IDLE state
    digitalWrite(RED_LED, LOW);    // Ensure RED LED is OFF
    digitalWrite(BLUE_LED, LOW);   // Ensure BLUE LED is OFF
    digitalWrite(YELLOW_LED, LOW); // Ensure YELLOW LED is OFF

    // Update LCD every minute
    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        waterLevel = readWaterLevel();
        updateLCD(temperature, humidity, waterLevel);
        lastUpdateTime = millis();
    }

    recordTransitionTime("IDLE");

    if (waterLevel < 100)
    { // Check if water level is too low
        currentState = ERROR;
    }
}

void handleRunningState()
{
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);

    // Record transition time to RUNNING state
    recordTransitionTime("RUNNING");

    // Check water level
    if (waterLevel < 100)
    {
        currentState = ERROR;
        return;
    }

    // Check temperature
    if (temperature <= 20)
    {
        currentState = IDLE;
        return;
    }

    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        waterLevel = readWaterLevel();
        updateLCD(temperature, humidity, waterLevel);
        lastUpdateTime = millis();
    }

    // Control fan motor
    controlFanMotor(true);
}

void onOffButtonISR()
{
    onOffButtonPressed = true;
}

void handleOnOffButton()
{
    if (currentState == DISABLED)
    {
        currentState = IDLE;
        digitalWrite(YELLOW_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
        Serial.println("System state: IDLE");
    }
    else
    {
        currentState = DISABLED;
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, LOW);
        digitalWrite(BLUE_LED, LOW);
        digitalWrite(YELLOW_LED, HIGH);
        Serial.println("System state: DISABLED");
    }
}

void resetButtonISR()
{
    resetButtonPressed = true;
}

void handleResetButton()
{
    static bool motorEnabled = false; // This will remember the motor state across function calls
    motorEnabled = !motorEnabled;     // Toggle the state of motorEnabled
    controlFanMotor(motorEnabled);    // Control the motor based on the toggled state

    // Log motor state change
    Serial.print("Motor is now ");
    Serial.println(motorEnabled ? "ON" : "OFF");
}

// Handle DISABLED State
void handleDisabledState()
{
    digitalWrite(YELLOW_LED, HIGH); // YELLOW LED ON to indicate DISABLED state
    digitalWrite(GREEN_LED, LOW);   // Ensure GREEN LED is OFF
    digitalWrite(BLUE_LED, LOW);    // Ensure BLUE LED is OFF
    digitalWrite(RED_LED, LOW);     // Ensure RED LED is OFF

    controlFanMotor(false); // Ensure fan motor is off

    recordTransitionTime("DISABLED");
}

void recordTransitionTime(String state)
{
    DateTime now = rtc.now();
    Serial.println("Transition to " + state + " State at: " + now.timestamp());
}

void recordStepperPosition()
{
    DateTime now = rtc.now();
    Serial.println("Stepper Motor Position Changed at: " + now.timestamp());
}
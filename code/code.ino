// these should be all the libraries we need
// can't use digitalWrite, pinmode, delay, analogRead, 
// can only use millis() for the one minute delay checking temp and humidity

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
const int MOTOR_PIN1 = 4;
const int ENABLE_PIN = 5;

// Stepper
const int STEPS_PER_REV = 1018;
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
const int STEP_MOTOR_BUTTON = 3;
const int YELLOW_LED = 25;
const int GREEN_LED = 27;
const int RED_LED = 29;
const int BLUE_LED = 31;

// volatile buttonStates
volatile bool onOffButtonPressed = false;
volatile bool resetButtonPressed = false;
volatile bool stepMotorButtonPressed = false;

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

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

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
    pinMode(STEP_MOTOR_BUTTON, INPUT_PULLUP);
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
    attachInterrupt(digitalPinToInterrupt(STEP_MOTOR_BUTTON), stepMotorISR, FALLING);
}

void loop()
{
    if (stepMotorButtonPressed)
    {
        stepMotorButtonPressed = false;
        handleStepMotorButton();
    }
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

    delay(100); // Loop delay for debouncing and stability
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

void updateLCD(float temperature, float humidity)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(temperature);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humidity);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------


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
    if (temperature == -999.0 || humidity == -999.0 || waterLevel < 100 || waterLevel > 200) // Use standardized error handling
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

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

void handleErrorState()
{
    // Turn off the fan motor
    controlFanMotor(false);

    // Display message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error: Low Water");
    waterLevel = readWaterLevel();
    lcd.setCursor(0, 1);
    lcd.print(waterLevel);

    // Set LED indicators
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);

    recordTransitionTime("ERROR");

    // Update LCD every minute
    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        waterLevel = readWaterLevel();
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
        delay(3000);
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
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
        delay(3000);
    }

    // Display message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Water Level:");
    waterLevel = readWaterLevel();
    lcd.setCursor(0, 1);
    lcd.print(waterLevel);

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
    if (humidity <= 20)
    {
        currentState = IDLE;
        return;
    }

    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
    }

    // Control fan motor
    controlFanMotor(true);
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

    lcd.clear(); 
    lcd.setCursor(0, 0);
    lcd.print("Disabled");
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------


void handleResetButton()
{
    if (currentState == ERROR)
    {
        currentState = IDLE;
        errorStateEntered = false;
    }
}

void handleStepMotorButton()
{
    stepper.setSpeed(7);
    stepper.step(STEPS_PER_REV);
    recordStepperPosition();
}


void handleOnOffButton()
{
    if (currentState == DISABLED)
    {
        currentState = IDLE;
    }
    else
    {
        currentState = DISABLED;
    }
}

void stepMotorISR()
{
    stepMotorButtonPressed = true;
}

void resetButtonISR()
{
    resetButtonPressed = true;
    // reset to idle state when in error state and water level above threshold
}

void onOffButtonISR()
{
    onOffButtonPressed = true;
}

void recordTransitionTime(String state)
{
    // Define static variables to store last state and transition time
    static String lastState = "";
    static DateTime lastTransitionTime;

    DateTime now = rtc.now();
    
    // Check if the new state is different from the last recorded state
    if (state != lastState) {
        Serial.println(state + " state at: " + now.timestamp());
        lastState = state;
        lastTransitionTime = now;
        delay(100);
    }
}

void recordStepperPosition()
{
    DateTime now = rtc.now();
    Serial.println("Stepper Motor Position Changed at: " + now.timestamp());
}

// these should be all the libraries we need

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
const int STEPPER_PIN1 = 8;
const int STEPPER_PIN2 = 9;
const int STEPPER_PIN3 = 10;
const int STEPPER_PIN4 = 11;

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

// ventPositions
const int VENT_CONTROL_PIN = A1;
const int VENT_CLOSED_POSITION = 0;
const int VENT_OPEN_POSITION = 100;

// Initialize LCD
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

// Initialize Stepper Motor
const int STEPS_PER_REV = 5; // Adjust this later, may be too low
Stepper stepper(STEPS_PER_REV, STEPPER_PIN1, STEPPER_PIN2, STEPPER_PIN3, STEPPER_PIN4);

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

    // Initialize Stepper Motor
    stepper.setSpeed(5); // Adjust the speed later

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

int readVentPosition()
{
    int potValue = analogRead(VENT_CONTROL_PIN);
    int ventPosition = map(analogRead(VENT_CONTROL_PIN), 0, 1023, VENT_CLOSED_POSITION, VENT_OPEN_POSITION);
    return ventPosition;
}

void adjustVentPosition(int position)
{
    stepper.step(position);
}

void controlFanMotor(bool state)
{
    if (state)
    {
        analogWrite(ENABLE_PIN, 255);
    }
    else
    {
        analogWrite(ENABLE_PIN, 0);
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
    if (temperature > 25) // Threshold for activating the system, adjust as necessary
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

    recordTransitionTime("ERROR");

    // Check reset button (ISR)
    // The reset button ISR should handle the transition to IDLE state
    // when the water level is above the threshold
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

    int ventPosition = readVentPosition();
    adjustVentPosition(ventPosition);
    recordStepperPosition(ventPosition);
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
    if (temperature <= 25)
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

    int ventPosition = readVentPosition();
    adjustVentPosition(ventPosition);
    recordStepperPosition(ventPosition);
}

void onOffButtonISR()
{
    onOffButtonPressed = true;
}

void handleResetButton()
{
    if (currentState == ERROR && waterLevel >= 100)
    {
        currentState = IDLE;
        resetButtonPressed = false;
    }
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

void resetButtonISR()
{
    if (waterLevel >= 100)
    {
        currentState = IDLE;
    }
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

    // ISR for start button should handle re-enabling the system
}

void recordTransitionTime(String state)
{
    DateTime now = rtc.now();
    Serial.println("Transition to " + state + " State at: " + now.timestamp());
}

void recordStepperPosition(int position)
{
    DateTime now = rtc.now();
    Serial.println("Stepper Motor Position Changed to " + String(position) + " at: " + now.timestamp());
}

/*

Core Functions
    void setup();
    void loop();


State Management Functions
    State determineState();
    void handleIdleState();
    void handleRunningState();
    void handleErrorState();
    void handleDisabledState();

Sensor Reading Functions
    float readTemperature();
    float readHumidity();
    int readWaterLevel();

Actuator Control Functions
    void controlFanMotor();


User Interface Functions
    void updateLCD();
    void setLEDs();

Error Handling Functions
    void handleError();

Utility Functions
    void recordTransitionTime();
    void sendDataToSerial();

Interrupt Service Routines
    void handleOnOffButton();
    void handleResetButton();
*/

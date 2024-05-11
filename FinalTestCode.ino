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
const int STEP_MOTOR_BUTTON = 3;
const int YELLOW_LED = 25;
const int GREEN_LED = 27;
const int RED_LED = 29;
const int BLUE_LED = 31;

// GPIO Pointers
volatile uint8_t *_portB = (volatile uint8_t *)0x25;
volatile uint8_t *_portDDRB = (volatile uint8_t *)0x24;
volatile uint8_t *_portH = (volatile uint8_t *)0x102;
volatile uint8_t *_portDDRH = (volatile uint8_t *)0x101;
volatile uint8_t *_portA = (volatile uint8_t *)0x22;
volatile uint8_t *_portDDRA = (volatile uint8_t *)0x21;
volatile uint8_t *_portL = (volatile uint8_t *)0x10B;
volatile uint8_t *_portDDRL = (volatile uint8_t *)0x10A;
volatile uint8_t *_portC = (volatile uint8_t *)0x28;    // Add this line
volatile uint8_t *_portDDRC = (volatile uint8_t *)0x27; // Add this line

// ADC Pointers
volatile uint8_t *_ADMUX = (volatile uint8_t *)0x7C;  // Add this line
volatile uint8_t *_ADCSRA = (volatile uint8_t *)0x7A; // Add this line
volatile uint8_t *_ADCL = (volatile uint8_t *)0x78;   // Add this line
volatile uint8_t *_ADCH = (volatile uint8_t *)0x79;   // Add this line

// volatile buttonStates
volatile bool onOffButtonPressed = false;
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
DateTime lastTransitionTime;
unsigned long lastUpdateTime = 0;

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

void setup()
{
    // Initialize serial communication
    Serial.begin(9600);

    // Pin modes
    *_portDDRH |= (1 << 4);                       // Set PH4 (MOTOR_PIN1) as output
    *_portDDRH |= (1 << 5);                       // Set PH5 (ENABLE_PIN) as output
    *_portDDRB |= (1 << 7);                       // Set PB7 (POWER_PIN) as output
    *_portDDRA |= (1 << 0);                       // Set PA0 (DHT_PIN) as input
    *_portDDRA |= (1 << 3) | (1 << 5) | (1 << 7); // Set PA3, PA5, PA7 (YELLOW_LED, GREEN_LED, RED_LED) as outputs
    *_portDDRC |= (1 << 6);                       // Set PC6 (BLUE_LED) as output

    // Set initial states of LEDs
    *_portA &= ~((1 << 3) | (1 << 5) | (1 << 7)); // Set PA3, PA5, PA7 (YELLOW_LED, GREEN_LED, RED_LED) low
    *_portC &= ~(1 << 6);                         // Set PC6 (BLUE_LED) low

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

// -----changed-------

int readWaterLevel()
{
    *_ADMUX = (*_ADMUX & 0xF0) | 0x00;
    *_ADCSRA |= (1 << ADSC);
    while ((*_ADCSRA & (1 << ADSC)) != 0)
        ;
    int waterLevel = *_ADCL | (*_ADCH << 8);

    *_portB |= (1 << 7);

    return waterLevel;
}

// --changed----
void controlFanMotor(bool state)
{
    if (state)
    {
        *_portH |= (1 << 5); // Set PH5 (ENABLE_PIN) high
        *_portH |= (1 << 4); // Set PH4 (MOTOR_PIN1) high
    }
    else
    {
        *_portH &= ~(1 << 4); // Set PH4 (MOTOR_PIN1) low
        *_portH &= ~(1 << 5); // Set PH5 (ENABLE_PIN) low
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
    if (humidity > 33.00) // Threshold for activating the system, adjust as necessary
    {
        return RUNNING;
    }

    // If none of the above conditions are met, return IDLE
    return IDLE;
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----changed all handle.l.States-------

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
    *_portA &= ~(1 << 5); // Set PA5 (GREEN_LED) low
    *_portC &= ~(1 << 6); // Set PC6 (BLUE_LED) low
    *_portA |= (1 << 7);  // Set PA7 (RED_LED) high
    *_portA &= ~(1 << 3); // Set PA3 (YELLOW_LED) low

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
    *_portA |= (1 << 5);  // Set PA5 (GREEN_LED) high to indicate IDLE state
    *_portA &= ~(1 << 7); // Set PA7 (RED_LED) low
    *_portC &= ~(1 << 6); // Set PC6 (BLUE_LED) low
    *_portA &= ~(1 << 3); // Set PA3 (YELLOW_LED) low

    controlFanMotor(false); // Ensure fan motor is off

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
    *_portC |= (1 << 6);  // Set PC6 (BLUE_LED) high
    *_portA &= ~(1 << 7); // Set PA7 (RED_LED) low
    *_portA &= ~(1 << 5); // Set PA5 (GREEN_LED) low
    *_portA &= ~(1 << 3); // Set PA3 (YELLOW_LED) low

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

    // Display message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Running");

    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
        delay(3000);
    }

    // Control fan motor
    controlFanMotor(true);
}

// Handle DISABLED State
void handleDisabledState()
{
    *_portA |= (1 << 3);  // Set PA3 (YELLOW_LED) high to indicate DISABLED state
    *_portA &= ~(1 << 5); // Set PA5 (GREEN_LED) low
    *_portC &= ~(1 << 6); // Set PC6 (BLUE_LED) low
    *_portA &= ~(1 << 7); // Set PA7 (RED_LED) low

    controlFanMotor(false);

    recordTransitionTime("DISABLED");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Disabled");
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

void handleStepMotorButton()
{
    if (currentState != ERROR)
    {
        stepper.setSpeed(7);
        stepper.step(STEPS_PER_REV);
        recordStepperPosition();
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

void stepMotorISR()
{
    stepMotorButtonPressed = true;
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
    if (state != lastState)
    {
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

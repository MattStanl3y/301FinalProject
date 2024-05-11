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

 #define RDA 0x80
 #define TBE 0x20  
 volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
 volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
 volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
 volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
 volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

// Stepper
const int STEPS_PER_REV = 1018;
Stepper stepper(STEPS_PER_REV, 8, 9, 10, 11);

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

// GPIO Pointers
volatile uint8_t *_portB = (volatile uint8_t *)0x25;
volatile uint8_t *_portDDRB = (volatile uint8_t *)0x24;
volatile uint8_t *_portG = (volatile uint8_t *)0x34;
volatile uint8_t *_portDDRG = (volatile uint8_t *)0x33;
volatile uint8_t *_portE = (volatile uint8_t *)0x2E;
volatile uint8_t *_portDDRE = (volatile uint8_t *)0x2D;
volatile uint8_t *_portA = (volatile uint8_t *)0x22;
volatile uint8_t *_portDDRA = (volatile uint8_t *)0x21;
volatile uint8_t *_portC = (volatile uint8_t *)0x28;    
volatile uint8_t *_portDDRC = (volatile uint8_t *)0x27; 

// ADC Pointers
volatile uint8_t *_ADMUX = (volatile uint8_t *)0x7C;  
volatile uint8_t *_ADCSRA = (volatile uint8_t *)0x7A; 
volatile uint8_t *_ADCL = (volatile uint8_t *)0x78;   
volatile uint8_t *_ADCH = (volatile uint8_t *)0x79;   

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
    U0init(9600);

    // Pins
    *_portDDRG |= (1 << 5); 
    *_portDDRE |= (1 << 3); 
    *_portDDRB |= (1 << 7);                       
    *_portDDRA |= (1 << 0);                       
    *_portDDRA |= (1 << 3) | (1 << 5) | (1 << 7); 
    *_portDDRC |= (1 << 6);                       

    // Set initial states of LEDs
    *_portA &= ~((1 << 3) | (1 << 5) | (1 << 7)); 
    *_portC &= ~(1 << 6);                         
    *_portB &= ~(1 << 7);
    *_portG &= ~(1 << 5); 
    *_portE &= ~(1 << 3); 

    *_ADMUX = (1 << REFS0);
    *_ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Initialize LCD
    lcd.begin(16, 2);
    lcd.print("System Initializing");

    // Initialize DHT Sensor
    dht.begin();

    // Initialize RTC
    rtc.begin();

    // button interrupt
    attachInterrupt(digitalPinToInterrupt(ON_OFF_BUTTON), onOffButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(STEP_MOTOR_BUTTON), stepMotorISR, FALLING);
}

void U0init(unsigned long U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
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
    my_delay(1);
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

float readTemperature()
{
    float temperature = dht.readTemperature();
    return temperature;
}

float readHumidity()
{
    float humidity = dht.readHumidity();
    if (isnan(humidity))
    return humidity;
}


int readWaterLevel()
{
    *_portB |= (1 << 7); 

    *_ADMUX = (*_ADMUX & 0xF0) | 0x00;
    *_ADCSRA |= (1 << ADSC);
    while ((*_ADCSRA & (1 << ADSC)) != 0);
    int waterValue = *_ADCL | (*_ADCH << 8);

    *_portB &= ~(1 << 7); 

    return waterValue;
}

void controlFanMotor(bool state)
{
    if (state)
    {
        *_portE |= (1 << 3);
        *_portG |= (1 << 5);
    }
    else
    {
        *_portG &= ~(1 << 5);
        *_portE &= ~(1 << 3);
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
    // Read sensors
    temperature = readTemperature(); 
    humidity = readHumidity();
    waterLevel = readWaterLevel();

    // Check if the system manually disabled by a button
    if (currentState == DISABLED)
    {
        return DISABLED;
    }

    // Check for any error conditions in sensor readings
    if (temperature == -999.0 || humidity == -999.0 || waterLevel < 100 || waterLevel > 200) 
    {
        return ERROR;
    }

    // Check if conditions to run the system are met
    if (temperature > 26.00) // Threshold for activating the system, adjust as necessary
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
    *_portA &= ~(1 << 5); 
    *_portC &= ~(1 << 6);
    *_portA |= (1 << 7);  
    *_portA &= ~(1 << 3); 

    recordTransitionTime("ERROR");

    // Update LCD every minute
    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        waterLevel = readWaterLevel();
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
        my_delay(3);
    }
}

void handleIdleState()
{
    *_portA |= (1 << 5);  
    *_portA &= ~(1 << 7); 
    *_portC &= ~(1 << 6); 
    *_portA &= ~(1 << 3); 

    controlFanMotor(false); 

    // Update LCD every minute
    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        waterLevel = readWaterLevel();
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
        my_delay(3);
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
    *_portC |= (1 << 6); 
    *_portA &= ~(1 << 7); 
    *_portA &= ~(1 << 5); 
    *_portA &= ~(1 << 3); 

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

    // Update LCD every minute
    if (millis() - lastUpdateTime >= 60000)
    {
        temperature = readTemperature();
        humidity = readHumidity();
        updateLCD(temperature, humidity);
        lastUpdateTime = millis();
        my_delay(3);
    }

    controlFanMotor(true);
}

// Handle DISABLED State
void handleDisabledState()
{
    *_portA |= (1 << 3); 
    *_portA &= ~(1 << 5); 
    *_portC &= ~(1 << 6); 
    *_portA &= ~(1 << 7); 
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

void my_delay(unsigned long delaySeconds) {
  DateTime currentTime = rtc.now(); 
  DateTime endTime = currentTime + TimeSpan(delaySeconds);
  while (rtc.now() < endTime) {
  }
}

void recordTransitionTime(const char *state) {
    static const char *lastState = "";
    DateTime now = rtc.now();

    if (strcmp(state, lastState) != 0) {
        char buffer[50];
        sprintf(buffer, "%s state at: %ld\n", state, now.timestamp());
        U0print(buffer);
        lastState = state;
        my_delay(1); 
    }
}

void recordStepperPosition() {
    DateTime now = rtc.now();
    char buffer[50];
    sprintf(buffer, "Stepper Motor Position Changed at: %ld\n", now.timestamp());
    U0print(buffer);
}

void U0print(const char *str) {
    while (*str != '\0') {
        U0putchar(*str);
        str++;
    }
}

void U0putchar(unsigned char U0pdata) {
    while (!(*myUCSR0A & TBE));
    *myUDR0 = U0pdata;
}



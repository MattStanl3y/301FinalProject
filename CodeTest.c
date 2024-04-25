// Code isn't working

// Check states if they are working properly
// check led's they're not working right either
// rtc time is off
// motor and fan both aren't turning on
// buttons aren't working right, tried to fix at the end but messed up more

// also have to figure out how to not use libraries. Can't use pinMode() or digitalWrite()

#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
#include <DHT.h>

// Pin definitions
const int RS_PIN = 51;
const int EN_PIN = 49;
const int D4_PIN = 45;
const int D5_PIN = 43;
const int D6_PIN = 41;
const int D7_PIN = 39;

const int STEPPER_PIN_1 = 8;
const int STEPPER_PIN_2 = 9;
const int STEPPER_PIN_3 = 10;
const int STEPPER_PIN_4 = 11;

const int WATER_LEVEL_POWER_PIN = 13;
const int WATER_LEVEL_SIGNAL_PIN = A0;

const int DHT_PIN = 53;
const int DHTTYPE = DHT11;

const int FAN_MOTOR_PIN_1 = 3;
const int FAN_MOTOR_PIN_2 = 4;
const int FAN_MOTOR_ENABLE_PIN = 5;

const int ON_OFF_BUTTON = 2;
const int RESET_BUTTON = 34;
const int YELLOW_LED = 25;
const int GREEN_LED = 27;
const int RED_LED = 29;
const int BLUE_LED = 31;

// Other constants
const int STEPS_PER_REV = 2038;
const int STEPS_PER_ACTIVATION = 100;
const int WATER_LEVEL_THRESHOLD = 500;
const float TEMPERATURE_THRESHOLD = 25.0;
const bool RTC_CRITICAL = true;

// other
const int DEBOUNCE_DELAY = 50;

int onOffButtonState;
int lastOnOffButtonState = HIGH;
unsigned long onOffDebounceTime = 0;

int resetButtonState;
int lastResetButtonState = HIGH;
unsigned long resetDebounceTime = 0;

// System states
enum State
{
    IDLE,
    RUNNING,
    ERROR,
    DISABLED
};

// Global variables
State currentState = IDLE;
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);
Stepper stepper(STEPS_PER_REV, STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4);
RTC_DS1307 rtc;
DHT dht(DHT_PIN, DHTTYPE);

void setup()
{
    Serial.begin(9600);
    initSensors();
    initDisplay();
    initMotor();
    initLogging();

    // Set start, stop, and reset button pins as inputs with pull-up resistors
    pinMode(ON_OFF_BUTTON, INPUT_PULLUP);
    pinMode(RESET_BUTTON, INPUT_PULLUP);

    // Set LED pins as outputs
    pinMode(YELLOW_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(ON_OFF_BUTTON), startButtonISR, FALLING);
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

    delay(1000); // Loop delay for stability
}
/*----------------------------------------*/

void startButtonISR()
{
    if (currentState == DISABLED)
    {
        currentState = IDLE;
    }
}

int readWaterLevel()
{

    pinMode(WATER_LEVEL_SIGNAL_PIN, INPUT);

    ADCSRA |= (1 << ADEN);

    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    ADMUX |= (1 << REFS0);

    ADMUX = (ADMUX & 0xF0) | (WATER_LEVEL_SIGNAL_PIN & 0x0F);

    ADCSRA |= (1 << ADSC);

    delay(10);

    while (ADCSRA & (1 << ADSC))
        ;

    int waterLevel = ADC;

    return waterLevel;
}

int readVentControl()
{

    pinMode(6, INPUT_PULLUP);

    int ventState = digitalRead(6);
    return ventState;
}

void displayTempHumidity(float temperature, float humidity)
{

    lcd.clear();

    lcd.setCursor(0, 0);

    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");

    lcd.setCursor(0, 1);

    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print("%%");
}

void logEvent(const char *event)
{

    DateTime now = rtc.now();

    // looks weird cause formatted, also date and time is messed up
    char timestamp[20];
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    Serial.print(timestamp);
    Serial.print(" - ");
    Serial.println(event);
}

void activateStepperMotor()
{
    stepper.step(STEPS_PER_ACTIVATION);
}

void activateFanMotor()
{
    pinMode(FAN_MOTOR_PIN_1, OUTPUT);
    pinMode(FAN_MOTOR_PIN_2, OUTPUT);
    pinMode(FAN_MOTOR_ENABLE_PIN, OUTPUT);

    digitalWrite(FAN_MOTOR_PIN_1, HIGH);
    digitalWrite(FAN_MOTOR_PIN_2, LOW);
    digitalWrite(FAN_MOTOR_ENABLE_PIN, HIGH);
}

bool readStartButton()
{
    return debounceButton(ON_OFF_BUTTON, onOffButtonState, lastOnOffButtonState, onOffDebounceTime);
}

bool readResetButton()
{
    return debounceButton(RESET_BUTTON, resetButtonState, lastResetButtonState, resetDebounceTime);
}

bool debounceButton(int buttonPin, int &buttonState, int &lastButtonState, unsigned long &debounceTime)
{
    bool buttonPressed = false;
    int reading = digitalRead(buttonPin);

    if (reading != lastButtonState)
    {
        debounceTime = millis();
    }

    if ((millis() - debounceTime) > DEBOUNCE_DELAY)
    {
        if (reading != buttonState)
        {
            buttonState = reading;
            if (buttonState == LOW)
            {
                buttonPressed = true;
            }
        }
    }

    lastButtonState = reading;
    return buttonPressed;
}

void deactivateFanMotor()
{

    digitalWrite(FAN_MOTOR_PIN_1, LOW);
    digitalWrite(FAN_MOTOR_PIN_2, LOW);
    digitalWrite(FAN_MOTOR_ENABLE_PIN, LOW);
}

void initSensors()
{
    pinMode(WATER_LEVEL_POWER_PIN, OUTPUT);
    digitalWrite(WATER_LEVEL_POWER_PIN, LOW);

    dht.begin();
}

void initDisplay()
{
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Swamp Cooler");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
}

void initMotor()
{
    stepper.setSpeed(60);
}

void initLogging()
{
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");

        if (RTC_CRITICAL)
        {
            Serial.println("RTC is critical for system operation. Halting.");
            while (1)
                ;
        }
        else
        {
            Serial.println("RTC is not critical. Continuing without RTC functionality.");
        }
    }

    if (!rtc.isrunning())
    {
        Serial.println("RTC is NOT running!");
        if (RTC_CRITICAL)
        {
            Serial.println("RTC is critical for system operation. Halting.");
            while (1)
                ;
        }
        else
        {
            Serial.println("RTC is not critical. Continuing without RTC functionality.");

            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    }
}

State determineState()
{

    int waterLevel = readWaterLevel();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Read button states
    bool startButton = readStartButton();
    bool resetButton = readResetButton();

    if (currentState == DISABLED)
    {
        if (startButton)
        {
            return IDLE;
        }
    }
    else if (currentState == IDLE)
    {
        if (resetButton)
        {
            return DISABLED;
        }
        else if (waterLevel < WATER_LEVEL_THRESHOLD)
        {
            return ERROR;
        }
        else if (temperature > TEMPERATURE_THRESHOLD)
        {
            return RUNNING;
        }
    }
    else if (currentState == RUNNING)
    {
        if (resetButton)
        {
            return DISABLED;
        }
        else if (waterLevel < WATER_LEVEL_THRESHOLD)
        {
            return ERROR;
        }
        else if (temperature <= TEMPERATURE_THRESHOLD)
        {
            return IDLE;
        }
    }
    else if (currentState == ERROR)
    {
        if (resetButton && waterLevel >= WATER_LEVEL_THRESHOLD)
        {
            return IDLE;
        }
    }

    return currentState;
}

void handleIdleState()
{
    pinMode(YELLOW_LED, OUTPUT);

    digitalWrite(YELLOW_LED, HIGH);

    int waterLevel = readWaterLevel();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    displayTempHumidity(temperature, humidity);

    activateStepperMotor();

    if (waterLevel < WATER_LEVEL_THRESHOLD)
    {
        currentState = ERROR;
        logEvent("Water level too low. Transitioning to ERROR state.");
    }
}

void handleRunningState()
{
    pinMode(GREEN_LED, OUTPUT);

    digitalWrite(GREEN_LED, HIGH);

    activateFanMotor();

    int waterLevel = readWaterLevel();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    displayTempHumidity(temperature, humidity);

    if (waterLevel < WATER_LEVEL_THRESHOLD)
    {
        currentState = ERROR;
        logEvent("Water level too low. Transitioning to ERROR state.");
    }
    else if (temperature <= TEMPERATURE_THRESHOLD)
    {
        currentState = IDLE;
        logEvent("Temperature dropped below threshold. Transitioning to IDLE state.");
    }
}

void handleErrorState()
{
    pinMode(RED_LED, OUTPUT);

    digitalWrite(RED_LED, HIGH);

    deactivateFanMotor();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error: Low Water");
    lcd.setCursor(0, 1);
    lcd.print("Reset to resume");

    if (readResetButton() && readWaterLevel() >= WATER_LEVEL_THRESHOLD)
    {
        currentState = IDLE;
        logEvent("Reset button pressed. Water level above threshold. Transitioning to IDLE state.");
    }
}

void handleDisabledState()
{
    pinMode(BLUE_LED, OUTPUT);

    digitalWrite(BLUE_LED, HIGH);

    if (readStartButton())
    {
        currentState = IDLE;
        logEvent("Start button pressed. Transitioning to IDLE state.");
    }
}

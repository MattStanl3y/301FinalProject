#define POWER_PIN 13
#define SIGNAL_PIN A0

// GPIO Pointers
volatile uint8_t *_portB = (volatile uint8_t *)0x25;
volatile uint8_t *_portDDRB = (volatile uint8_t *)0x24;

int value = 0;

void setup()
{
    Serial.begin(9600);
    *_portDDRB |= (1 << 7); // Set PB7 (digital pin 13) as output
    *_portB &= ~(1 << 7);   // Set PB7 (digital pin 13) low
}

void loop()
{
    *_portB |= (1 << 7); // Set PB7 (digital pin 13) high
    delay(10);
    value = analogRead(SIGNAL_PIN);
    *_portB &= ~(1 << 7); // Set PB7 (digital pin 13) low
    Serial.print("Sensor value: ");
    Serial.println(value);
    delay(1000);
}
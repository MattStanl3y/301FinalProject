#define POWER_PIN 13
#define SIGNAL_PIN A0

// GPIO Pointers
volatile uint8_t *_portB = (volatile uint8_t *)0x25;
volatile uint8_t *_portDDRB = (volatile uint8_t *)0x24;

// ADC Pointers
volatile uint8_t *_ADMUX = (volatile uint8_t *)0x7C;
volatile uint8_t *_ADCSRA = (volatile uint8_t *)0x7A;
volatile uint8_t *_ADCL = (volatile uint8_t *)0x78;
volatile uint8_t *_ADCH = (volatile uint8_t *)0x79;

int value = 0;

void setup()
{
    Serial.begin(9600);
    *_portDDRB |= (1 << 7); // Set PB7 (digital pin 13) as output
    *_portB &= ~(1 << 7);   // Set PB7 (digital pin 13) low

    // Configure ADC
    *_ADMUX = (1 << 6);                                                         // Set reference voltage to AVCC
    *_ADCSRA = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 2) | (1 << 1) | (1 << 0); // Enable ADC, set prescaler to 128
}

void loop()
{
    *_portB |= (1 << 7); // Set PB7 (digital pin 13) high
    delay(10);

    // Start ADC conversion
    *_ADCSRA |= (1 << 6);

    // Wait for conversion to complete
    while ((*_ADCSRA & (1 << 6)) != 0)
        ;

    // Read ADC value
    value = *_ADCL | (*_ADCH << 8);

    *_portB &= ~(1 << 7); // Set PB7 (digital pin 13) low
    Serial.print("Sensor value: ");
    Serial.println(value);
    delay(1000);
}
// GPIO Pointers
volatile uint8_t *_portC = (volatile uint8_t *)0x28;
volatile uint8_t *_portDDRC = (volatile uint8_t *)0x27;
volatile uint8_t *_portA = (volatile uint8_t *)0x22;
volatile uint8_t *_portDDRA = (volatile uint8_t *)0x21;

const int button1Pin = 35;
const int button2Pin = 34;
const int led1Pin = 25;
const int led2Pin = 27;
const int led3Pin = 29;
const int led4Pin = 31;

int button1State = 0;
int button2State = 0;

void setup()
{
    *_portDDRA |= (1 << 3) | (1 << 5) | (1 << 7); // Set PA3, PA5, PA7 (digital pins 25, 27, 29) as outputs
    *_portDDRC |= (1 << 6);                       // Set PC6 (digital pin 31) as output
    *_portC &= ~((1 << 3) | (1 << 2));            // Set PC3, PC2 (digital pins 35, 34) as inputs
}

void loop()
{
    button1State = (*_portC & (1 << 3)) >> 3; // Read PC3 (digital pin 35)
    button2State = (*_portC & (1 << 2)) >> 2; // Read PC2 (digital pin 34)

    if (button1State == HIGH)
    {
        *_portA |= (1 << 3) | (1 << 5); // Set PA3, PA5 (digital pins 25, 27) high
    }
    else
    {
        *_portA &= ~((1 << 3) | (1 << 5)); // Set PA3, PA5 (digital pins 25, 27) low
    }

    if (button2State == HIGH)
    {
        *_portA |= (1 << 7); // Set PA7 (digital pin 29) high
        *_portC |= (1 << 6); // Set PC6 (digital pin 31) high
    }
    else
    {
        *_portA &= ~(1 << 7); // Set PA7 (digital pin 29) low
        *_portC &= ~(1 << 6); // Set PC6 (digital pin 31) low
    }
}
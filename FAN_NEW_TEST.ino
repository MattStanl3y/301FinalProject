// GPIO Pointers
volatile uint8_t *_portG = (volatile uint8_t *)0x34;
volatile uint8_t *_portDDRG = (volatile uint8_t *)0x33;
volatile uint8_t *_portE = (volatile uint8_t *)0x2E;
volatile uint8_t *_portDDRE = (volatile uint8_t *)0x2D;

const int motorPin1 = 4; // Corresponds to PG5
const int enablePin = 5; // Corresponds to PE3

void setup()
{
    *_portDDRG |= (1 << 5); // Set PG5 as output
    *_portDDRE |= (1 << 3); // Set PE3 as output

    *_portG &= ~(1 << 5); // Set PG5 low,
    *_portE &= ~(1 << 3); // Set PE3 low
}

void loop()
{
    *_portE |= (1 << 3); // Set PE3 high,
    *_portG |= (1 << 5); // Set PG5 high
    delay(5000);

    *_portG &= ~(1 << 5); // Set PG5 low
    *_portE &= ~(1 << 3); // Set PE3 low,
    delay(5000);
}


// GPIO Pointers
volatile uint8_t *_portE = (volatile uint8_t *)0x2E;
volatile uint8_t *_portDDRE = (volatile uint8_t *)0x2D;

const int motorPin1 = 3; // Corresponds to PE3
const int enablePin = 5; // Corresponds to PE5

void setup()
{
    *_portDDRE |= (1 << enablePin); // Set PE5 as output, labeled pin 5
    *_portDDRE |= (1 << motorPin1); // Set PE3 as output, labeled pin 3

    // start pins low
    *_portE &= ~(1 << enablePin); // Set PE5 low
    *_portE &= ~(1 << motorPin1); // Set PE3 low
}

void loop()
{
    *_portE |= (1 << motorPin1); // Set PE3 high
    *_portE |= (1 << enablePin); // Set PE5 high
    delay(5000);

    // Turn off fan
    *_portE &= ~(1 << enablePin); // Set PE5 low
    *_portE &= ~(1 << motorPin1); // Set PE3 low
    delay(5000);
}

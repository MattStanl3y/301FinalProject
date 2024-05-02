

// GPIO Pointers
volatile unsigned char *portE = (unsigned char *)0x2E;
volatile unsigned char *portDDRE = (unsigned char *)0x2D;

const int motorPin1 = 3;
const int enablePin = 5;

void setup()
{
    *portDDRE |= (1 << 5); // PE5 (pin 3) as output
    *portDDRE |= (1 << 3); // PE3 (pin 5) as output

    // Ensure both pins are initially low
    *portE &= ~(1 << 5); // PE5 (pin 3) low
    *portE &= ~(1 << 3); // PE3 (pin 5) low
}

void loop()
{
    *portE |= (1 << 3); // PE3 (pin 5) high
    *portE |= (1 << 5); // PE5 (pin 3) high

    delay(5000);

    // Turn off  fan
    *portE &= ~(1 << 5); // PE5 (pin 3) low
    *portE &= ~(1 << 3); // PE3 (pin 5) low

    delay(5000);
}
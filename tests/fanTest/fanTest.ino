
const int motorPin1 = 3;
const int enablePin = 5;

void setup()
{

  Serial.begin(9600);

  // Direct access to registers didn't work , change back to pinMode to get to work
  DDRD |= (1 << DDD3);
  DDRD |= (1 << DDD5);

  PORTD &= ~(1 << PORTD3);
  PORTD &= ~(1 << PORTD5);

  Serial.println("Fan motor test setup complete.");
}

void loop()
{

  PORTD |= (1 << PORTD5);

  PORTD |= (1 << PORTD3);
  Serial.println("Fan motor is ON");

  delay(5000);

  PORTD &= ~(1 << PORTD3);
  PORTD &= ~(1 << PORTD5);
  Serial.println("Fan motor is OFF");

  delay(5000);
}

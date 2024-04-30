const int motorPin1 = 3;
const int enablePin = 5;

void setup()
{
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // Set motorPin1 and enablePin as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(enablePin, OUTPUT);

  // Ensure both pins are initially low
  digitalWrite(motorPin1, LOW);
  digitalWrite(enablePin, LOW);

  Serial.println("Fan motor test setup complete.");
}

void loop()
{
  // Turn on the fan motor
  digitalWrite(enablePin, HIGH);
  digitalWrite(motorPin1, HIGH);
  Serial.println("Fan motor is ON");

  // Keep the fan motor on for 5 seconds
  delay(5000);

  // Turn off the fan motor
  digitalWrite(motorPin1, LOW);
  digitalWrite(enablePin, LOW);
  Serial.println("Fan motor is OFF");

  // Keep the fan motor off for 5 seconds
  delay(5000);
}

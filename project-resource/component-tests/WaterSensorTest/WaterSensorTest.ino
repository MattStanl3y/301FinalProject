#define POWER_PIN 13
#define SIGNAL_PIN A0

int value = 0;

void setup()
{
    Serial.begin(9600);
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW);
}

void loop()
{
    digitalWrite(POWER_PIN, HIGH);
    delay(10);
    value = analogRead(SIGNAL_PIN);
    digitalWrite(POWER_PIN, LOW);

    Serial.print("Sensor value: ");
    Serial.println(value);

    delay(1000);
}

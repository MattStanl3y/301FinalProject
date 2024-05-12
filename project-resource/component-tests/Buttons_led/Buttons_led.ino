
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

    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);
    pinMode(led3Pin, OUTPUT);
    pinMode(led4Pin, OUTPUT);

    pinMode(button1Pin, INPUT);
    pinMode(button2Pin, INPUT);
}

void loop()
{

    button1State = digitalRead(button1Pin);
    button2State = digitalRead(button2Pin);

    if (button1State == HIGH)
    {

        digitalWrite(led1Pin, HIGH);
        digitalWrite(led2Pin, HIGH);
    }
    else
    {

        digitalWrite(led1Pin, LOW);
        digitalWrite(led2Pin, LOW);
    }

    if (button2State == HIGH)
    {

        digitalWrite(led3Pin, HIGH);
        digitalWrite(led4Pin, HIGH);
    }
    else
    {

        digitalWrite(led3Pin, LOW);
        digitalWrite(led4Pin, LOW);
    }
}

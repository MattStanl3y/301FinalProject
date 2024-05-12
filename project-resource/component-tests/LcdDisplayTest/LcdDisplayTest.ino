#include <LiquidCrystal.h>

const int rs = 51, en = 49, d4 = 45, d5 = 43, d6 = 41, d7 = 39;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup()
{
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Hello");
    lcd.setCursor(2, 1);
    lcd.print("World");
}
void loop() {}

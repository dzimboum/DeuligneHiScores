#include <Wire.h>
#include <Deuligne.h>
#include <EEPROM.h>
#include <DeuligneHiScores.h>

Deuligne lcd;
DeuligneHiScores hsc(lcd);

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  lcd.init();
  hsc.begin(10, 0, 0x1001);
  hsc.insert(1234);
  hsc.insert(2345);
  hsc.insert(123);
  hsc.insert(123);
  hsc.insert(123);
  hsc.insert(123);
}

void loop()
{
  hsc.display();
  delay(1000);
}

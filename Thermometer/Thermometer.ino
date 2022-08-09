/*
  LiquidCrystal Library - display() and noDisplay()

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD and uses the
 display() and noDisplay() functions to turn on and off
 the display.

 The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystalDisplay

*/

// include the library code:
#include <LiquidCrystal.h>
#include <Arduino.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11;
const int d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const int PIN_V2 = A1;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  pinMode(PIN_V2, INPUT);
}

void print_scalar(const float t, const char unit) {
  int whole = (int)t;
  int fraction = (int)((t-whole)*100);
  lcd.print(whole, DEC);
  lcd.print('.');
  lcd.print(fraction, DEC);
  lcd.print(unit);
}

void print_temp(const float t) {
  print_scalar(t, 'C');
}

float read_R2() {
  const float R1 = 10e3;
  const float V = 1023.0;
  const auto V2 = analogRead(PIN_V2);
  const float R2 = R1 * (V2 / (V-V2) );
  return R2;
}

float thermistor_r_to_t(float r) {
// The beta coefficient of the thermistor (usually 3000-4000)
// https://learn.adafruit.com/thermistor/using-a-thermistor
// values from https://cdn-shop.adafruit.com/datasheets/103_3950_lookuptable.pdf
  constexpr auto BCOEFFICIENT = 3950;
  constexpr auto THERMISTORNOMINAL = 10e3;
  constexpr auto TEMPERATURENOMINAL = 25.0;

  float steinhart;
  steinhart = r / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert absolute temp to C
    
  return steinhart;
}

float read_temp() {
  const auto r2 = read_R2();
  const auto t = thermistor_r_to_t(r2);
  return t;
}


void read_temp_task() {
  auto temp = read_temp();
  print_scalar(temp, 'C');
}

void loop() {
  lcd.noDisplay();
  lcd.clear();
  read_temp_task();
  lcd.display();
  delay(500);
}

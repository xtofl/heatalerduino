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

struct Temperature {
  float celcius;
};

template<size_t Size>
class History {
  
  Temperature buffer[Size] = {1};

public:
  size_t current = 0;
  
  void append(Temperature t) {
    const auto at_end = current == Size - 1;
    auto next = at_end
      ? 0
      : current + 1;
    buffer[next] = t;
    current = next;
  }

  size_t wrap(int index) const {
    if(index < 0) {
      return index + Size;
    }
    if(index >= static_cast<int>(Size)) {
      return index - Size;
    }
    return index;
  }

  template<typename F>
  void for_each(F f, int stride = -5) const {
    int end = static_cast<int>(Size);
    if (stride < 0) end = -end;
    for (int i = 0; i != end; i += stride) {      
      f(buffer[wrap(current + i)], i);
    }
  }
};

class UI {
  // initialize the library by associating any needed LCD interface pin
  // with the arduino pin number it is connected to
  const int rs = 12, en = 11;
  const int d4 = 2, d5 = 3, d6 = 4, d7 = 5;

  LiquidCrystal lcd;

public:
  UI(): lcd(rs, en, d4, d5, d6, d7)
  {
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
  }

  void stream_scalar(const float t, const char unit) {
    int whole = (int)t;
    int fraction = (int)((t-whole)*100);
    lcd.print(whole, DEC);
    lcd.print('.');
    lcd.print(fraction, DEC);
    lcd.print(unit);
  }

  void stream(const Temperature t) {
    stream_scalar(t.celcius, 'C');
  }

  void stream_filled_negative(int i) {
    if (i > -10) {
      lcd.print("-0");
      lcd.print(-i);
    } else if (i > -100 ){
      lcd.print(i);
    } else {
      lcd.print(i);
    }
  }
  void stream(const int i, const int N) {
    stream_filled_negative(i);
    lcd.print("/");
    lcd.print(N);
  }

  template<size_t N>
  void update_temperatures(const History<N> history) {
    history.for_each([&](Temperature t, int i){
      lcd.noDisplay();
      lcd.clear();
      
      stream(i, N);
      lcd.print(": ");
      stream(t);
      lcd.display();
      delay(500);
    });
  }
} ui;

class Thermistor {
  static constexpr int PIN_V2 = A1;

  struct Resistance {
    float ohm;
  };
  Resistance read_R2() {
    const float R1 = 10e3;
    const float V = 1023.0;
    const auto V2 = analogRead(PIN_V2);
    const float R2 = R1 * (V2 / (V-V2) );
    return {R2};
  }

  static Temperature thermistor_r_to_t(Resistance r) {
  // The beta coefficient of the thermistor (usually 3000-4000)
  // https://learn.adafruit.com/thermistor/using-a-thermistor
  // values from https://cdn-shop.adafruit.com/datasheets/103_3950_lookuptable.pdf
    constexpr auto BCOEFFICIENT = 3950;
    constexpr Resistance THERMISTORNOMINAL = {10e3};
    constexpr Temperature TEMPERATURENOMINAL = {25.0};
  
    float steinhart;
    steinhart = r.ohm / THERMISTORNOMINAL.ohm;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL.celcius + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    steinhart -= 273.15;                         // convert absolute temp to C
  
    return {steinhart};
  }

public:
  Thermistor(){
    pinMode(PIN_V2, INPUT);
  }
  Temperature read_temp() {
    const auto r2 = read_R2();
    const auto t = thermistor_r_to_t(r2);
    return t;
  }
} thermistor;

History<50> history;

void setup() {
  auto current_temp = thermistor.read_temp();
  for(int i = 0; i != 50; ++i) history.append(current_temp);
}



void read_temp_task() {
  history.append(thermistor.read_temp());
  ui.update_temperatures(history);
}

void loop() {
  read_temp_task();
  delay(2000);
}
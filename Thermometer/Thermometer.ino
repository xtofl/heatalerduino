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

struct Time {
  int64_t ticks;
};
constexpr Time one_second  = {1000};
constexpr Time operator-(const Time lhs, const Time other) { return {lhs.ticks - other.ticks}; }
constexpr float operator/(const Time lhs, const Time other) { return (float)lhs.ticks/other.ticks; }
constexpr Time operator*(const Time lhs, const float f) { return Time{static_cast<float>(lhs.ticks*f)}; }
constexpr float minutes(const Time lhs) {
  return lhs/one_second/60;
}

template<class X, class Y>
struct Sample {
  X x;
  Y y;
};

struct TemperatureSample : public Sample<Time, Temperature> {
  Time time() const { return x; }
  Temperature temperature() const { return y; }
};

template<size_t N, class T>
class History {

  T buffer[N];

public:
  constexpr static size_t Size = N;
  size_t count = 0;
  size_t current = 0;

  void append(T t) {
    const auto at_end = current == Size - 1;
    auto next = at_end
      ? 0
      : current + 1;
    buffer[next] = t;
    current = next;
    if (count < Size) ++count;
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
    int end = static_cast<int>(count);
    if (stride < 0) end = -end;
    for (int i = 0; i != end; i += stride) {
      f(buffer[wrap(current + i)], i);
    }
  }
  T most_recent() const {
    return buffer[current];
  }
  template<typename F, typename R>
  R reduce(F f, R acc) const {
    for_each([&](T s, int){
      acc = f(acc, s);
    }, 1);
    return acc;
  }
  T oldest() const {
    return buffer[wrap(current-count+1)];
  }
};

class UI {
  // initialize the library by associating any needed LCD interface pin
  // with the arduino pin number it is connected to
  const int rs = 12, en = 11;
  const int d4 = 2, d5 = 3, d6 = 4, d7 = 5;

  LiquidCrystal lcd;
  struct Row {
    int value;
    operator int () const { return value; }
  };

public:
  UI(): lcd(rs, en, d4, d5, d6, d7)
  {
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
  }

  template<typename T>
  void stream(T t) {
    lcd.print(t);
  }

  void stream_scalar(const float t, const char unit) {
    lcd.print(t, 2);
    lcd.print(unit);
  }

  void stream(Time t) {
    stream<long>(t.ticks / 1000);
    stream(":");
  }

  void stream(const Temperature t) {
    stream_scalar(t.celcius, 'C');
  }

  void setCursor(Row r){
    lcd.setCursor(0, r.value);
  }

  template<class Hist>
  Time update_temperatures(const Hist history) {
    Time time{0};

    auto avg = Temperature{
      history.reduce([](float acc, TemperatureSample s){
        return acc + s.y.celcius;
      }, 0.0) / Hist::Size
    };
    lcd.clear();
    setCursor(Row{0});
    const auto current = history.most_recent().temperature();
    stream(current);
      if (current.celcius > avg.celcius + 0.2) {
        stream(">");
      }
      else if (current.celcius < avg.celcius - 0.2) {
        stream("<");
      } else {
        stream("=");
      }
    stream("|");
    stream(avg);
    stream("|");
    setCursor(Row{1});

    auto t0 = history.oldest().x;
    auto t1 = history.most_recent().x;
    stream(static_cast<float>(minutes(t1-t0))) ;
    stream('\'');

    // stream(" ~");
    // stream(range);
    return time;
  }
};

class Thermistor {
  const int pin_v2;

public:
  Thermistor(int pin_v2): pin_v2{pin_v2} {
    pinMode(pin_v2, INPUT);
  }
  Temperature read_temp() {
    const auto r2 = read_R2();
    const auto t = thermistor_r_to_t(r2);
    return t;
  }

private:
  struct Resistance {
    float ohm;
  };
  Resistance read_R2() {
    const float R1 = 10e3;
    const float V = 1023.0;
    const auto V2 = analogRead(pin_v2);
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
};


class Thermometer {
public:
  Thermistor thermistor {A1};
  History<50, TemperatureSample> history;
  Time elapsed_time{0};
  UI ui;

  void update_temperature() {
    Temperature current_temp = thermistor.read_temp();

    TemperatureSample s;
    s.x = elapsed_time;
    s.y = current_temp;
    history.append(s);

    elapsed_time.ticks += ui.update_temperatures(history).ticks;
  }
  void add_delay(Time d) {
    elapsed_time.ticks += d.ticks;
  }
};

Thermometer thermometer;

void setup() {
  //auto current_temp = thermistor.read_temp();
  //for(int i = 0; i != 50; ++i) history.append(TemperatureSample{{current_temp}, {0}});
}

void loop() {
  thermometer.update_temperature();
  constexpr auto step = one_second * 10;
  delay(step.ticks);
  thermometer.add_delay(step);
}

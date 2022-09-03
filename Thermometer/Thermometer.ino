#include <LiquidCrystal.h>
#include <Arduino.h>


struct Temperature {
  float celcius;
};

struct Time {
  int64_t ticks;
};

bool operator<(const Temperature &lhs, const Temperature &rhs) { return lhs.celcius < rhs.celcius; }
bool operator>(const Temperature &lhs, const Temperature &rhs) { return lhs.celcius > rhs.celcius; }
bool operator<(const Time &lhs, const Time &rhs) { return lhs.ticks < rhs.ticks; }
bool operator>(const Time &lhs, const Time &rhs) { return lhs.ticks > rhs.ticks; }


constexpr Time one_second  = {1000};
constexpr Time one_millisecond  = {1};
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

template<typename T>
struct BoundingBox {
  T top_left;
  T bottom_right;
  void stretch_to(T t) {
    if (t.x < top_left.x) top_left.x = t.x;
    if (t.x > bottom_right.x) bottom_right.x = t.x;
    if (t.y > top_left.y) top_left.y = t.y;
    if (t.y < bottom_right.y) bottom_right.y = t.y;
  }
};

template<size_t N, class T>
class History {

  T buffer[N];
  BoundingBox<T> bounding_box_;

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
    if (count == 1) {
      bounding_box_ = {t, t};
    } else {
      bounding_box_.stretch_to(t);
    }
  }
  const BoundingBox<T> bounding_box() const {
    return bounding_box_;
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

    stream(avg);
    stream("/");
    auto t0 = history.oldest().x;
    auto t1 = history.most_recent().x;
    stream(static_cast<int>(minutes(t1-t0))) ;
    stream("\'");

    setCursor(Row{1});
    stream("[");
    lcd.print(history.bounding_box().bottom_right.temperature().celcius, 1);
    stream(";");
    lcd.print(history.bounding_box().top_left.temperature().celcius, 1);
    stream("]");

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

constexpr auto step = one_second * 10;

struct Frequency {
  uint64_t per_second;
};

template<typename T> constexpr uint64_t MaxTimerTicks(){ return (1ull << (sizeof(T) * 8));}
static_assert(MaxTimerTicks<uint8_t>() == 256);
static_assert(MaxTimerTicks<uint16_t>() == 65536u);

struct PreScaler {
  int divider;
  uint8_t code;
};
namespace atmega326 {
  constexpr const Frequency FREQ{16000000};
  constexpr const PreScaler prescalers[]  = {
    {1024, 0b101},
    {256, 0b100},
    {64, 0b011},
    {8, 0b010},
    {1, 0b001}
  };
}

template<typename CounterType, size_t N>
constexpr auto prescaler(const Time interval, const Frequency cpu, const PreScaler (&prescalers)[N]) {
  for(auto prescaler: prescalers) {
    const auto scaled_frequency = Frequency{cpu.per_second / prescaler};
    const auto time_to_fill_counter = MaxTimerTicks<CounterType>() / scaled_frequency.per_second;
    if (interval/one_second < time_to_fill_counter) {
      return prescaler;
    }
  }
  return 0;
}


static_assert(is_equal<int, 10, 10>);

// cf https://www.robotshop.com/community/forum/t/arduino-101-timers-and-interrupts/13072
static_assert(is_equal<int, prescaler<uint16_t>(Time{500}, atmega326::FREQ, atmega326::prescalers).divider, 256>);

auto every(Time step, auto callback){

}

void setup() {
  //auto current_temp = thermistor.read_temp();
  //for(int i = 0; i != 50; ++i) history.append(TemperatureSample{{current_temp}, {0}});
  every(step, [&](void*){
    thermometer.update_temperature();
    thermometer.add_delay(step);
  });

}

void loop() {
  delay(step.ticks);
}

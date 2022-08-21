ARDUINO_VERSION ?= 1.8.19
ARDUINO_PATH ?= $(HOME)/opt/arduino/arduino-$(ARDUINO_VERSION)

compile: Thermometer/Thermometer.ino build build-cache
	$(ARDUINO_PATH)/arduino-builder\
	 \
	  -compile \
	  -logger=machine \
	  -hardware $(ARDUINO_PATH)/hardware \
	  -hardware $(HOME)/.arduino15/packages \
	  -tools $(ARDUINO_PATH)/tools-builder \
	  -tools $(ARDUINO_PATH)/hardware/tools/avr \
	  -tools $(HOME)/.arduino15/packages \
	  -built-in-libraries $(ARDUINO_PATH)/libraries \
	  -libraries $(HOME)/Arduino/libraries \
	  -fqbn=arduino:avr:nano:cpu=atmega328 \
	  -ide-version=10819 \
	  -build-path build \
	  -warnings=all \
	  -build-cache build-cache \
	  -prefs=build.warn_data_percentage=75 \
	  -prefs=runtime.tools.arduinoOTA.path=$(HOME)/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 \
	  -prefs=runtime.tools.arduinoOTA-1.3.0.path=$(HOME)/.arduino15/packages/arduino/tools/arduinoOTA/1.3.0 \
	  -prefs=runtime.tools.avrdude.path=$(HOME)/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17 \
	  -prefs=runtime.tools.avrdude-6.3.0-arduino17.path=$(HOME)/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17 \
	  -prefs=runtime.tools.avr-gcc.path=$(HOME)/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 \
	  -prefs=runtime.tools.avr-gcc-7.3.0-atmel3.6.1-arduino7.path=$(HOME)/.arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7 \
	  -verbose \
	  $<

build build-cache:
	mkdir $@

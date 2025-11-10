#pragma once

#define F_CPU 8000000

// note: if you get a compilation error about constexpr, add --std=c++17 to the compiler arguments

constexpr double   signal_period_sec = 8.0 / F_CPU;
constexpr double   measure_duration_sec = 1.0 / 50.0; // 1/50 for 50Hz countries, 1/60 for 60Hz countries
constexpr uint16_t measure_signal_cycles = static_cast<uint16_t>(measure_duration_sec / signal_period_sec);
constexpr uint8_t  measure_phase = 7;

constexpr double   auto_power_off_sec = 300;
static_assert(auto_power_off_sec / measure_duration_sec <= 65535, "auto_power_off_sec too long.");
constexpr uint16_t power_off_measure_count = static_cast<uint16_t>(auto_power_off_sec / measure_duration_sec);

/*
PIN  1  VCC
PIN  2  PB0 INIT=OUT0
PIN  3  PB1 INIT=OUT0
PIN  4  PB3 INIT=IN   PORG-RESET
PIN  5  PB2 INIT=OUT1 (SERIAL)
PIN  6  PA7 INIT=OUT0           OC0B/GEN-OUT
PIN  7  PA6 INIT=IN   PROG-MOSI (Power-Button)
PIN  8  PA5 INIT=OUT0 PROG-MISO OC1B/BEEP-OUT
PIN  9  PA4 INIT=IN   PROG-SCK  (Power-Button)
PIN 10  PA3 INIT=OUT0           (Power-On-LED)
PIN 11  PA2 INIT=OUT0
PIN 12  PA1 INIT=OUT            SENSE_B
PIN 13  PA0 INIT=OUT            SENSE_A
PIN 14  GND
*/

#define INIT_PORTA             0b0101'0000
#define INIT_DDRA              0b1010'1111
#define INIT_PORTB             0b0000'1100
#define INIT_DDRB              0b0000'0111

// PIN must be OC0B = PA7
#define PIN_GENOUT_PORT        PORTA
#define PIN_GENOUT_DDR         DDRA
#define PIN_GENOUT_BIT         7
#define PIN_GENOUT_MASK        (1<<PIN_GENOUT_BIT)

// PIN must be OC1B = PA5
#define PIN_SPEAKER_PORT       PORTA
#define PIN_SPEAKER_DDR        DDRA
#define PIN_SPEAKER_BIT        5
#define PIN_SPEAKER_MASK       (1<<PIN_SPEAKER_BIT)

#define PIN_SENSE_PORT         PORTA
#define PIN_SENSE_DDR          DDRA
#define PIN_SENSE_A_BIT        0
#define PIN_SENSE_A_MASK       (1<<PIN_SENSE_A_BIT)
#define PIN_SENSE_B_BIT        1
#define PIN_SENSE_B_MASK       (1<<PIN_SENSE_B_BIT)
#define PIN_SENSE_B_ADMUX      1

#define PIN_PWRLED_PORT        PORTA
#define PIN_PWRLED_DDR         DDRA
#define PIN_PWRLED_BIT         3
#define PIN_PWRLED_MASK        (1<<PIN_PWRLED_BIT)

#define PIN_PWRBUTTON_PIN      PINA
#define PIN_PWRBUTTON_PORT     PORTA
#define PIN_PWRBUTTON_DDR      DDRA
#define PIN_PWRBUTTON_MASK     0b0101'0000

//#define SERIAL_OUT
// when using SERIAL_OUT, the files ATtinySerialOut.h and ATtinySerialOut.hpp
// from https://github.com/ArminJo/ATtinySerialOut/tree/v2.3.1/src must be placed
// into the source directory and the "#include <Arduino.h>" must be removed from
// ATtinySerialOut.h.
// Also note that ATtinySerialOut does not support the ATtinyX4A, just the non-A versions
// ATtinyX4. Use that type for building the project; it works with the A chips as well.

//#define SERIAL_PHASE_TEST
// used during development to determine the phase value that results is the largest signal.

#ifdef SERIAL_OUT

#define PIN_TX_PORT            PORTB
#define PIN_TX_DDR             DDRB
#define PIN_TX_BIT             2
#define PIN_TX_MASK            (1<<PIN_TX_BIT)
#define PIN_TX_ARDUINO         10

constexpr bool SERIAL_OUT_ON_SPEAKER_PIN = &PIN_SPEAKER_PORT == &PIN_TX_PORT && PIN_SPEAKER_BIT == PIN_TX_BIT;

#else

constexpr bool SERIAL_OUT_ON_SPEAKER_PIN = false;

#endif

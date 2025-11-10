#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "config.h"

#include <util/delay.h>

#ifdef SERIAL_OUT

#define digitalPinToPCMSKbit(p) (p&7)
#define TX_PIN PIN_TX_ARDUINO
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "ATtinySerialOut.hpp"

#endif

void set_no_beep()
{
   OCR1B = 0xFFFF;
   OCR1A = 1;
}

void set_beep_freq(uint16_t f)
{
   constexpr uint32_t base_freq = F_CPU / 64;
   static_assert(base_freq >= 0xFFFFu, "timer calculation assumes base_freq >= 65535 Hz");
   if (f < (base_freq+65535)>>16) {
      set_no_beep();
      } else {
      auto div = base_freq / f;
      OCR1B = (div>>1)-1;
      OCR1A = div-1;
   }
}

void bbtest_no_dead_time(uint8_t phase/*r24*/, uint16_t count/*r23:r22*/)
{
   asm volatile
   (
   "in   r20, %[oreg] \n\t"
   "andi r20, ~(%[ma] | %[mb]) \n\t"
   "mov  r18, r20     \n\t"
   "ori  r18, %[ma]   \n\t"
   "mov  r19, r20     \n\t"
   "ori  r19, %[mb]   \n\t"

   "out %[tcreg], r24 \n\t"

   "loop1:            \n\t"
   "out %[oreg], r18  \n\t" // 1 cycle
   "nop               \n\t" // 1 cycle
   "subi r22, 1       \n\t" // 1 cycle
   "sbc  r23, r1      \n\t" // 1 cycle
   "out %[oreg], r19  \n\t" // 1 cycle
   "nop               \n\t" // 1 cycle
   "brne loop1        \n\t" // 2 cycles

   "out %[oreg], r20  \n\t"
   :
   :
   [oreg]  "I" _SFR_IO_ADDR(PIN_SENSE_DDR),
   [ma]    "I" PIN_SENSE_A_MASK,
   [mb]    "I" PIN_SENSE_B_MASK,
   [tcreg] "I" _SFR_IO_ADDR(TCNT0)
   :
   );
}

void bbtest_with_dead_time(uint8_t phase/*r24*/, uint16_t count/*r23:r22*/)
{
   asm volatile
   (
   "in   r20, %[oreg] \n\t"
   "andi r20, ~(%[ma] | %[mb]) \n\t"
   "mov  r18, r20     \n\t"
   "ori  r18, %[ma]   \n\t"
   "mov  r19, r20     \n\t"
   "ori  r19, %[mb]   \n\t"

   "out %[tcreg], r24 \n\t"

   "loop2:            \n\t"
   "out %[oreg], r20  \n\t" // 1 cycle
   "out %[oreg], r18  \n\t" // 1 cycle
   "subi r22, 1       \n\t" // 1 cycle
   "sbc  r23, r1      \n\t" // 1 cycle
   "out %[oreg], r20  \n\t" // 1 cycle
   "out %[oreg], r19  \n\t" // 1 cycle
   "brne loop2        \n\t" // 2 cycles

   "out %[oreg], r20  \n\t"
   :
   :
   [oreg]  "I" _SFR_IO_ADDR(PIN_SENSE_DDR),
   [ma]    "I" PIN_SENSE_A_MASK,
   [mb]    "I" PIN_SENSE_B_MASK,
   [tcreg] "I" _SFR_IO_ADDR(TCNT0)
   :
   );
}

void prepare_measure()
{
   // SENSE_A output HIGH
   // SENSE_B output LOW
   PIN_SENSE_PORT |= PIN_SENSE_A_MASK;
   PIN_SENSE_PORT &= ~PIN_SENSE_B_MASK;
   PIN_SENSE_DDR |= (PIN_SENSE_A_MASK | PIN_SENSE_B_MASK);

   // wait 50us for Cap to charge to VDD
   _delay_us(50);

   // SENSE_A open
   // SENSE_B open
   MCUCR |= _BV(PUD); // disable pull-up
   PIN_SENSE_DDR &= ~(PIN_SENSE_A_MASK | PIN_SENSE_B_MASK);
}

uint16_t get_measure_result()
{
   // SENSE_A output HIGH, SENSE_B left floating
   PIN_SENSE_DDR |= PIN_SENSE_A_MASK;
   ADMUX = PIN_SENSE_B_ADMUX;
   ADCSRA |= _BV(ADSC);
   while (ADCSRA & _BV(ADSC)) {}
   ADMUX = 0b100000; // GND input
   MCUCR &= ~_BV(PUD); // enable pull-up
   return ADC;
}

uint16_t measure(uint8_t phase, bool dt)
{
   prepare_measure();
   if (dt) {
      bbtest_with_dead_time(phase, measure_signal_cycles);
   } else {
      bbtest_no_dead_time(phase, measure_signal_cycles);
   }
   return get_measure_result();
}

void do_measure()
{
   auto m = measure(measure_phase, true);
   set_beep_freq(8*m);
#ifdef SERIAL_OUT
   Serial.print(m);
   Serial.print("\r\n");
#endif
}

#ifdef PIN_PWRBUTTON_MASK

ISR(PCINT0_vect) {}
ISR(PCINT1_vect) {}

inline bool power_button_pressed()
{
   return (PIN_PWRBUTTON_PIN & PIN_PWRBUTTON_MASK) != PIN_PWRBUTTON_MASK;
}

bool power_button_press()
{
   static bool was_pressed = false;
   bool pressed = power_button_pressed();
   auto ret = pressed & !was_pressed;
   was_pressed = pressed;
   return ret;
}

void power_off()
{
   // all ports to init state
   PORTA = INIT_PORTA;
   DDRA = INIT_DDRA;
   PORTB = INIT_PORTB;
   DDRB = INIT_DDRB;
   // timer output off
   TCCR0A &= ~(_BV(COM0B1) | _BV(COM0B0));
   TCCR1A &= ~(_BV(COM1B1) | _BV(COM1B0));
   // ADC off
   ADCSRA = 0;

   // end sleep on pin-change
   if (&PIN_PWRBUTTON_PORT == &PORTA) {
      GIMSK = _BV(PCIE0);
      PCMSK0 = PIN_PWRBUTTON_MASK;
   } else {
      GIMSK = _BV(PCIE1);
      PCMSK1 = PIN_PWRBUTTON_MASK;
   }

   // wait for button release with debounce
   for(uint8_t i = 0; i < 10; ++i) {
      if (power_button_pressed()) {
         i = 0;
      } else {
         _delay_ms(1);
      }
   }

   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_enable();
   sleep_bod_disable();
   sleep_cpu();
   sleep_disable();

   // no pin change interrupts any more
   GIMSK = 0;

   // debounce
   _delay_ms(10);

   // consume "press" of power-on
   power_button_press();
}

#endif

int main(void)
{
   // switch to 8 MHz
   CLKPR = 0x80;
   CLKPR = 0;

   MCUCR |= _BV(BODS) | _BV(BODSE);


#ifdef PIN_PWRBUTTON_MASK
   // do not handle auto-power-off after getting external power, only use
   // power-button to switch off. This allows to use the same code for
   // devices with and without power-button.
   bool first_power_on = true;
#endif
   sei();

   for(;;) {
      DIDR0 = PIN_SENSE_A_MASK | PIN_SENSE_B_MASK;
      PORTA = INIT_PORTA;
      DDRA = INIT_DDRA;
      PORTB = INIT_PORTB;
      DDRB = INIT_DDRB;

#ifdef PIN_PWRLED_MASK
      PORTA |= PIN_PWRLED_MASK;
#endif
#ifdef SERIAL_OUT
      initTXPin();
#endif

      // ADC
      ADMUX = 0b100000; // GND input
      ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1); // ADC clock = CPU / 64 = 125 kHz
      ADCSRB = 0;

      // init Timer0 Fast PWM Mode with TOP = OCR0A
      TCNT0 = 0;
      OCR0A = 7;
      OCR0B = 3;
      TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
      TCCR0B = _BV(WGM02) | _BV(CS00);

      // init Timer1 Fast PWM with TOP = OCR1A, clock = FCPU/64
      TCNT1 = 0;
      set_no_beep();
      if (SERIAL_OUT_ON_SPEAKER_PIN) {
         TCCR1A =                             _BV(WGM11) | _BV(WGM10);
      } else {
         TCCR1A = _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11) | _BV(WGM10);
      }
      TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11) | _BV(CS10);


#ifdef SERIAL_PHASE_TEST
      for(;;) {
         Serial.print("\r\n\r\nNDT");
         for(uint8_t p = 0; p <= 7; ++p) {
            Serial.print(" P");
            Serial.print((char)('0'+p));
            Serial.print(' ');
            Serial.print(measure(p, false));
         }
         _delay_ms(100);
         Serial.print("\r\nDT ");
         for(uint8_t p = 0; p <= 7; ++p) {
            Serial.print(" P");
            Serial.print((char)('0'+p));
            Serial.print(' ');
            Serial.print(measure(p, true));
         }
         _delay_ms(500);
      }
#endif

#ifdef PIN_PWRBUTTON_MASK

      if (first_power_on) {
         first_power_on = false;
         do {
            do_measure();
         } while (!power_button_press());
      } else {
         for(uint16_t auto_off_countdown = power_off_measure_count; auto_off_countdown != 0 && !power_button_press(); --auto_off_countdown) {
            do_measure();
         }
      }
      power_off();

#else

      for(;;) {
         do_measure();
      }

#endif
   }
}

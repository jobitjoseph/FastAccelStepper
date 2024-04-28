#ifndef AVRSTEPPERPINS_H
#define AVRSTEPPERPINS_H
#include <Arduino.h>

/** * Warning: Other libraries may also use the timers!
 *
 * For example Serial library and delay() functions for example.
 * Using the same timer may cause strange effects, you are best to avoid using
 * those other libraries at the some time or use a different pin where
 * possible!
 */

#if defined(ARDUINO_ARCH_AVR)
#if !(defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
      defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || \
      defined(__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__))
#error "Unsupported AVR derivate"
#endif
#endif

// The ATmega328P has one 16 bit timer: Timer 1
// The ATmega2560 has four 16 bit timers: Timer 1, 3, 4 and 5
#if (defined(__AVR_ATmega168__) || defined(__AVR_ATmega168P__) || \
     defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__))
#define FAS_TIMER_MODULE 1
#define stepPinStepper1A 9  /* OC1A */
#define stepPinStepper1B 10 /* OC1B */
#define OC1A_PORT PORTB
#define OC1A_PIN PINB
#define OC1A_BIT 0x20
#define OC1B_PORT PORTB
#define OC1B_PIN PINB
#define OC1B_BIT 0x40
#elif defined(__AVR_ATmega2560__)
#ifndef FAS_TIMER_MODULE
#define FAS_TIMER_MODULE 4
#endif
#define stepPinStepper1A 11 /* OC1A */
#define stepPinStepper1B 12 /* OC1B */
#define stepPinStepper1C 13 /* OC1C */
#define stepPinStepper3A 5  /* OC3A */
#define stepPinStepper3B 2  /* OC3B */
#define stepPinStepper3C 3  /* OC3C */
#define stepPinStepper4A 6  /* OC4A */
#define stepPinStepper4B 7  /* OC4B */
#define stepPinStepper4C 8  /* OC4C */
#define stepPinStepper5A 46 /* OC5A */
#define stepPinStepper5B 45 /* OC5B */
#define stepPinStepper5C 44 /* OC5C */

#define OC1A_PORT PORTB
#define OC1A_PIN PINB
#define OC1A_BIT 0x20
#define OC1B_PORT PORTB
#define OC1B_PIN PINB
#define OC1B_BIT 0x40
#define OC1C_PORT PORTB
#define OC1C_PIN PINB
#define OC1C_BIT 0x80

#define OC3A_PORT PORTC
#define OC3A_PIN PINC
#define OC3A_BIT 0x08
#define OC3B_PORT PORTC
#define OC3B_PIN PINC
#define OC3B_BIT 0x10
#define OC3C_PORT PORTC
#define OC3C_PIN PINC
#define OC3C_BIT 0x20

#define OC4A_PORT PORTH
#define OC4A_PIN PINH
#define OC4A_BIT 0x08
#define OC4B_PORT PORTH
#define OC4B_PIN PINH
#define OC4B_BIT 0x10
#define OC4C_PORT PORTH
#define OC4C_PIN PINH
#define OC4C_BIT 0x20

#define OC5A_PORT PORTL
#define OC5A_PIN PINL
#define OC5A_BIT 0x08
#define OC5B_PORT PORTL
#define OC5B_PIN PINL
#define OC5B_BIT 0x10
#define OC5C_PORT PORTL
#define OC5C_PIN PINL
#define OC5C_BIT 0x20

#elif defined(__AVR_ATmega32U4__)
#define FAS_TIMER_MODULE 1
#define stepPinStepper1A 9  /* OC1A */
#define stepPinStepper1B 10 /* OC1B */
#define stepPinStepper1C 11 /* OC1C */
#define OC1A_PORT PORTB
#define OC1A_PIN PINB
#define OC1A_BIT 0x20
#define OC1B_PORT PORTB
#define OC1B_PIN PINB
#define OC1B_BIT 0x40
#define OC1C_PORT PORTB
#define OC1C_PIN PINB
#define OC1C_BIT 0x80
#endif

#if (FAS_TIMER_MODULE == 1)
#define stepPinStepperA stepPinStepper1A
#define stepPinStepperB stepPinStepper1B
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__)
#define stepPinStepperC stepPinStepper1C
#endif
#elif (FAS_TIMER_MODULE == 3)
#define stepPinStepperA stepPinStepper3A
#define stepPinStepperB stepPinStepper3B
#if defined(__AVR_ATmega2560__)
#define stepPinStepperC stepPinStepper3C
#endif
#elif (FAS_TIMER_MODULE == 4)
#define stepPinStepperA stepPinStepper4A
#define stepPinStepperB stepPinStepper4B
#if defined(__AVR_ATmega2560__)
#define stepPinStepperC stepPinStepper4C
#endif
#elif (FAS_TIMER_MODULE == 5)
#define stepPinStepperA stepPinStepper5A
#define stepPinStepperB stepPinStepper5B
#if defined(__AVR_ATmega2560__)
#define stepPinStepperC stepPinStepper5C
#endif
#endif

#endif  // AVRSTEPPERPINS_H

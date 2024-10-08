#include "test_seq.h"

bool test_seq_09(FastAccelStepper *stepper, struct test_seq_s *seq,
                 uint32_t time_ms) {
  switch (seq->state) {
    case 0:  // INIT
      srand(135);
#if defined(SUPPORT_ESP32_PULSE_COUNTER)
      if (!stepper->attachToPulseCounter(7)) {
        PRINTLN("Error attaching to pulse counter");
        seq->state = TEST_STATE_ERROR;
        return true;
      }
#endif
      seq->u32_1 = time_ms;
      seq->state++;
      break;
    case 1:
      if (!stepper->isRunning()) {
#define VMIN 40
#define VMAX 16384L
        uint32_t speed = rand() % (VMAX * 4);
        speed = speed >> ((speed % 4) + 2);
        speed = speed + VMIN;

#define AMIN 10
#define AMAX (4 * 65536)
        uint32_t accel = rand() % (AMAX * 4);
        accel = accel >> ((accel % 4) + 2);
        accel = accel + AMIN;
        PRINT("speed=");
        PRINTU32(speed);
        PRINT(" accel=");
        PRINTU32(accel);
        PRINTLN("");
        stepper->setSpeedInUs(speed);
        stepper->setAcceleration(accel);
        if (rand() & 1) {
          stepper->runForward();
        } else {
          stepper->runBackward();
        }
        seq->u32_1 = time_ms;
        seq->state++;
      }
      break;
    case 2:
      if (time_ms - seq->u32_1 >= 1000) {
#if defined(SUPPORT_ESP32_PULSE_COUNTER)
        seq->s16_1 = stepper->readPulseCounter();
#endif
        stepper->forceStopAndNewPosition(0);
        seq->u32_1 = time_ms;
        seq->state++;
      }
      break;
    case 3:
      if (!stepper->isRunning()) {
#if defined(SUPPORT_ESP32_PULSE_COUNTER)
        int16_t old = seq->s16_1;
        seq->s16_1 = stepper->readPulseCounter();
        PRINT("Steps needed for stop=");
        PRINTU16(old - seq->s16_1);
        PRINTLN("");
#endif
        seq->state++;
      }
      break;
    case 4:
      if (time_ms - seq->u32_1 >= 100) {
#if defined(SUPPORT_ESP32_PULSE_COUNTER)
        if (seq->s16_1 != stepper->readPulseCounter()) {
          PRINTLN("Step AFTER stop");
        }
#endif
        seq->u32_1 = time_ms;
        seq->state = 1;
      }
      break;
  }
  return false;
}

#include "test_seq.h"

//
// This test sequence is to reproduce issue #68

bool test_seq_11(FastAccelStepper *stepper, struct test_seq_s *seq,
                 uint32_t time_ms) {
  uint32_t prev_pos;
  switch (seq->state) {
    case 0:  // INIT
      seq->s32_1 = stepper->getCurrentPosition();
      if (seq->s32_1 != 0) {
        seq->state = TEST_STATE_ERROR;
        return true;
      }
      stepper->setSpeedInUs(43);
      stepper->setAcceleration(10000);
      stepper->moveTo(1000000);
      seq->state++;
      break;
    case 1:
      if (!stepper->isRunning()) {
        stepper->moveTo(0);
        seq->state++;
        break;
      }
      prev_pos = seq->s32_1;
      for (uint16_t i = 0; i <= 1000; i++) {
        int32_t pos = stepper->getCurrentPosition();
        if (pos < seq->s32_1) {
          PRINT("i=");
          PRINTU16(i);
          if (i != 0) {
            PRINT(" prev pos=");
            PRINTI32(prev_pos);
          }
          PRINT(" old pos=");
          PRINTI32(seq->s32_1);
          PRINT(" curr pos=");
          PRINTI32(pos);
          PRINTLN("");
          stepper->stopMove();
          seq->state = TEST_STATE_ERROR;
          return true;
        }
        prev_pos = seq->s32_1;
        seq->s32_1 = pos;
      }
      break;
    case 2:
      if (!stepper->isRunning()) {
        return true;
      }
      prev_pos = seq->s32_1;
      for (uint16_t i = 0; i <= 1000; i++) {
        int32_t pos = stepper->getCurrentPosition();
        if (pos > seq->s32_1) {
          PRINT("i=");
          PRINTU16(i);
          if (i != 0) {
            PRINT(" prev pos=");
            PRINTI32(prev_pos);
          }
          PRINT(" old pos=");
          PRINTI32(seq->s32_1);
          PRINT(" curr pos=");
          PRINTI32(pos);
          PRINTLN("");
          stepper->stopMove();
          seq->state = TEST_STATE_ERROR;
          return true;
        }
        prev_pos = seq->s32_1;
        seq->s32_1 = pos;
      }
      break;
  }
  return false;
}

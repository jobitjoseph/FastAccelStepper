#if defined(ARDUINO_ARCH_AVR)
#include "AVRStepperPins.h"
#include "FastAccelStepper.h"
#include "StepperISR.h"

// T is the timer module number 0,1,2,3...
// X is the Channel name A or B
#define Stepper_Toggle(T, X) \
  TCCR##T##A = (TCCR##T##A | _BV(COM##T##X##0)) & ~_BV(COM##T##X##1)
#define Stepper_Zero(T, X) \
  TCCR##T##A = (TCCR##T##A | _BV(COM##T##X##1)) & ~_BV(COM##T##X##0)
#define Stepper_Disconnect(T, X) \
  TCCR##T##A = (TCCR##T##A & ~(_BV(COM##T##X##1) | _BV(COM##T##X##0)))
#define Stepper_IsToggling(T, X) \
  ((TCCR##T##A & (_BV(COM##T##X##0) | _BV(COM##T##X##1))) == _BV(COM##T##X##0))
#define Stepper_IsDisconnected(T, X) \
  ((TCCR##T##A & (_BV(COM##T##X##0) | _BV(COM##T##X##1))) == 0)

#ifdef SIMAVR_TIME_MEASUREMENT
#define prepareISRtimeMeasurement() DDRB |= 0x18
#define enterStepperISR() PORTB |= 0x08
#define exitStepperISR() PORTB ^= 0x08
#define enterFillQueueISR() PORTB |= 0x10
#define exitFillQueueISR() PORTB ^= 0x10
#elif defined(SIMAVR_TIME_MEASUREMENT_QUEUE)
#define prepareISRtimeMeasurement() DDRB |= 0x10
#define enterStepperISR() \
  {}
#define exitStepperISR() \
  {}
#define enterFillQueueISR() PORTB |= 0x10
#define exitFillQueueISR() PORTB ^= 0x10
#else
#define prepareISRtimeMeasurement() \
  {}
#define enterStepperISR() \
  {}
#define exitStepperISR() \
  {}
#define enterFillQueueISR() \
  {}
#define exitFillQueueISR() \
  {}
#endif

#ifdef SIMAVR_FOC_WORKAROUND
#define ForceCompare(T, X)                    \
  {                                           \
    uint16_t x = OCR##T##X;                   \
    OCR##T##X = TCNT##T + 8;                  \
    while ((TIFR##T & _BV(OCF##T##X)) == 0) { \
    };                                        \
    OCR##T##X = x;                            \
  }                                           \
  ClearInterruptFlag(T, X); 
#else
#define ForceCompare(T, X) TCCR##T##C = _BV(FOC##T##X)
#endif
#define DisableCompareInterrupt(T, X) TIMSK##T &= ~_BV(OCIE##T##X)
#define EnableCompareInterrupt(T, X) TIMSK##T |= _BV(OCIE##T##X)
#define ClearInterruptFlag(T, X) TIFR##T = _BV(OCF##T##X)
#define SetTimerCompareRelative(T, X, D) OCR##T##X = TCNT##T + D

#define ConfigureTimer(T)                                                 \
  {                                                                       \
    /* Set WGMn3:0 to all zero => Normal mode */                          \
    TCCR##T##A &= ~(_BV(WGM##T##1) | _BV(WGM##T##0));                     \
    TCCR##T##B &= ~(_BV(WGM##T##3) | _BV(WGM##T##2));                     \
    /* Set prescaler to 1 */                                              \
    TCCR##T##B =                                                          \
        (TCCR##T##B & ~(_BV(CS##T##2) | _BV(CS##T##1) | _BV(CS##T##0))) | \
        _BV(CS##T##0);                                                    \
  }
#define EnableOverflowInterrupt(T) TIMSK##T |= _BV(TOIE##T)
#define DisableOverflowInterrupt(T) TIMSK##T &= ~_BV(TOIE##T)

// Here are the global variables to interface with the interrupts
StepperQueue fas_queue[NUM_QUEUES];

#define AVR_INIT(T, CHANNEL)                       \
  {                                                \
    /* Disconnect stepper on next compare event */ \
    Stepper_Disconnect(T, CHANNEL);                \
    /* disable compare A interrupt */              \
    DisableCompareInterrupt(T, CHANNEL);           \
    /* force compare to ensure disconnect */       \
    ForceCompare(T, CHANNEL);                      \
    /* Initialize timer for correct time base */   \
    ConfigureTimer(T);                             \
    /* ensure cyclic interrupt is running */       \
    EnableOverflowInterrupt(T);                    \
  }
void StepperQueue::init(uint8_t queue_num, uint8_t step_pin) {
  prepareISRtimeMeasurement();
  _initVars();
  digitalWrite(step_pin, LOW);
  pinMode(step_pin, OUTPUT);
  if (step_pin == stepPinStepperA) {
    channel = channelA;
    AVR_INIT(FAS_TIMER_MODULE, A)
  }
  if (step_pin == stepPinStepperB) {
    channel = channelB;
    AVR_INIT(FAS_TIMER_MODULE, B)
  }
#ifdef stepPinStepperC
  if (step_pin == stepPinStepperC) {
    channel = channelC;
    AVR_INIT(FAS_TIMER_MODULE, C)
  }
#endif
}

// The interrupt is called on compare event, which eventually
// generates a L->H transition. In any case, the current command's
// wait time has still to be executed for the next command, if any.
//
// If reaching here without further commands, then the queue is done
#define AVR_STEPPER_ISR_NEXT(T, CHANNEL, FOC_STATE_ZERO)              \
  rp++;                                                               \
  fas_queue_##CHANNEL.read_idx = rp;                                  \
  if (rp != fas_queue_##CHANNEL.next_write_idx) {                     \
    /* command in queue */                                            \
    e = &fas_queue_##CHANNEL.entry[rp & QUEUE_LEN_MASK];              \
    if (e->steps == 0) {                                              \
      if (!FOC_STATE_ZERO) {                                          \
        Stepper_Zero(T, CHANNEL);                                     \
      }                                                               \
    } else {                                                          \
      if (FOC_STATE_ZERO) {                                           \
        Stepper_Toggle(T, CHANNEL);                                   \
      }                                                               \
    }                                                                 \
    if (e->toggle_dir) {                                              \
      uint8_t dirPin = fas_queue_##CHANNEL.dirPin;                    \
      digitalWrite(dirPin, digitalRead(dirPin) == HIGH ? LOW : HIGH); \
    }                                                                 \
  } else {                                                            \
    if (!FOC_STATE_ZERO) {                                            \
      Stepper_Zero(T, CHANNEL);                                       \
    }                                                                 \
    fas_queue_##CHANNEL._prepareForStop = true;                       \
  }

#define AVR_STEPPER_ISR(T, CHANNEL)                                          \
  ISR(TIMER##T##_COMP##CHANNEL##_vect) {                                     \
    enterStepperISR();                                                       \
    uint8_t rp = fas_queue_##CHANNEL.read_idx;                               \
    if (rp == fas_queue_##CHANNEL.next_write_idx) {                          \
      /* queue is empty => set to disconnect */                              \
      Stepper_Disconnect(T, CHANNEL);                                        \
      /* force compare to ensure disconnect */                               \
      ForceCompare(T, CHANNEL);                                              \
      /* disable compare interrupt */                                        \
      DisableCompareInterrupt(T, CHANNEL);                                   \
      fas_queue_##CHANNEL._isRunning = false;                                \
      fas_queue_##CHANNEL._prepareForStop = false;                           \
      fas_queue_##CHANNEL.queue_end.ticks = TICKS_FOR_STOPPED_MOTOR;         \
      exitStepperISR();                                                      \
      return;                                                                \
    }                                                                        \
    struct queue_entry* e = &fas_queue_##CHANNEL.entry[rp & QUEUE_LEN_MASK]; \
    OCR##T##CHANNEL += e->ticks;                                             \
    if (Stepper_IsToggling(T, CHANNEL)) {                                    \
      /* Clear output bit by another toggle */                               \
      ForceCompare(T, CHANNEL);                                              \
      if (e->steps-- > 1) {                                                  \
        /* perform another step with this queue entry */                     \
        exitStepperISR();                                                    \
        return;                                                              \
      }                                                                      \
      /* Here the output compare is in Toggle mode */                        \
      AVR_STEPPER_ISR_NEXT(T, CHANNEL, false)                                \
    } else {                                                                 \
      if (fas_queue_##CHANNEL._prepareForStop) {                             \
        /* new command received after running out of commands */             \
        /* if this new command requires a step, then this step would be lost \
         */                                                                  \
        fas_queue_##CHANNEL._prepareForStop = false;                         \
        if (e->steps > 0) {                                                  \
          /* That's the problem, so generate a step */                       \
          Stepper_Toggle(T, CHANNEL);                                        \
          ForceCompare(T, CHANNEL);                                          \
          /* Use a high time of 3us */                                       \
          delayMicroseconds(3);                                              \
          /* Clear output bit by another toggle */                           \
          ForceCompare(T, CHANNEL);                                          \
          if (e->steps-- > 1) {                                              \
            /* perform another step with this queue entry */                 \
            exitStepperISR();                                                \
            return;                                                          \
          }                                                                  \
      /* Here the output compare is in Toggle mode */                        \
      AVR_STEPPER_ISR_NEXT(T, CHANNEL, false)                                \
            exitStepperISR();                                                \
            return;                                                          \
        }                                                                    \
      }                                                                      \
      /* Here the output compare is in Zero mode */                          \
      AVR_STEPPER_ISR_NEXT(T, CHANNEL, true)                                 \
    }                                                                        \
    exitStepperISR();                                                        \
  }
#define AVR_STEPPER_ISR_GEN(T, CHANNEL) AVR_STEPPER_ISR(T, CHANNEL)
AVR_STEPPER_ISR_GEN(FAS_TIMER_MODULE, A)
AVR_STEPPER_ISR_GEN(FAS_TIMER_MODULE, B)
#ifdef stepPinStepperC
AVR_STEPPER_ISR_GEN(FAS_TIMER_MODULE, C)
#endif

// this is for cyclic task
#define AVR_CYCLIC_ISR(T)                          \
  ISR(TIMER##T##_OVF_vect) {                       \
    enterFillQueueISR();                           \
                                                   \
    /* disable OVF interrupt to avoid nesting */   \
    DisableOverflowInterrupt(T);                   \
                                                   \
    /* enable interrupts for nesting */            \
    interrupts();                                  \
                                                   \
    /* manage steppers */                          \
    fas_engine->manageSteppers();                  \
                                                   \
    /* disable interrupts for exist ISR routine */ \
    noInterrupts();                                \
                                                   \
    /* enable OVF interrupt again */               \
    EnableOverflowInterrupt(T);                    \
                                                   \
    exitFillQueueISR();                            \
  }
#define AVR_CYCLIC_ISR_GEN(T) AVR_CYCLIC_ISR(T)
AVR_CYCLIC_ISR_GEN(FAS_TIMER_MODULE)

#define AVR_START_QUEUE(T, CHANNEL)                                          \
  {                                                                          \
    /* force compare to ensure zero */                                       \
    Stepper_Zero(T, CHANNEL);                                                \
    ForceCompare(T, CHANNEL);                                                \
    /* enter critical section */                                             \
    noInterrupts();                                                          \
    /* ensure no compare event */                                            \
    SetTimerCompareRelative(T, CHANNEL, 32768);                              \
    /* set toggle mode, if steps to be generated */                          \
    uint8_t rp = fas_queue_##CHANNEL.read_idx;                               \
    struct queue_entry* e = &fas_queue_##CHANNEL.entry[rp & QUEUE_LEN_MASK]; \
    if (e->steps > 0) {                                                      \
      Stepper_Toggle(T, CHANNEL);                                            \
    } else {                                                                 \
      Stepper_Zero(T, CHANNEL);                                              \
    }                                                                        \
    if (e->toggle_dir) {                                                     \
      uint8_t dirPin = fas_queue_##CHANNEL.dirPin;                           \
      digitalWrite(dirPin, digitalRead(dirPin) == HIGH ? LOW : HIGH);        \
    }                                                                        \
    /* clear interrupt flag */                                               \
    ClearInterruptFlag(T, CHANNEL);                                          \
    /* enable compare interrupt */                                           \
    EnableCompareInterrupt(T, CHANNEL);                                      \
    /* definite start point */                                               \
    SetTimerCompareRelative(T, CHANNEL, 10);                                 \
    interrupts();                                                            \
  }

void StepperQueue::startQueue() {
  if (_isRunning) {
    return;
  }
  if (isQueueEmpty()) {
    return;
  }
  _isRunning = true;
  _prepareForStop = false;
  switch (channel) {
    case channelA:
      AVR_START_QUEUE(FAS_TIMER_MODULE, A)
      break;
    case channelB:
      AVR_START_QUEUE(FAS_TIMER_MODULE, B)
      break;
#ifdef stepPinStepperC
    case channelC:
      AVR_START_QUEUE(FAS_TIMER_MODULE, C)
      break;
#endif
  }
}

#define FORCE_STOP(T, CHANNEL)               \
  {                                          \
    /* disable compare interrupt */          \
    DisableCompareInterrupt(T, CHANNEL);     \
    /* set to disconnect */                  \
    Stepper_Disconnect(T, CHANNEL);          \
    /* force compare to ensure disconnect */ \
    ForceCompare(T, CHANNEL);                \
  }
void StepperQueue::forceStop() {
  switch (channel) {
    case channelA:
      FORCE_STOP(FAS_TIMER_MODULE, A)
      break;
    case channelB:
      FORCE_STOP(FAS_TIMER_MODULE, B)
      break;
#ifdef stepPinStepperC
    case channelC:
      FORCE_STOP(FAS_TIMER_MODULE, C)
      break;
#endif
  }
  _isRunning = false;
  queue_end.ticks = TICKS_FOR_STOPPED_MOTOR;

  // empty the queue
  read_idx = next_write_idx;
}
void StepperQueue::connect() {}
void StepperQueue::disconnect() {}
bool StepperQueue::isValidStepPin(uint8_t step_pin) {
  if (step_pin == stepPinStepperA) {
    return true;
  }
  if (step_pin == stepPinStepperB) {
    return true;
  }
#ifdef stepPinStepperC
  if (step_pin == stepPinStepperC) {
    return true;
  }
#endif
  return false;
}

int8_t StepperQueue::queueNumForStepPin(uint8_t step_pin) {
  if (step_pin == stepPinStepperA) {
    return 0;
  }
  if (step_pin == stepPinStepperB) {
    return 1;
  }
#ifdef stepPinStepperC
  if (step_pin == stepPinStepperC) {
    return 2;
  }
#endif
  return -1;
}
#endif

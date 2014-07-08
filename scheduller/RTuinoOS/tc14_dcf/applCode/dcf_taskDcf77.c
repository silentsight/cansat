/**
 * @file tc14_dcf/applCode/dcf_taskDcf77.c
 *   This module implements a task, that can be used to sample and decode a demodulated
 * DCF77 time signal. The task is autonomous. It may be integrated into any RTuinOS
 * application as an additional task. The decoded time information is provided to the
 * rest of the application via a callback.\n
 *   Successful DCF time information reception requires hardware, which is not available on
 * the Arduino board. You need to connect an external DCF77 receiver to the board. The
 * receiver's output signal is a digital pulse of low frequency and two-valued duty cycle.
 * The way how to connect a receiver hardware is out of scope of this module. The task's
 * interface is an only declared but externally implemented function, which returns the
 * current, Boolean signal state to the DCF task. Part of the integration of the DCF task
 * into an application environment is to provide this function. You'll write a function
 * that considers your specific hardware conditions. Please see
 * tc14_dcf/applCode/dcf_taskDcf77.h, boolean dcf_readDcfInput(void) for more.\n
 *   The task is regular and can operate at different task cycle times. (The absolute time
 * unit is derived from RTuinOS configuration define #RTOS_TIC.) Due to resolution issues
 * and rounding effects this holds true only in a certain range and it further depends on
 * the specified task priority and timing inaccuracies because of other tasks and their
 * priorities. This might require some adjustment of the time constants defined in this
 * module. Look for TODO tags to get more information how to adjust the module for your
 * specific environment, or application respectively.
 *
 * Copyright (C) 2013 Peter Vranken (mailto:Peter_Vranken@Yahoo.de)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
/* Module interface
 *   dcf_initializeTask
 * Local functions
 *   taskDcf
 *   sampleAndDecodeDcfSignal
 */

/*
 * Include files
 */

#include <Arduino.h>
#include <stdio.h>
#include "rtos.h"
#include "dcf_taskDcf77.h"


/*
 * Defines
 */

/** The desired cycle time of the regular task in unit s. The actual task time, see
    #ACTUAL_TASK_TIME_IN_S, can deviate as the task time needs to be a multiple of the
    given RTuinOS system timer tic. The latter macro rounds to the nearest number of
    tics.\n
      The actual, rounded cycle time of the regular DCF task is the sample time of the DCF
    signal at the same time and it is the unit of all impulse/pause duration
    measurements.\n
      If the system timer tic is too large rounding issues can lead to failures in the
    recognition of good telegrams or - much worse but much less probable - to failing
    recognition of bad telegrams.\n
      The smaller the task cycle time the better the DCF decoding result but the higher the
    CPU consumption of the DCF task is.\n
      Another consideration is the priority in which the DCF task is run. The higher it is,
    the higher the accuracy of the task timing becomes and the larger the task cycle time
    can become without endangering successful DCF signal sampling and decoding.\n
      A receiver of lower quality or bad reception conditions will demand a smaller task
    time than good reception conditions.
      @todo Set the desired task time as large as possible for the conditions given in your
    application. Most promising are values between 10 and 30 ms. */
#define DESIRED_TASK_TIME_IN_S (25e-3 /* s */)


/** The cycle time of the regular task and the sample time of the DCF signal at the same
    time, given in task tics, i.e. in multiples of the task cycle time. */
#define TASK_TIME_IN_TICS ((uintTime_t)((DESIRED_TASK_TIME_IN_S / (RTOS_TIC)) + 0.5))

/** The cylce time of the regular task in unit s. */
#define ACTUAL_TASK_TIME_IN_S ((float)(TASK_TIME_IN_TICS) * (RTOS_TIC))

/** Duration of the pause that indicates a new telegram in task tics. */
#define MIN_PAUSE_AT_START_OF_MIN_IN_TICS   TIME_IN_TICS(1.7 /* s */)

/** Minimum tolerated duration of a normal bit cycle, which has a nominal length of 1 second.
      @todo You might need to adjust the time towards smaller values if you have problems
    decoding the DCF signal. */
#define MIN_LEN_BIT_CYCLE_IN_TICS       TIME_IN_TICS(1.0 - 0.02)

/** Maximum tolerated duration of a normal bit cycle, which has a nominal length of 1 second.
      @todo You might need to adjust the time towards larger values if you have problems
    decoding the DCF signal. */
#define MAX_LEN_BIT_CYCLE_IN_TICS       TIME_IN_TICS(1.0 + 0.02)

/** Minimum duration of a short time impulse, a null bit.
      @todo You might need to adjust the time towards smaller values if you have problems
    decoding the DCF signal. */
#define MIN_LEN_BIT_0_IN_TICS           TIME_IN_TICS(0.1 - 0.03)

/** Maximum duration of a short time impulse, a null bit.
      @todo You might need to adjust the time towards larger values if you have problems
    decoding the DCF signal. */
#define MAX_LEN_BIT_0_IN_TICS           TIME_IN_TICS(0.1 + 0.03)

/** Minimum duration of a long time impulse, a one bit.
      @todo You might need to adjust the time towards smaller values if you have problems
    decoding the DCF signal. */
#define MIN_LEN_BIT_1_IN_TICS           TIME_IN_TICS(0.2 - 0.02)

/** Maximum duration of a long time impulse, a one bit.
      @todo You might need to adjust the time towards larger values if you have problems
    decoding the DCF signal. */
#define MAX_LEN_BIT_1_IN_TICS           TIME_IN_TICS(0.2 + 0.03)

/** Rescale a time span T in task tics. T is a literal floating point constant. The result
    is a uint8_t, which defines the maximum time T, that can be handled without overflow. */
#define TIME_IN_TICS(T /* in seconds */ )                                               \
            ((uint8_t)(((T)/((TASK_TIME_IN_TICS)*(RTOS_TIC))) + 0.5))


/*
 * Local type definitions
 */


/*
 * Local prototypes
 */


/*
 * Data definitions
 */

static uint8_t _stackTSampleAndDecodeDcfSignal[DCF_TASK_STACK_SIZE];


/*
 * Function implementation
 */

/**
 * This function is regularly called. It reads the DCF signal input and does all the
 * processing, including the state machine, which is required to synchronize with the time
 * telegrams.
 */

static void sampleAndDecodeDcfSignal()
{
    /* Read the current signal value. */
    boolean s = dcf_readDcfInput();

    static uint8_t cntImpulse_ = 0
                 , cntPause_ = 0
                 , cntBit_ = 0
                 , parityBit_ = 0;
    static boolean s_lastValue_ = DCF_DIGITAL_LEVEL_DCF_IMPULSE;

    static dcf_timeInfo timeInfo_ = { /* minute */ 0
                                    , /* hour */ 0
                                    , /* day */ 1
                                    , /* dayOfWeek */ 1
                                    , /* month */ 1
                                    , /* validity */ 0
                                    , /* year */ 2000
                                    };
    static uint16_t lastMin_ = 0;

    /* Count pause and impulse duration. The time tic now ends the interval, which is
       considered to belong to the left state. Therefore we use the last value to decide,
       what to count. */
    if(s_lastValue_ == DCF_DIGITAL_LEVEL_DCF_IMPULSE)
        ++ cntImpulse_;
    else
        ++ cntPause_;

    /* An evaluating action is taken always at the end of a pair of impulse and pause, this
       is a strict 1s pulse (besides the start of minute event). */
    if(s == DCF_DIGITAL_LEVEL_DCF_IMPULSE  &&  s != s_lastValue_)
    {
        /* Start of a new impulse. */

        /* The check for task overruns can easily fail because of the client implemented
           notification callback, which is invoked when a new time telegram could be
           decoded. See dcf_onNewTimeTelegram. */
#if 0
        printf( "rtos_getTaskOverrunCounter: %u\n"
              , rtos_getTaskOverrunCounter(0, /* doReset */ false)
              );
#endif

        /* Increment bit counter. Or set it to its initial value 0 after any error or after
           re-synchronization. */
        ++ cntBit_;

        /* Classify the impulse. */
        uint8_t thisBit;
        if(cntImpulse_ >= MIN_LEN_BIT_0_IN_TICS  &&  cntImpulse_ <= MAX_LEN_BIT_0_IN_TICS)
            thisBit = 0;
        else if(cntImpulse_ >= MIN_LEN_BIT_1_IN_TICS  &&  cntImpulse_ <= MAX_LEN_BIT_1_IN_TICS)
            thisBit = 1;
        else
        {
            /* Bad bit, distortion, telegram is refused, restart. */
            thisBit = 0;
            cntBit_ = 0xff;
        }

        /** The printf statements in this task are intended for development and diagnostic
            purpose only. They must never be switched on at productive run-time as they
            invalidate the task timing.
              @todo Mainly enable this print command if you intend to adjust the bit or
            pause timing related time constants. However, be aware that enabling the printf
            might change the task timing significantly so that the printed values are not
            identical to the system state without the printf. This effect is lowered by
            selecting the highest possible baud rate for stdout or Serial, respectively. */
#if 0
        printf( "Bit %02u: %u, Impulse: %u tics, Cycle: %u tics\n"
              , cntBit_
              , thisBit
              , cntImpulse_
              , cntImpulse_ + cntPause_
              );
#endif
        /* Interpret bit depending on its position in the telegram. */
        if(cntBit_ == 0)
        {
            if(thisBit != 0)
            {
                /* Bad telegram, first bit is always 0. */
                cntBit_ = 0xff;
            }
        }
#if 0
        else if(cntBit_ >= 15  &&  cntBit_ <= 19)
        {
            /* Information about second antenna, time zone and switch of summer time zone,
               see e.g. http://www.stefan-buchgeher.info/elektronik/dcf/dcf.html. All of
               this information is useless for our application but could be evaluated here
               if desired. */
        }
#endif
        else if(cntBit_ == 20)
        {
            if(thisBit != 1)
            {
                /* Bad telegram, this bit is always 1. */
                cntBit_ = 0xff;
            }

            /* Reset parity for next use. */
            parityBit_ = 0;

            /* Reset time information. */
            timeInfo_.minute = 0;
            timeInfo_.hour = 0;
            timeInfo_.day = 0;
            timeInfo_.month = 0;
            timeInfo_.year = 2000;
            timeInfo_.dayOfWeek = 0;
        }

        /*
         * Decode the minute.
         */
        else if(cntBit_ >= 21  &&  cntBit_ <= 24)
        {
            /* Minute information, lower digit. */
            if(thisBit)
                timeInfo_.minute += (1 << (cntBit_-21));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ >= 25  &&  cntBit_ <= 27)
        {
            /* Minute information, upper digit. */
            if(thisBit)
                timeInfo_.minute += 10*(1 << (cntBit_-25));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 28)
        {
            /* Check 1st parity bit for minute information. */
            if(parityBit_ != thisBit)
            {
                /* Bad parity, bad telegram, reset decoding. */
                cntBit_ = 0xff;
            }
#if 0
            else
                printf("Minute is: %02u\n", timeInfo_.minute);
#endif

            /* Reset parity for next use. */
            parityBit_ = 0;
        }

        /*
         * Decode the hour.
         */
        else if(cntBit_ >= 29  &&  cntBit_ <= 32)
        {
            /* Hour information, lower digit. */
            if(thisBit)
                timeInfo_.hour += (1 << (cntBit_-29));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 33)
        {
            /* Hour information, upper digit, lower bit. */
            if(thisBit)
                timeInfo_.hour += 10;

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 34)
        {
            /* Hour information, upper digit, upper bit. */
            if(thisBit)
                timeInfo_.hour += 20;

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 35)
        {
            /* Check 2nd parity bit for hour information. */
            if(parityBit_ != thisBit)
            {
                /* Bad parity, bad telegram, reset decoding. */
                cntBit_ = 0xff;
            }
#if 0
            else
                printf("Hour is: %02u\n", timeInfo_.hour);
#endif
            /* Reset parity for next use. */
            parityBit_ = 0;
        }

        /*
         * Decode the day of month.
         */
        else if(cntBit_ >= 36  &&  cntBit_ <= 39)
        {
            /* Day of month information, lower digit. */
            if(thisBit)
                timeInfo_.day += (1 << (cntBit_-36));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 40)
        {
            /* Day of month information, upper digit, lower bit. */
            if(thisBit)
                timeInfo_.day += 10;

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 41)
        {
            /* Day of month information, upper digit, upper bit. */
            if(thisBit)
                timeInfo_.day += 20;

            /* Update parity. */
            parityBit_ ^= thisBit;
        }

        /*
         * Decode the day of week.
         */
        else if(cntBit_ >= 42  &&  cntBit_ <= 44)
        {
            /* Day of week information. */
            if(thisBit)
                timeInfo_.dayOfWeek += (1 << (cntBit_-42));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }

        /*
         * Decode the month.
         */
        else if(cntBit_ >= 45  &&  cntBit_ <= 48)
        {
            /* Month information, lower digit. */
            if(thisBit)
                timeInfo_.month += (1 << (cntBit_-45));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ == 49)
        {
            /* Month information, upper digit. */
            if(thisBit)
                timeInfo_.month += 10;

            /* Update parity. */
            parityBit_ ^= thisBit;
        }

        /*
         * Decode the year.
         */
        else if(cntBit_ >= 50  &&  cntBit_ <= 53)
        {
            /* Year information, lower digit. */
            if(thisBit)
                timeInfo_.year += (1 << (cntBit_-50));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }
        else if(cntBit_ >= 54  &&  cntBit_ <= 57)
        {
            /* Year information, upper digit. */
            if(thisBit)
                timeInfo_.year += 10*(1 << (cntBit_-54));

            /* Update parity. */
            parityBit_ ^= thisBit;
        }

        /*
         * Check parity of date information and validate telegram.
         */
        else if(cntBit_ == 58)
        {
            /* Check 3rd parity bit for date information. */
            if(parityBit_ != thisBit)
            {
                /* Bad parity, bad telegram, reset decoding. */
                cntBit_ = 0xff;
            }
            else
            {
                /* Validation of telegram: We compute the number of minutes and compare
                   with previous telegram. The number of successive, consistent telegrams
                   is defined to be the validity. The update of a (stable) real time clock
                   should be done at a validity of >=2 only.
                     The number of minutes is taken from time of day and day of month only.
                   This leads to a maximum number of minutes in the uint16_t range.
                   Considering the year would require expensive 32 Bit operations. And the
                   only drawback of not doing so is the badly determined validity once a
                   month - once a month there's a good chance that an RTC would not be
                   synchronized although we actually have a good telegram, but only a few
                   minutes later it would be re-synchronized even in this case. */
                uint16_t thisMin = timeInfo_.minute
                                   + 60*(timeInfo_.hour + 24*(timeInfo_.day-1));
                if(thisMin == lastMin_+1)
                {
                    if(timeInfo_.validity < 255)
                        ++ timeInfo_.validity;
                }
                else
                    timeInfo_.validity = 1;

                lastMin_ = thisMin;

            } /* End if(3rd parity bit and thus the complete telegram is okay?) */

        } /* End if/else if(Which bit to evaluate?) */

        if(cntPause_ > MIN_PAUSE_AT_START_OF_MIN_IN_TICS)
        {
            /* If the bit counter indicates that the complete telegram has been received,
               then we can notify the time to the client of this task. */
            if(cntBit_ == 58)
            {
                dcf_onNewTimeTelegram(&timeInfo_);
#if 0
                printf( "New time is: %02u:%02u %u.%u.%u (%u), validity: %u\n"
                      , timeInfo_.hour, timeInfo_.minute
                      , timeInfo_.day, timeInfo_.month, timeInfo_.year, timeInfo_.dayOfWeek
                      , timeInfo_.validity
                      );
#endif
            }
#if 0
            else
                printf("Re-starting\n");
#endif

            /* Normal restart of decoding or re-synchronization after any kind of error. */
            cntBit_ = 0xff;
        }
        else
        {
            /* Check duration of complete bit/pause combination. */
            cntImpulse_ += cntPause_;
            if(cntImpulse_ < MIN_LEN_BIT_CYCLE_IN_TICS
               ||  cntImpulse_ > MAX_LEN_BIT_CYCLE_IN_TICS
              )
            {
#if 0
                printf("Re-starting, bad pause duration %u\n", cntImpulse_);
#endif
                cntBit_ = 0xff;
            }
        }

        /* Measure next impulse/pause pair. */
        cntImpulse_ = 0;
        cntPause_ = 0;
    }

    s_lastValue_ = s;

} /* End of sampleAndDecodeDcfSignal */




/**
 * This function implements a regular task, which reads the demodulated DCF signal. It
 * samples the signal and does the decoding and validation of the telegram.\n
 *   This function is the entry point of the task, used for the basic configuration of the
 * RTuinOS application in void dcf_initializeTask(uint8_t, uint8_t). It implements the
 * basic task behavior, which is a simple regular task and branches periodically into the
 * true worker function of the task.
 *   @param initialResumeCondition
 * The vector of events, which made this task initially due.
 */

static void taskDcf(uint16_t initialResumeCondition)
{
    do
    {
        sampleAndDecodeDcfSignal();
    }
    while(rtos_suspendTaskTillTime(/* deltaTimeTillResume */ TASK_TIME_IN_TICS));

} /* End of taskDcf */




/**
 * Initialize the autonomous DCF task in the RTuinOS kernel. The function encapsulates the
 * call of rtos_initializeTask, thereby hiding most of its parameters and therefore
 * reducing the complexity for the user.
 *   @param idxTask
 * The RTuinOS task index has to be passed. This index is almost meaningless but is needed
 * to distinguish the tasks when calling the RTuinOS diagnostic functions.
 *   @param prioClass
 * The priority class the DCF task should belong to. Basically the task has no high demands
 * on priority. The signal processing is quite slow and the timing constraints when
 * measuring the bit impulses are rather weak. The task will probably succeed even with low
 * priority. Nonetheless, the priority can't be hidden: The user needs to take it into
 * consideration ...\n
 *   ... when designing the notification callback, which delivers the new time information
 * to the client of the task,\n
 *   ... when doing the static configuration of RTuinOS in rtos.config.h. Then the sizes of
 * the priority classes need to be known.
 *   @remark
 * This function need to be called from function void setup(void).
 *   @remark
 * In compilation configuration DEBUG the values of the receiver's timing constants can be
 * printed. This is very helpful to double check if the (rounded) values are useful for
 * proper signal decoding.\n
 *   The values are written to the Arduino console using printf. The use of the printf
 * function is uncritical here with respect to multi-threading; we are still in the RTuinOS
 * initialization phase. However, the application need to have made printf visisble: The
 * Serial object needs to be initialized and the standard output needs to be re-directed
 * into Serial, see init_stdout(void) for more.
 */

void dcf_initializeTask(uint8_t idxTask, uint8_t prioClass)
{
#ifdef DEBUG
    printf( "Task time: %u us = %u tics\n"
          , (uint16_t)(1e6 * ACTUAL_TASK_TIME_IN_S + 0.5)
          , TASK_TIME_IN_TICS
          );
    printf( "A bit cycle needs to be in the range [%u, %u] tics = 1 s + [%i, %i] us\n"
          , MIN_LEN_BIT_CYCLE_IN_TICS, MAX_LEN_BIT_CYCLE_IN_TICS
          , (int16_t)(1e6*((float)MIN_LEN_BIT_CYCLE_IN_TICS*ACTUAL_TASK_TIME_IN_S - 1.0) + 0.5)
          , (int16_t)(1e6*((float)MAX_LEN_BIT_CYCLE_IN_TICS*ACTUAL_TASK_TIME_IN_S - 1.0) + 0.5)
          );
    printf( "A time telegram ends with a pause of at least %u tics.\n"
            "Pauses longer than %u * 0.1 ms could be recognized as end of telegram\n"
            "Pauses longer than %u * 0.1 ms are surely recognized as end of telegram\n"
          , MIN_PAUSE_AT_START_OF_MIN_IN_TICS
          , (uint16_t)(1e4*(float)(MIN_PAUSE_AT_START_OF_MIN_IN_TICS-1)
                       *ACTUAL_TASK_TIME_IN_S
                       + 0.5
                      )
          , (uint16_t)(1e4*(float)MIN_PAUSE_AT_START_OF_MIN_IN_TICS
                       *ACTUAL_TASK_TIME_IN_S
                       + 0.5
                      )
          );
    printf( "A null bit needs to be in the range [%u, %u] tics.\n"
            "Impulses in the range (%u, %u) * 0.1 ms could be recognized as null bit\n"
            "Impulses in the range (%u, %u) * 0.1 ms are surely recognized as null bit\n"
          , MIN_LEN_BIT_0_IN_TICS, MAX_LEN_BIT_0_IN_TICS
          , (uint16_t)(1e4*(float)(MIN_LEN_BIT_0_IN_TICS-1)*ACTUAL_TASK_TIME_IN_S + 0.5)
          , (uint16_t)(1e4*(float)(MAX_LEN_BIT_0_IN_TICS+1)*ACTUAL_TASK_TIME_IN_S + 0.5)
          , (uint16_t)(1e4*(float)MIN_LEN_BIT_0_IN_TICS*ACTUAL_TASK_TIME_IN_S + 0.5)
          , (uint16_t)(1e4*(float)MAX_LEN_BIT_0_IN_TICS*ACTUAL_TASK_TIME_IN_S + 0.5)
          );
    printf( "A one bit needs to be in the range [%u, %u] tics.\n"
            "Impulses in the range (%u, %u) * 0.1 ms could be recognized as one bit\n"
            "Impulses in the range (%u, %u) * 0.1 ms are surely recognized as one bit\n"
          , MIN_LEN_BIT_1_IN_TICS, MAX_LEN_BIT_1_IN_TICS
          , (uint16_t)(1e4*(float)(MIN_LEN_BIT_1_IN_TICS-1)*ACTUAL_TASK_TIME_IN_S + 0.5)
          , (uint16_t)(1e4*(float)(MAX_LEN_BIT_1_IN_TICS+1)*ACTUAL_TASK_TIME_IN_S + 0.5)
          , (uint16_t)(1e4*(float)MIN_LEN_BIT_1_IN_TICS*ACTUAL_TASK_TIME_IN_S + 0.5)
          , (uint16_t)(1e4*(float)MAX_LEN_BIT_1_IN_TICS*ACTUAL_TASK_TIME_IN_S + 0.5)
          );
#endif

    rtos_initializeTask( idxTask
                       , /* taskFunction     */ taskDcf
                       , prioClass
                       , /* pStackArea       */ _stackTSampleAndDecodeDcfSignal
                       , /* stackSize        */ sizeof(_stackTSampleAndDecodeDcfSignal)
                       , /* startEventMask   */ RTOS_EVT_DELAY_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout     */ 50
                       );

} /* End of dcf_initializeTask */

/**
 * @file clk_clock.cpp
 *   Implementation of a real time clock for a sample task.
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
 *   dcf_readDcfInput
 *   dcf_onNewTimeTelegram
 *   clk_taskRTC
 * Local functions
 */

/*
 * Include files
 */

#include <Arduino.h>

#include "rtos.h"
#include "rtos_assert.h"
#include "aev_applEvents.h"
#include "dpy_display.h"
#include "dcf_taskDcf77.h"
#include "clk_clock.h"


/*
 * Defines
 */

/** The standard RTuinOS clock tic on the Arduino Mega 2560 board is 1/(16MHz/64/510) =
    51/25000s. We add 51 in each tic and have the next second when we reach 25000. This
    permits a precise implementation of the real time clock even with 16 Bit integer
    arithmetics. See #CLOCK_TIC_DENOMINATOR also. */
#define CLOCK_TIC_NUMERATOR     51

/** Denominator of ratio, which implements the clock's task rate. See #CLOCK_TIC_NUMERATOR
    for details. */
#define CLOCK_TIC_DENOMINATOR   (25000-(CLOCK_TIC_DENOMINATOR_TRIM_TERM))

/** Trim term for clock: By long term observation a correction has been figured out, which
    makes the clock significantly more accurate. The term is defined as (negative) addend
    to the denominator #CLOCK_TIC_DENOMINATOR. It is device dependent. Starting point on a
    new hardware device should be 0. Positive values advance the clock. */
#define CLOCK_TIC_DENOMINATOR_TRIM_TERM 58

/*
 * Local type definitions
 */


/*
 * Local prototypes
 */


/*
 * Data definitions
 */

/** Counter of seconds. The value is written without access synchronization code. The time
    information clk_noHour, clk_noMin, clk_noSec can be safely and consistently read only by
    a task of same or lower priority and using a critical section. */
volatile uint8_t clk_noSec = 0;

/** Counter of minutes. The value is written without access synchronization code. The time
    information clk_noHour, clk_noMin, clk_noSec can be safely and consistently read only by
    a task of same or lower priority and using a critical section. */
volatile uint8_t clk_noMin = 0;

/** Counter of hours. The value is written without access synchronization code. The time
    information clk_noHour, clk_noMin, clk_noSec can be safely and consistently read only by
    a task of same or lower priority and using a critical section.*/
volatile uint8_t clk_noHour = 20;

/** Input to the module: Recognized button-down events, which are used to adjust the clock
    ahead. The value is read/modified using a critical section. */
volatile uint8_t clk_noButtonEvtsUp = 0;

/** Input to the module: Recognized button-down events, which are used to adjust the clock
    towards lower time designations. The value is read/modified using a critical section. */
volatile uint8_t clk_noButtonEvtsDown = 0;

/** This flag is used to notify the real time clock task that the DCF task has sent new
    time information. It's set by the DCF callback and reset when the information has been
    evaluated. */
static boolean _dcfGotNewTimeInfo = false;

/** The time telegram  last recently got from the DCF task via its callback. The data is
    valid and can be accessed without the danger of race conditions for a minute after a
    rising edge of \a _dcfGotNewTimeInfo. */
static dcf_timeInfo _dcfTimeInfo;

/* Accumulator for task tics which generates a precise one second clock. */
static uint16_t _noTaskTics = 0;


/*
 * Function implementation
 */


/**
 * Get the next sample of the DCF input signal. To make the DCF task autonomous it doesn't
 * implement any I/O itself. The read function is implemented by the client of the task.
 * The client will take care of all relevant multi-tasking considerations.\n
 *   The function is executed in the context of the DCF task. It is called once every task
 * tic of of the regular DCF task. This task tic is configured at compile time in the DCF
 * module.
 *   @return dcfSignalValue
 * The current dcf signal value is returned as a Boolean. The polarity is specified by
 * means of a define, see #DCF_DIGITAL_LEVEL_DCF_IMPULSE.
 */

boolean dcf_readDcfInput(void)
{
    // @todo Move this code into a dedicated multi-threading-aware digital I/O module

    /* It's assumed that the DCF receiver is connected to a digital port bit. The Arduino
       port number is assigned to the define.
         The DCF receiver may have any polarity, i.e. the sent 100 ms or 200 ms time bits
       may be represented by either logical HIGH or LOW, when reading the port. The DCF
       module has a define DCF_DIGITAL_LEVEL_DCF_IMPULSE, which tells the module, whether
       a logical true or false is seen at impulse time. This define needs to be set in
       accordance with the actual implementation of the interface function
       dcf_readDcfInput(void), that propagtes the digital port reading to the to the DCF
       module. */
#define DIGITAL_PORT_USED_FOR_DCF (24)

    /* In this simple test we directly use the Arduino standard library to read a digital
       input. In general, this libary must not be used from an RTuinOS task as it is not
       thread-safe. A thread-safe implementation would anticipate that different tasks may
       access (different) digital ports at a time. The eight bits of a port demand
       read/modify/write operations to access a single bit and such operations demand
       mutual exclusion of tasks in a multi-tasking environment. In our simple test we
       don't have any other access to a digital port bit and so it doesn't matter here. */
    return (boolean)digitalRead(DIGITAL_PORT_USED_FOR_DCF);

#undef DIGITAL_PORT_USED_FOR_DCF
} /* End of dcf_readDcfInput */



/**
 * Synchronize real time clock with DCF time information.\n
 *   This is the callback function, which reports successful reception of new time
 * information to the client of the DCF task. It is called in the instance of having
 * another time telegram successfully decoded. (Bad telegrams are not reported to the
 * client at all.)\n
 *   This callback is executed in the context of the DCF task. Data synchronization with
 * the the event evaluating task needs to be implemented in the callback. To safely do so
 * the priorities of the DCF and its client task need to be taken into consideration, see
 * void dcf_initializeTask(uint8_t, uint8_t).
 *   @param pTimeInfo
 * The pointer to the object holding the up-to-date time information. This information is
 * accurate at the instance of callback execution. The object the parameter points to is
 * accessible during callback execution only. Data needs to be copied while the callback
 * executes.
 */

void dcf_onNewTimeTelegram(const dcf_timeInfo * const pTimeInfo)
{
    /* Task synchronization between the DCF task (of higher priority) and the real time
       clock task (of lower priority) is done by a Booelan flag and mutal exclusion. The
       mutal exclusion is done without further technical means but simply by time
       constraints: The callback is called no more frequently than once a minute and the
       data evaluation is done immediately after the callback - thus surely terminated
       before the next callback can appear. The evaluation is triggered by a Boolean flag,
       which is set in the callback as its very last action. */
    _dcfTimeInfo = *pTimeInfo;
    ASSERT(!_dcfGotNewTimeInfo);
    _dcfGotNewTimeInfo = true;

} /* End of dcf_onNewTimeTelegram */




/**
 * The regular task function of the real time clock. Has to be called every 100th tic of
 * the RTuinOS system time, which is running in its standard configuration of about 2 ms a
 * tic.
 */
void clk_taskRTC()
{
    /* Global interface of module: Do we have to adjust the time because of a user
       interaction? This code is kept very simple: Any button down event will advance
       or retard the clock by five minutes.
         The interface is written by the user interaction task, which has a higher
       priority. We need to apply a critical section for consistent read and safe
       modification. */
    cli();
    int8_t deltaTime = clk_noButtonEvtsUp - clk_noButtonEvtsDown;
    clk_noButtonEvtsUp   =
    clk_noButtonEvtsDown = 0;
    sei();

    boolean doDisplay;
    
    /* The interface with the DCF task: Did we get new time information so that we have to
       adjust our real time clock?
         The interface data is protected by mutual exclusion of both tasks. This is
       achieved by time constraints, see void dcf_onNewTimeTelegram(const dcf_timeInfo *
       const). */
    
    /* The very first clock adjustment is done as soon as possible, without high
       demands about the validity. */
    static uint8_t minDcfValidity_ = 1;

    /* A flag helps to recognize days without any DCF notification. It is used to control
       the validity demand. */
    static boolean noDcfReception_ = true;
    
    if(_dcfGotNewTimeInfo)
    {
        if(_dcfTimeInfo.validity >= minDcfValidity_)
        {
            /* Set the clock to the DCF time. */
            _noTaskTics = 0;
            clk_noSec   = 0;
            clk_noMin   = _dcfTimeInfo.minute;
            clk_noHour  = _dcfTimeInfo.hour;

            /* Once we have adjusted the clock it's running at high accuracy and we can
               effort to accept future DCF time telegrams only if they have a significantly
               higher validity. */
            minDcfValidity_ = 5;
            
            /* Indicate successful clock adjustment to the control logic of minDcfValidity_. */
            noDcfReception_ = false;
        }

        /* Reset the trigger flag to enable the notification of future telegrams. */
        _dcfGotNewTimeInfo = false;

        /* Enforce display of adjusted time. */
        doDisplay = true;
    }
    else if(deltaTime != 0)
    {
        /* Reset the second and fraction of a second counters. */
        clk_noSec   = 0;
        _noTaskTics = 0;
        
        /* Open DCF reception for soon synchronization, do no longer insist on high
           valididy. */ 
        minDcfValidity_ = 1;
        
        while(deltaTime > 0)
        {
            -- deltaTime;
            if((clk_noMin+=5) > 59)
            {
                clk_noMin -= 60;
                if(++clk_noHour > 23)
                    clk_noHour = 0;
            }
        }
        while(deltaTime < 0)
        {
            /* By defining the downwards operation not strictly inverse to upwards we can
               reach all times, not just the multiples of five. Maybe not the kind of thing
               one would expect and no true alternative to a state machine, which begins to
               auto-repeat the key event after a while, but this is just a simple
               demonstration of RTuinOS, not a high-end application. */
            ++ deltaTime;
            if((clk_noMin-=4) > 59)
            {
                clk_noMin += 60;
                if(--clk_noHour > 23)
                    clk_noHour = 23;
            }
        }

        /* Enforce display of adjusted time. */
        doDisplay = true;
    }
    else /* normal clock operation */
    {
        /* Advance the clock by one tic. */
        _noTaskTics += CLK_TASK_TIME_RTUINOS_STANDARD_TICS*CLOCK_TIC_NUMERATOR;

        /* Carry ripple. */
        if(_noTaskTics >= CLOCK_TIC_DENOMINATOR)
        {
            _noTaskTics -= CLOCK_TIC_DENOMINATOR;

            /* We display hh:mm:ss, so a change of the seconds leads to a write into the
               display. */
            doDisplay = true;

            if(++clk_noSec > 59)
            {
                clk_noSec = 0;
                if(++clk_noMin > 59)
                {
                    clk_noMin = 0;
                    if(++clk_noHour > 23)
                    {
                        clk_noHour = 0;

                        /* A day has gone by. If now DCF telegram should have been received for
                           more than one day, we reduce the validity level at which we accept a
                           DCF telegram. Once a day is fast enough for the validity control as
                           the real time clock is running even without DCF synchronization very
                           stable and accurate. */
                        if(noDcfReception_)
                            if(minDcfValidity_ > 1)
                                -- minDcfValidity_;
                        noDcfReception_ = true;
                    }
                }
            }
        }
        else
        {
            /* Don't write into the display in every tic! */
            doDisplay = false;

        } /* End if(A second has elapsed?) */

    } /* if/else if(Did we get new DCF time information? A button has been touched to
         adjust the clock?) */

    /* Update display because visible information has changed. */
    if(doDisplay)
        dpy_display.printTime(clk_noHour, clk_noMin, clk_noSec);

} /* End of clk_taskRTC */
/**
 * @file setup.c
 *   The RTuinOS application is initialized. The DCF task starts running and each decoded
 * time telegram is reported via Serial at 115200 bps. (The none standard, high rate is
 * needed to not affect the task timing too much.)\n
 *   Additional information about the status of DCF reception and decoding can be enabled
 * by compile time switches in the DCF module.\n
 *   This file also serves as a code sample how to integrate the DCF task into an RTuinOS
 * application, how to initialize the task, how to bind the DCF module to the actual DCF
 * signal input, how to receive the results.
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
 *   setup
 *   loop
 * Local functions
 */

/*
 * Include files
 */

#include <Arduino.h>
#include "rtos.h"
#include "stdout.h"
#include "dcf_taskDcf77.h"
//#include "setup.h"


/*
 * Defines
 */

/** The test assumes that the DCF receiver is connected to a digital port bit. The Arduino
    port number is assigned to the define.
      @remark The DCF receiver may have any polarity, i.e. the sent 100 ms or 200 ms time
    bits may be represented by either logical HIGH or LOW, when reading the port. The DCF
    module has a define #DCF_DIGITAL_LEVEL_DCF_IMPULSE, which tells the module, whether a
    logical true or false is seen at impulse time. This define needs to be set in
    accordance with the actual implementation of the interface function boolean
    dcf_readDcfInput(void), that propagtes the digital port reading to the to the DCF
    module. */
#define DIGITAL_PORT_USED_FOR_DCF (24)
 
 
/*
 * Local type definitions
 */
 
 
/*
 * Local prototypes
 */
 
 
/*
 * Data definitions
 */
 
 
/*
 * Function implementation
 */


/**
 * Get the next sample of the DCF input signal. To make the DCF task autonomous it doesn't
 * implement any I/O itself. The read function is implemented by the client of the task.
 * The client will take care of all relevant multi-tasking considerations.\n
 *   The function is executed in the context of the DCF task. It is called once every task
 * tic of of the regular DCF task. This task tic is configured in the DCF module.
 *   @return dcfSignalValue
 * The current dcf signal value is return as a Boolean. The polarity is specified by means
 * of a define, see #DCF_DIGITAL_LEVEL_DCF_IMPULSE.
 */

boolean dcf_readDcfInput(void)
{
    /* In this simple test we directly use the Arduino standard library to read a digital
       input. In general, thislibary must not be used from an RTuinOS task as it is not
       thread-safe. A thread-safe implementation would anticipate that different tasks may
       access (different) digital ports at a time. The eight bits of a port demand a
       read/modify/write operations to access a single bit and such operations demand
       mutual exclusion of tasks in a multi-tasking environment. In our simple test we
       don't have any other access to a digital port bit and so it doesn't matters here. */ 
    return (boolean)digitalRead(DIGITAL_PORT_USED_FOR_DCF);
    
} /* End of dcf_readDcfInput */



/**
 * The callback function, which reports successful reception of new time information to the
 * client of the DCF task. It is called in the instance of having another time telegram
 * successfully decoded. (Bad telegrams are not reported to the client at all.)\n
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
    printf( "New time is: %02u:%02u %u.%u.%u (%u), validity: %u\n"
          , pTimeInfo->hour, pTimeInfo->minute
          , pTimeInfo->day, pTimeInfo->month, pTimeInfo->year, pTimeInfo->dayOfWeek
          , pTimeInfo->validity
          );
          
} /* End of dcf_onNewTimeTelegram */



/**
 * Initialization of system, particularly specification of tasks and their properties.
 */

void setup()
{
    /* Start serial communication for some feedback about the progress and redirect stdout
       into Serial.
         The baud rate is chosen high, because in this test environment the DCF task may
       easily be operated with some printf based status output. This (slow) output
       endangers the accuracy of the task timing and makes the measured and reported timing
       values less meaningful. */
    Serial.begin(115200);
    init_stdout();

    /* Write greeting to the console. */
    puts_progmem(rtos_rtuinosStartupMsg);

    /* Initialize the RTuinOS task for DCF decoding. */
    dcf_initializeTask(/* idxTask */ 0, /* prioClass */ 0);
    
} /* End of setup */




/**
 * The idle task loop function. Is cyclically invoked by the RTuinOS kernel if no other
 * task is due.
 */

void loop()
{
} /* End of loop */






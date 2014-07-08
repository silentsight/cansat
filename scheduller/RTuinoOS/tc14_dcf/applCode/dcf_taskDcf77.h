#ifndef DCF_TASKDCF77_INCLUDED
#define DCF_TASKDCF77_INCLUDED
/**
 * @file tc14_dcf/applCode/dcf_taskDcf77.h
 * Definition of global interface of module dcf_taskDcf77.c
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

/*
 * Include files
 */

#include <Arduino.h>


/*
 * Defines
 */

/** The stack usage of the DCF task. A value of about 100 Byte plus the need of your
    notification callback dcf_onNewTimeTelegram(const dcf_timeInfo * const) is
    appropriate.
      @remark Be careful if you enable any of the printf statements for debugging purpose
    in the dcf module: They increase the need for stack memory by some tens of Byte.
      @todo Start with a generous value and use function uint16_t
    rtos_getStackReserve(uint8_t) in the running application to find out. */
#define DCF_TASK_STACK_SIZE 160

/** The Boolean signal value, which is read in the instance of a time impulse. Whether it
    is a true or a false may depend on the receiver hardware and the interfacing circuitry
    and the implementation of the access function. The access function
    dcf_readDcfInput(void) will return #DCF_DIGITAL_LEVEL_DCF_IMPULSE if and only if one of
    the 100 or 200 ms impulses is active and seen at the input. */
#define DCF_DIGITAL_LEVEL_DCF_IMPULSE   false


/*
 * Global type definitions
 */

/** The passed time information after decoding. */
typedef struct
{
    uint8_t minute    /// Minute, 0..59
          , hour      /// Hour, 0..23
          , day       /// Day of month 1..31
          , dayOfWeek /// Day of week, 1..7, 1 is Monday, 2 is Tuesday and so on
          , month;    /// Month, 1..12
          
    uint8_t validity; /// The number of successfully received subsequent time telegrams,
                      /// with consistent, consecutive time contents. Starts with 1 at
                      /// initial reception. A client could protect himself against
                      /// unrecognized reception failures by accepting time information
                      /// only if the validity is >= 4

    uint16_t year;    /// Year, 2000..2099
    
} dcf_timeInfo;


/*
 * Global data declarations
 */



/*
 * Global prototypes
 */

/**
 * Get the next sample of the DCF input signal. To make the DCF task autonomous it doesn't
 * implement any I/O itself. The read function is implemented by the client of the task.
 * The client will take care of all relevant multi-tasking considerations.\n
 *   The function is executed in the context of the DCF task. It is called once every
 * #ACTUAL_TASK_TIME_IN_S seconds, which is the period time of the regular DCF task.
 *   @return dcfSignalValue
 * The current DCF signal value is returned as a Boolean. The polarity is specified by
 * means of a define, see #DCF_DIGITAL_LEVEL_DCF_IMPULSE.
 *   @remark
 * It's not an option but a must that this function is defined. Otherwise the linker will
 * report an unknown symbol.
 */
extern boolean dcf_readDcfInput(void);

 
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
 *   @remark
 * It's not an option but a must that a callback function is defined. Otherwise the linker
 * will report an unknown symbol.
 */
extern void dcf_onNewTimeTelegram(const dcf_timeInfo * const pTimeInfo);

/** Initialize the DCF task at RTuinOS startup time. */
void dcf_initializeTask(uint8_t idxTask, uint8_t prioClass);

#endif  /* DCF_TASKDCF77_INCLUDED */

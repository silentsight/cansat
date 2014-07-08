/**
 * @file tc14_dcf/tc14_adcInput.cpp
 *   Extended test case 14 of RTuinOS. The meaning and specific issues of this test case
 * are documented in the original test case, please refer to
 * RTuinOS/code/applications/tc14/tc14_adcInput.cpp in your RTuinOS installation
 * directory.\n
 *   The modifications made in this application demonstrate the integration of the
 * re-usable, self-contained DCF task for RTuinOS into an existing application. tc14
 * already contained a real time clock, implemented in tc14_dcf/applCode/clk_clock.cpp.
 * That file now has an additional function; it implements the notification callback of the
 * DCF task. The notification is used to write the time information from a decoded DCF time
 * telegram into the (existing) real time clock. The time synchronization code demonstrates
 * how the validity information received from the DCF task can be evaluated in order to
 * refuse accidentally misunderstood telegrams.\n
 *   This file adds the DCF task to the set of concurrent RTuinOS tasks and the
 * configuration file rtos.config.h is of course updated in order to take account of the
 * new task.\n
 *   Finally, the source files of the DCF task (dcf_taskDcf77.c/.h) have been added to the
 * source folder applCode). This way, they are included into the build.\n
 *   Please use a diff tool to find all differences between the original RTuinOS test case
 * tc14 and this sample application.\n
 *   Successful DCF time information reception requires hardware, which is not available on
 * the Arduino board. You need to connect an external DCF77 receiver to the board. The
 * receiver's output signal is a digital pulse of low frequency and two-valued duty cycle.
 * The most natural port to connect the receiver to is a digital input. The sample code
 * supports the selection of a digital input and the polarity of the signal by compile time
 * switches. It's basically possible to use an analog input as well; now you'd have to
 * modify the interface function, which is not part of the DCF task but only declared
 * (extern) by it. Please refer to tc14_dcf/applCode/clk_clock.cpp, boolean
 * dcf_readDcfInput(void).
 * @remark
 *   The compilation of this sample requires linkage against the stdio library with
 * floating point support for printf & co. The selection of this library is done in the
 * makefile "callback" into tc14_dcf.mk.
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
 *
 * Module interface
 *   rtos_enableIRQUser00
 *   setup
 *   loop
 * Local functions
 *   blink
 *   taskOnADCComplete
 *   taskRTC
 *   taskIdleFollower
 *   taskButton
 *   taskDisplayVoltage
 */

/*
 * Include files
 */

#include <Arduino.h>

#include "rtos.h"
#include "rtos_assert.h"
#include "gsl_systemLoad.h"
#include "stdout.h"
#include "aev_applEvents.h"
#include "dpy_display.h"
#include "but_button.h"
#include "clk_clock.h"
#include "dcf_taskDcf77.h"
#include "adc_analogInput.h"


/*
 * Defines
 */

/** Pin 13 has an LED connected on most Arduino boards. */
#define LED 13

/** The index to the task objects as needed for requesting the overrun counter or the stack
    usage. */
enum { idxTaskOnADCComplete
     , idxTaskRTC
     , idxTaskIdleFollower
     , idxTaskButton
     , idxTaskDisplayVoltage
     , idxTaskDcf
     , noTasks
     };


/*
 * Local type definitions
 */


/*
 * Local prototypes
 */


/*
 * Data definitions
 */

static volatile uint16_t _adcResult = 0;
static volatile uint32_t _noAdcResults = 0;
static uint8_t _stackTaskOnADCComplete[256];
static uint8_t _stackTaskRTC[256];
static uint8_t _stackTaskIdleFollower[256];
static uint8_t _stackTaskButton[256];
static uint8_t _stackTaskDisplayVoltage[256];

/* Results of the idle task. */
volatile uint8_t _cpuLoad = 200;


/*
 * Function implementation
 */

/**
 * Trivial routine that flashes the LED a number of times to give simple feedback. The
 * routine is blocking.
 *   @param noFlashes
 * The number of times the LED is lit.
 */
static void blink(uint8_t noFlashes)
{
#define TI_FLASH 150

    while(noFlashes-- > 0)
    {
        digitalWrite(LED, HIGH);  /* Turn the LED on. (HIGH is the voltage level.) */
        delay(TI_FLASH);          /* The flash time. */
        digitalWrite(LED, LOW);   /* Turn the LED off by making the voltage LOW. */
        delay(TI_FLASH);          /* Time between flashes. */
    }
    delay(1000-TI_FLASH);         /* Wait for a second after the last flash - this command
                                     could easily be invoked immediately again and the
                                     bursts need to be separated. */
#undef TI_FLASH
}



/**
 * The ADC is already configured when this callback is invoked from the RTuinOS kernel
 * initialization code. The callback is just used to release the interrupt on ADC
 * conversion complete - only now the kernel is ready to accept and handle these
 * interrupts.
 */

void rtos_enableIRQUser00()
{
    /* Complete the ADC configuration: Enable interrupt. */

    /* The already regularly running conversions trigger interrupts after this register
       read/modify/write operation.
         Remark: Writing ADIF to one means to reset a probably already pending interrupt.
       We must not handle this interrupt as we are not yet in sync with the running
       conversions. */
    ADCSRA = ADCSRA
             | (1 << ADIF)  /* Reset the "conversion-ready" flag by writing a one. */
             | (1 << ADIE)  /* Allow interrupts on conversion-ready. */
             ;
} /* End of rtos_enableIRQUser00 */



/**
 * This task is triggered one by one by the interrupts triggered by the ADC, when it
 * completes a conversion. The task reads the ADC result register and processes the
 * sequence of values. The processing result is input to a slower, reporting task.
 *   @param initialResumeCondition
 * The vector of events which made the task due the very first time.
 */

static void taskOnADCComplete(uint16_t initialResumeCondition)
{
    ASSERT(initialResumeCondition == EVT_ADC_CONVERSION_COMPLETE);

#ifdef DEBUG
    /* Test: Our ADC interrupt should be synchronous with Arduino's TIMER0_OVF (see
       wiring.c). */
    extern volatile unsigned long timer0_overflow_count;
    uint32_t deltaCnt = timer0_overflow_count;
#endif

    do
    {
        /* Test: Our ADC interrupt should be synchronous with Arduino's TIMER0_OVF. */
        ASSERT(adc_noAdcResults + deltaCnt == timer0_overflow_count);

        /* Call the actual interrupt handler code. */
        adc_onConversionComplete();
    }
    while(rtos_waitForEvent( EVT_ADC_CONVERSION_COMPLETE | RTOS_EVT_DELAY_TIMER
                           , /* all */ false
                           , /* timeout */ 1
                           )
#ifdef DEBUG
          == EVT_ADC_CONVERSION_COMPLETE
#endif
         );

    /* The following assertion fires if the ADC interrupt isn't timely. The wait condition
       specifies a sharp timeout. True production code would be designed more failure
       tolerant and e.g. not specify a timeout at all. This code would cause a reset in
       case. */
    ASSERT(false);

} /* End of taskOnADCComplete */




/**
 * A regular task of about 250 ms task time, which implements a real time clock.
 *   @param initialResumeCondition
 * The vector of events which made the task due the very first time.
 */

static void taskRTC(uint16_t initialResumeCondition)
{
    ASSERT(initialResumeCondition == RTOS_EVT_ABSOLUTE_TIMER);

    /* Regularly call the RTC implementation at its expected rate: The RTC module exports the
       expected task time by a define. */
    do
    {
        clk_taskRTC();
    }
    while(rtos_suspendTaskTillTime
                    (/* deltaTimeTillResume */ CLK_TASK_TIME_RTUINOS_STANDARD_TICS)
         );
    ASSERT(false);

} /* End of taskRTC */




/**
 * A task, which is triggered by the idle loop each time it has new results to display. The
 * idle task itself must not acquire any mutexes and consequently, it can't ever own the
 * display. This task however can.
 *   @param initialResumeCondition
 * The vector of events which made the task due the very first time.
 */

static void taskIdleFollower(uint16_t initialResumeCondition)
{
    ASSERT(initialResumeCondition == EVT_TRIGGER_IDLE_FOLLOWER_TASK);
    do
    {
        dpy_display.printCpuLoad(_cpuLoad);
    }
    while(rtos_waitForEvent(EVT_TRIGGER_IDLE_FOLLOWER_TASK, /* all */ false, 0));
    ASSERT(false);

} /* End of taskIdleFollower */





/**
 * A task, which is triggered by the processing of the ADC conversion results: Whenever it
 * has a new voltage measurement of the analog button input this task is triggered to do
 * the further evaluation, i.e. identification of the pressed button, debouncing, state
 * machine and dispatching to the clients.
 *   @param initialResumeCondition
 * The vector of events which made the task due the very first time.
 */

static void taskButton(uint16_t initialResumeCondition)
{
    ASSERT(initialResumeCondition == EVT_TRIGGER_TASK_BUTTON);
    do
    {
        but_onNewButtonVoltage();
    }
    while(rtos_waitForEvent(EVT_TRIGGER_TASK_BUTTON, /* all */ false, 0));
    ASSERT(false);

} /* End of taskButton */





/**
 * A task, which is triggered by the processing of the ADC conversion results: Whenever it
 * has a new input voltage measurement this task is triggered to display the result.
 *   @param initialResumeCondition
 * The vector of events which made the task due the very first time.
 */

static void taskDisplayVoltage(uint16_t initialResumeCondition)
{
    ASSERT(initialResumeCondition == EVT_TRIGGER_TASK_DISPLAY_VOLTAGE);
    
    /* The rate of the result values is about once every 133 ms, which makes the display
       quite nervous. And it would become even faster is the averaging constant
       ADC_NO_AVERAGED_SAMPLES would be lowered. Therefore we average here again to get are
       better readable, more stable display.
         The disadvantage: The state machine in module adc synchronizes switching the ADC
       input with the series of averaged samples. This is impossible here, which means that
       - in the instance of switching to another ADC input - the averaging series formed
       here typically consist of some samples from the former input and some from the new
       input. We do no longer see a sharp switch but a kind of cross fading. */
#define NO_AVERAGED_SAMPLES     5
#define SCALING_BIN_TO_V(binVal)                                                            \
        ((ADC_U_REF/(double)((uint32_t)NO_AVERAGED_SAMPLES*ADC_NO_AVERAGED_SAMPLES)/1024.0) \
         *(double)(binVal)                                                                  \
        )

    static uint32_t accumuatedAdcResult_ = 0;
    static uint8_t noMean_ = NO_AVERAGED_SAMPLES;
    do
    {
        /* This low priority task needs to apply a critical section to read the result of
           the ADC interrupt task of high priority. */
        cli();
        accumuatedAdcResult_ += adc_inputVoltage;
        sei();
        
        if(--noMean_ == 0)
        {
            dpy_display.printVoltage(SCALING_BIN_TO_V(accumuatedAdcResult_));
            
            /* Start next series on averaged samples. */
            noMean_ = NO_AVERAGED_SAMPLES;
            accumuatedAdcResult_ = 0;
        }
    }        
    while(rtos_waitForEvent(EVT_TRIGGER_TASK_DISPLAY_VOLTAGE, /* all */ false, 0));
    ASSERT(false);
    
#undef NO_AVERAGED_SAMPLES
#undef SCALING_BIN_TO_V
} /* End of taskDisplayVoltage */





/**
 * The initalization of the RTOS tasks and general board initialization.
 */

void setup()
{
#ifdef DEBUG
    /* Start serial port at 9600 bps. */
    Serial.begin(9600);

    /* Redirect stdout into Serial. */
    init_stdout();

    /* Print greeting to the console window. */
    puts_progmem(rtos_rtuinosStartupMsg);
#endif

    /* Print greeting on the LCD. */
    dpy_display.printGreeting();

    /* Initialize the digital pin as an output. The LED is used for most basic feedback about
       operability of code. */
    pinMode(LED, OUTPUT);

#ifdef DEBUG
//    printf( "ADC configuration at startup:\n"
//            "  ADMUX  = 0x%02x\n"
//            "  ADCSRB = 0x%02x\n"
//          , ADMUX
//          , ADCSRB
//          );
#endif

    /* Write the invariant parts of the display once. This needs to be done here, before
       multitasking begins. */
    dpy_display.printBackground();

    /* Configure the interrupt task of highest priority class. */
    ASSERT(noTasks == RTOS_NO_TASKS);
    rtos_initializeTask( /* idxTask */          idxTaskOnADCComplete
                       , /* taskFunction */     taskOnADCComplete
                       , /* prioClass */        RTOS_NO_PRIO_CLASSES-1
                       , /* pStackArea */       &_stackTaskOnADCComplete[0]
                       , /* stackSize */        sizeof(_stackTaskOnADCComplete)
                       , /* startEventMask */   EVT_ADC_CONVERSION_COMPLETE
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );

    /* Configure the real time clock task of lowest priority class. */
    rtos_initializeTask( /* idxTask */          idxTaskRTC
                       , /* taskFunction */     taskRTC
                       , /* prioClass */        0
                       , /* pStackArea */       &_stackTaskRTC[0]
                       , /* stackSize */        sizeof(_stackTaskRTC)
                       , /* startEventMask */   RTOS_EVT_ABSOLUTE_TIMER
                       , /* startByAllEvents */ false
                       , /* startTimeout */     CLK_TASK_TIME_RTUINOS_STANDARD_TICS
                       );

    /* Configure the idle follower task of lowest priority class. */
    rtos_initializeTask( /* idxTask */          idxTaskIdleFollower
                       , /* taskFunction */     taskIdleFollower
                       , /* prioClass */        0
                       , /* pStackArea */       &_stackTaskIdleFollower[0]
                       , /* stackSize */        sizeof(_stackTaskIdleFollower)
                       , /* startEventMask */   EVT_TRIGGER_IDLE_FOLLOWER_TASK
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );

    /* Configure the button evaluation task. Its priority is below the interrupt but - as
       it implements user interaction - above the priority of the display tasks. */
    rtos_initializeTask( /* idxTask */          idxTaskButton
                       , /* taskFunction */     taskButton
                       , /* prioClass */        1
                       , /* pStackArea */       &_stackTaskButton[0]
                       , /* stackSize */        sizeof(_stackTaskButton)
                       , /* startEventMask */   EVT_TRIGGER_TASK_BUTTON
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );

    /* Configure the result display task. */
    rtos_initializeTask( /* idxTask */          idxTaskDisplayVoltage
                       , /* taskFunction */     taskDisplayVoltage
                       , /* prioClass */        0
                       , /* pStackArea */       &_stackTaskDisplayVoltage[0]
                       , /* stackSize */        sizeof(_stackTaskDisplayVoltage)
                       , /* startEventMask */   EVT_TRIGGER_TASK_DISPLAY_VOLTAGE
                       , /* startByAllEvents */ false
                       , /* startTimeout */     0
                       );
    
    /* Configure the DCF task. Its priority is higher than that of the real time clock and
       the display tasks. DCF reception benefits from task time accuracy. If the priority
       is high enough for proper timing then the task cycle time (which is the DCF signal
       sample time at the same time) can be chosen relatively low and this saves CPU
       consumption. */
    dcf_initializeTask( /* idxTask */ idxTaskDcf
                      , /* prioClass */ 1
                      );
    
    /* Initialize other modules. */
    adc_initAfterPowerUp();
    
} /* End of setup */




/**
 * The application owned part of the idle task. This routine is repeatedly called whenever
 * there's some execution time left. It's interrupted by any other task when it becomes
 * due.
 *   @remark
 * Different to all other tasks, the idle task routine may and should return. (The task as
 * such doesn't terminate). This has been designed in accordance with the meaning of the
 * original Arduino loop function.
 */

void loop()
{
    /* Give an alive sign. */
    blink(3);

#ifdef DEBUG
    printf("\nRTuinOS is idle\n");
#endif

    /* Share result of CPU load computation with the displaying idle follower task. No
       acces synchronization is needed here for two reasons: Writing a uint8 is atomic and
       we have a strict coupling in time between the idle task and the data reading task:
       They become active one after another. */
    _cpuLoad = gsl_getSystemLoad();

#ifdef DEBUG
    cli();
    uint16_t adcResult       = adc_inputVoltage;
    uint16_t adcResultButton = adc_buttonVoltage;
    uint32_t noAdcResults = adc_noAdcResults;
    uint8_t hour = clk_noHour
          , min  = clk_noMin
          , sec  = clk_noSec;
    sei();

    printf("At %02u:%02u:%02u:\n", hour, min, sec);
    printf( "ADC result %7lu at %7.2f s: %.4f V (input), %.4f V (buttons)\n"
          , noAdcResults
          , 1e-3*millis()
          , ADC_SCALING_BIN_TO_V(adcResult)
          , ADC_SCALING_BIN_TO_V(adcResultButton)
          );
    printf("CPU load: %.1f %%\n", (double)_cpuLoad/2.0);
    ASSERT(rtos_getTaskOverrunCounter(/* idxTask */ idxTaskRTC, /* doReset */ false) == 0);
    
    uint8_t u;
    for(u=0; u<RTOS_NO_TASKS; ++u)
        printf("Unused stack area of task %u: %u Byte\n", u, rtos_getStackReserve(u));
#endif

    /* Trigger the follower task, which is capable to safely display the results. */
    rtos_sendEvent(EVT_TRIGGER_IDLE_FOLLOWER_TASK);

} /* End of loop */





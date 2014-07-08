/************************************************************************************************************
 * arduOS - a library for scheduling multiple tasks on the AVR                                              *
 * Copyright 2014 Lars Hollenbach (lohlive@googlemail.com)                                                  *
 ************************************************************************************************************

 ************************************************************************************************************
 * This library is free software; you can redistribute it and/or                                            *
 * modify it under the terms of the GNU Lesser General Public                                               *
 * License as published by the Free Software Foundation; either                                             *
 * version 2.1 of the License, or (at your option) any later version.                                       *
 *                                                                                                          *
 * This library is distributed in the hope that it will be useful,                                          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                                           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                                        *
 * Lesser General Public License for more details.                                                          *
 *                                                                                                          *
 * You should have received a copy of the GNU Lesser General Public                                         *
 * License along with this library; If not, see <http://www.gnu.org/licenses/>.                             *
 ***********************************************************************************************************/

#ifndef ARDUOS_H
#define ARDUOS_H
#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>
#include "../arduos_new/arduos_new.h"

//typedef byte byte;
typedef unsigned int uint;
typedef unsigned long int ulong;

#define SYS_getStack() \
  asm volatile\
  (\
    "LDS R26, stack_addr     \n\t"\
    "LDS R27, stack_addr + 1 \n\t"\
    "IN  R0, __SP_L__        \n\t"\
    "ST  X+, R0              \n\t"\
    "IN  R0, __SP_H__        \n\t"\
    "ST  X+, R0              \n\t"\
  );

#define SYS_setStack() \
  asm volatile\
  (\
    "LDS R26, stack_addr     \n\t"\
    "LDS R27, stack_addr + 1 \n\t"\
    "LD  R0, X+              \n\t"\
    "OUT __SP_L__, R0        \n\t"\
    "LD  R0, X+              \n\t"\
    "OUT __SP_H__, R0        \n\t"\
  );

#define SYS_storeContext() \
  asm volatile\
  (\
  "PUSH R0            \n\t"\
  "IN   R0, __SREG__  \n\t"\
  "CLI                \n\t"\
  "PUSH R0            \n\t"\
  "PUSH R1            \n\t"\
  "CLR R1             \n\t"\
  "PUSH R2            \n\t"\
  "PUSH R3            \n\t"\
  "PUSH R4            \n\t"\
  "PUSH R5            \n\t"\
  "PUSH R6            \n\t"\
  "PUSH R7            \n\t"\
  "PUSH R8            \n\t"\
  "PUSH R9            \n\t"\
  "PUSH R10           \n\t"\
  "PUSH R11           \n\t"\
  "PUSH R12           \n\t"\
  "PUSH R13           \n\t"\
  "PUSH R14           \n\t"\
  "PUSH R15           \n\t"\
  "PUSH R16           \n\t"\
  "PUSH R17           \n\t"\
  "PUSH R18           \n\t"\
  "PUSH R19           \n\t"\
  "PUSH R20           \n\t"\
  "PUSH R21           \n\t"\
  "PUSH R22           \n\t"\
  "PUSH R23           \n\t"\
  "PUSH R24           \n\t"\
  "PUSH R25           \n\t"\
  "PUSH R26           \n\t"\
  "PUSH R27           \n\t"\
  "PUSH R28           \n\t"\
  "PUSH R29           \n\t"\
  "PUSH R30           \n\t"\
  "PUSH R31           \n\t"\
  );

#define SYS_loadContext() \
  asm volatile\
  (\
  "POP R31            \n\t"\
  "POP R30            \n\t"\
  "POP R29            \n\t"\
  "POP R28            \n\t"\
  "POP R27            \n\t"\
  "POP R26            \n\t"\
  "POP R25            \n\t"\
  "POP R24            \n\t"\
  "POP R23            \n\t"\
  "POP R22            \n\t"\
  "POP R21            \n\t"\
  "POP R20            \n\t"\
  "POP R19            \n\t"\
  "POP R18            \n\t"\
  "POP R17            \n\t"\
  "POP R16            \n\t"\
  "POP R15            \n\t"\
  "POP R14            \n\t"\
  "POP R13            \n\t"\
  "POP R12            \n\t"\
  "POP R11            \n\t"\
  "POP R10            \n\t"\
  "POP R9             \n\t"\
  "POP R8             \n\t"\
  "POP R7             \n\t"\
  "POP R6             \n\t"\
  "POP R5             \n\t"\
  "POP R4             \n\t"\
  "POP R3             \n\t"\
  "POP R2             \n\t"\
  "POP R1             \n\t"\
  "POP R0             \n\t"\
  "OUT __SREG__, R0   \n\t"\
  "POP R0             \n\t"\
  );

#define SYS_storeFunc(X) \
  asm volatile\
  (\
    "PUSH R30   \n\t"\
    "PUSH R31   \n\t"\
    ::"z" (X)\
  );

#define SYS_SEI() \
  asm volatile\
      (\
        "SEI           \n\t"\
      );

namespace SYS
{
  class Task_base
  {
  public:
    Task_base(uint stacksize, void(main)());
    ~Task_base();
    byte *stack;
    void (*const iMain)();
    void exec();
    virtual void exit();
    bool called;
  protected:
    byte *mem;
  };

  class Scheduler_base
  {
  public:
    Scheduler_base();
    virtual ~Scheduler_base();
    virtual Task_base *get() = 0;
    virtual bool remove(Task_base*) = 0;
  };

  class Lock
  {
  public:
    Lock();
    ~Lock();
    static void lock();
    static void unlock();
  protected:
    bool prev;
  };

  class TimedTask_base;
  class TimedScheduler_base : public Scheduler_base
  {
  public:
    TimedScheduler_base();
    virtual ~TimedScheduler_base();
    virtual bool add(TimedTask_base *, bool trylater = true) = 0;
    virtual void tick() = 0;
  };

  void yield() __attribute__ ((naked)); // switch the tasks manually

  extern Scheduler_base *master;
  extern bool switch_blocked;
  extern Task_base *currentTask;
  extern TimedScheduler_base *timedmaster;
  extern byte *originalStack;
}

extern byte **stack_addr;

#endif // ARDUOS_H

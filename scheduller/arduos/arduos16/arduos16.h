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

#ifndef ARDUOS328_H
#define ARDUOS328_H

#include "../arduos_core/arduos_core.h"

namespace SYS
{
  extern byte ledc;
  extern bool first;

  void start();
}

#define SYS_yield_end() \
{\
  {\
    SYS::first = false;\
    if(SYS::switch_blocked == false)\
      SYS::currentTask = SYS::master == 0 ? 0 : SYS::master->get();\
    if(SYS::currentTask == (0x0000))\
    {\
      stack_addr = &SYS::originalStack;\
    }\
    else\
    {\
      if(SYS::currentTask->called == false)\
      {\
        SYS::first = true;\
        SYS::currentTask->called = true;\
      }\
      stack_addr = &(SYS::currentTask->stack);\
    }\
  }\
  if(SYS::first)\
  {\
    SYS_setStack();\
    SYS_SEI();\
    SYS::currentTask->exec();\
  }\
  else\
  {\
    SYS_setStack();\
    SYS_loadContext();\
    asm volatile("RETI");\
  }\
}

#define SYS_setup() \
stack_addr = &SYS::originalStack;

#define SYS_enable_preemptive \
ISR(TIMER0_COMPA_vect) __attribute__ ((signal, naked));\
\
ISR(TIMER0_COMPA_vect)  \
{\
  SYS_storeContext();\
  SYS_getStack();\
  {\
    if(SYS::timedmaster != 0)\
    {\
      SYS::timedmaster->tick();\
    }\
    SYS::ledc++;\
    if((SYS::ledc & B01111111) >= 100)\
    {\
      SYS::ledc ^= B10000000;\
      digitalWrite(13, (SYS::ledc & B10000000) != 0 ? HIGH : LOW);\
      SYS::ledc &= B10000000;\
    }\
  }\
  SYS_yield_end();\
};

#define SYS_enable_cooperative \
ISR(TIMER0_COMPA_vect) __attribute__ ((signal, naked));\
\
ISR(TIMER0_COMPA_vect)  \
{\
  SYS_storeContext();\
  SYS_getStack();\
  {\
    if(SYS::timedmaster != 0)\
    {\
      SYS::timedmaster->tick();\
    }\
    SYS::ledc++;\
    if((SYS::ledc & B01111111) >= 100)\
    {\
      SYS::ledc ^= B10000000;\
      digitalWrite(13, (SYS::ledc & B10000000) != 0 ? HIGH : LOW);\
      SYS::ledc &= B10000000;\
    }\
  }\
  SYS_setStack();\
  SYS_loadContext(); \
  asm volatile("RETI"); \
};

#endif // ARDUOS328_H

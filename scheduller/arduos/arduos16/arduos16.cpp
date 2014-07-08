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

#include "arduos16.h"

namespace SYS
{
  byte ledc = 0x00;
  bool first = false;
  byte *originalStack = 0x00;
}

void SYS::yield()
{
  SYS_storeContext();
  SYS_getStack();
  SYS_yield_end();
}

void SYS::start()
{
  pinMode(13, OUTPUT);
  stack_addr = &SYS::originalStack;
  TCCR0A |= (1 << WGM01);
  OCR0A = 0xF9;
  TIMSK0 |= (1 << OCIE0A);
  sei();
  TCCR0B |= (1 << CS01) | (1 << CS00);
}

/*
  {
    SYS::currentTask = SYS::master == 0 ? 0 : SYS::master->get();
    if(SYS::currentTask == (0x0000))
    {
      stack_addr = &SYS::originalStack;
    }
    else
    {
      if(SYS::currentTask->called == false)
      {
        SYS::first = true;
        SYS::currentFunc = SYS::currentTask->iMain;
        SYS::currentTask->called = true;
      }
      stack_addr = &(SYS::currentTask->stack);
    }
  }
  if(SYS::first)
  {
    SYS::first = false;
    SYS_setStack();
    SYS_invoke(SYS::currentFunc)
  }
  else
  {\
    SYS_setStack();\
    SYS_loadContext();\
    asm volatile("RETI");\
  }\
  */

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

#include "combinedscheduler.h"

SYS::CombinedScheduler::CombinedScheduler(byte tarrs, bool text, byte rarrs, bool rext) :
  timed(tarrs, text), robin(rarrs, rext)
{
  if(SYS::master == 0 ||
     SYS::master == &timed ||
     SYS::master == &robin)
  {
    SYS::master = this;
  }
}

SYS::CombinedScheduler::~CombinedScheduler()
{
  if(SYS::master == this)
    SYS::master = 0;
}

bool SYS::CombinedScheduler::remove(Task_base *task)
{
  bool out;
  if(timed.remove(task))
    out = true;
  if(robin.remove(task))
    out = true;
  return out;
}

bool SYS::CombinedScheduler::add(TimedTask_base *t, bool l)
{
  return timed.add(t, l);
}

bool SYS::CombinedScheduler::add(Task_base *t)
{
  return robin.add(t);
}

SYS::Task_base *SYS::CombinedScheduler::get()
{
  Task_base *out;
  out = timed.get();
  if(out != 0)
  {
    robin.add(out);
    return out;
  }
  else
  {
    return robin.get();
  }
}

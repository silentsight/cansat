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

#include"arduos_core.h"

byte **stack_addr = 0;

namespace SYS
{
  Scheduler_base *master = 0;
  bool switch_blocked = false;
  Task_base *currentTask;
  TimedScheduler_base *timedmaster = 0;
}

SYS::Task_base::Task_base(uint stacksize, void(main)()) :
  iMain(main), called(false)
{
  mem = new byte[stacksize];
  stack = &mem[stacksize - 1];
}

SYS::Task_base::~Task_base()
{
  delete[] mem;
}

void SYS::Task_base::exec()
{
  (*iMain)();
  {
    SYS::Lock lock;
    exit();
  }
  SYS::yield();
}

void SYS::Task_base::exit()
{
  SYS::master->remove(this);
}


SYS::Scheduler_base::Scheduler_base()
{
  if(SYS::master == 0)
  {
    SYS::master = this;
  }
}

SYS::Scheduler_base::~Scheduler_base()
{
  if(SYS::master == this)
  {
    SYS::master = 0;
  }
}


SYS::Lock::Lock() :
  prev(SYS::switch_blocked)
{
  SYS::switch_blocked = true;
}

SYS::Lock::~Lock()
{
  SYS::switch_blocked = prev;
}

SYS::TimedScheduler_base::TimedScheduler_base() :
  SYS::Scheduler_base()
{
  if(SYS::timedmaster == 0)
    SYS::timedmaster = this;
}

SYS::TimedScheduler_base::~TimedScheduler_base()
{
  if(SYS::timedmaster == this)
    SYS::timedmaster = 0;
}

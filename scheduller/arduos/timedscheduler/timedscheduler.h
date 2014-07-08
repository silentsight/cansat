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

#ifndef TIMEDSCHEDULER_H
#define TIMEDSCHEDULER_H

#include "../timedtask/timedtask.h"

namespace SYS
{
  class TimedScheduler : public SYS::TimedScheduler_base
  {
  public:
    TimedScheduler(uint arrsize, bool extending = true);
    virtual ~TimedScheduler();
    virtual bool remove(Task_base *);
    virtual bool add(TimedTask_base *, bool trylater);
    virtual void tick();
    virtual Task_base *get();
    byte occupied();
  protected:
    byte size;
    bool ext;
    TimedTask_base **arr;
  };
}

#endif // TIMEDSCHEDULER_H

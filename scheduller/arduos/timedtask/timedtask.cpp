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

#include "timedtask.h"

SYS::TimedTask::TimedTask(uint ss, void (m)(), ulong ticks, bool &success, bool trylater) :
  SYS::TimedTask_base(ss, m, ticks)
{
  success = SYS::timedmaster->add(this, trylater);
}

SYS::TimedTask::TimedTask(uint ss, void (m)(), ulong ticks) :
  SYS::TimedTask_base(ss, m, ticks)
{
  SYS::timedmaster->add(this, true);
}

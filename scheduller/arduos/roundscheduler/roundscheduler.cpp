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

#include "roundscheduler.h"

namespace SYS
{
  RoundScheduler *roundmaster = 0;
}

SYS::RoundScheduler::RoundScheduler(byte arrsize, bool extending) :
  SYS::Scheduler_base(), size(arrsize), ext(extending), pos(255), occsize(0)
{
  if(SYS::roundmaster == 0)
    SYS::roundmaster = this;
  arr = new Task_base*[size];
}

SYS::RoundScheduler::~RoundScheduler()
{
  if(SYS::roundmaster == this)
    SYS::roundmaster = 0;
  delete[] arr;
}

SYS::Task_base *SYS::RoundScheduler::get()
{
  if(occsize == 0)
    return 0;
  pos++;
  if(pos >= occsize)
    pos = 0;
  return arr[pos];
}

bool SYS::RoundScheduler::remove(Task_base *rm)
{
  SYS::Lock lock;
  bool out = false;
  for(byte i = 0; i < occsize; i++)
  {
    if(arr[i] == rm)
    {
      for(byte j = i + 1; j < occsize; j++)
        arr[j - 1] = arr[j];
      occsize--;
      out = true;
    }
  }
  return out;
}

bool SYS::RoundScheduler::add(Task_base *add)
{
  SYS::Lock lock;
  bool out = true;

  if(occsize == size)
  {//no space left
    if(ext == true)
    {//extend array by 1
      SYS::Task_base **oldarr = arr;
      arr = new SYS::Task_base*[size + 1];
      for(byte i = 0; i < size; i++)
      {
        arr[i] = oldarr[i];
      }
      oldarr[size] = 0;
      size++;
    }
    else
    {//no space left
      out = false;
    }
  }
  if(out == true)
  {
    arr[occsize] = add;
    occsize++;
  }
  return out;
}

byte SYS::RoundScheduler::free()
{
  return size - occsize;
}

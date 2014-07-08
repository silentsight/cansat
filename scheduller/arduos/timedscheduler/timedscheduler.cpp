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

#include "timedscheduler.h"

SYS::TimedScheduler::TimedScheduler(uint arrsize, bool extending) :
  SYS::TimedScheduler_base() ,size(arrsize), ext(extending)
{
  arr = new TimedTask_base*[size];
  for(byte i = 0; i < size; i++)
  {
    arr[i] = 0;
  }
}

SYS::TimedScheduler::~TimedScheduler()
{
  delete[] arr;
}

void SYS::TimedScheduler::tick()
{
  for(byte i = 0; i < size; i++)
  {
    if(arr[i] != 0)
    {
      if(arr[i]->delay != 0)
        arr[i]->delay--;
    }
  }
}

byte SYS::TimedScheduler::occupied()
{
  SYS::Lock lock;
  byte out = 0;
  for(byte i = 0; i < size; i++)
  {
    if(arr[i] != 0)
    {
      out++;
    }
  }
  return out;
}

bool SYS::TimedScheduler::add(TimedTask_base *newTask, bool trylater)
{
  SYS::Lock lock;

  bool out = true;

  if(occupied() == size)
  {//no space left
    if(ext == true)
    {//extend array by 1
      SYS::TimedTask_base **oldarr = arr;
      arr = new SYS::TimedTask_base*[size + 1];
      for(byte i = 0; i < size; i++)
      {
        arr[i] = oldarr[i];
      }
      arr[size] = 0;
      size++;
    }
    else
    {//no space left
      out = false;
    }
  }
  if(out == true)
  {
    for(byte i = 0; i < size; i++)
    {
      if(arr[i] != 0)
      {
        if(arr[i]->delay == newTask->delay)
        { //Tasks with the same delay are not allowed
          if(trylater == false)
            out = false;
          else
          {
            newTask->delay++;
            i = 0;
          }
        }
      }
    }
  }
  if(out == true)
  {
    for(byte i = 0; i < size; i++)
    {//search for free places
      if(arr[i] == 0)
      {// place is free
        arr[i] = newTask;
        break;
      }
    }
  }
  return out;
}

bool SYS::TimedScheduler::remove(Task_base *rmt)
{
  SYS::Lock lock;
  bool out = false;
  for(byte i = 0; i < size; i++)
  {
    if(arr[i] == rmt)
    {
      arr[i] = 0;
      out = true;
    }
  }
  return out;
}

SYS::Task_base *SYS::TimedScheduler::get()
{
  SYS::Task_base *out = 0;
  for(byte i = 0; i < size; i++)
  {
    if(arr[i] != 0)
    {
      if(arr[i]->delay == 0)
      {
        out = arr[i];
        return out;
      }
    }
  }
  return out;
}

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

#include "arduos_new.h"

void * operator new(size_t size)
{
  return malloc(size);
}

void * operator new[](size_t size)
{
  return malloc(size);
}

void operator delete(void * ptr)
{
  free(ptr);
}

void operator delete[](void * ptr)
{
  free(ptr);
}

int __cxa_guard_acquire(__guard *g)
{
  return !*(char *)(g);
}
void __cxa_guard_release (__guard *g)
{
  *(char *)g = 1;
}
void __cxa_guard_abort (__guard *)
{

}
void __cxa_pure_virtual(void)
{

}

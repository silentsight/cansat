/* Header to define new/delete operators as they aren't provided by avr-gcc by default
   Taken from http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=59453 
   and modified by Lars Ole Hollenbach
 */

#ifndef NEW_H
#define NEW_H

#include "../arduos_core/arduos_core.h"

  void * operator new(size_t size);
  void * operator new[](size_t size);
  void operator delete(void * ptr);
  void operator delete[](void * ptr);

__extension__ typedef int __guard __attribute__((mode (__DI__)));

extern "C" int __cxa_guard_acquire(__guard *);
extern "C" void __cxa_guard_release (__guard *);
extern "C" void __cxa_guard_abort (__guard *); 
extern "C" void __cxa_pure_virtual(void);

void *malloc(size_t len);
void free(void *p);
void *realloc(void *ptr, size_t len);

#endif

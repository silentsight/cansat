#include<arduos_core.h>
#include<arduos16.h>

#include<arduos_new.h>
#include<tmptask.h>
#include<satask.h>
#include<roundscheduler.h>

void lup()
{
  for(byte i = 0; i < 256; i++)
  {
    delay(1000);
    Serial.write('|');
    digitalWrite(11, LOW);
  }
}

void setup()
{
  SYS_setup();
  pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);
  Serial.begin(9600);
  new SYS::RoundScheduler(4, true);
  new SYS::SATask(128, lup);
  new SYS::SATask(128, loop);
  Serial.println("Hello!");
  delay(1000);

  SYS::start();
  for(;;);
}

void loop()
{
  for(;;)
  {
    delay(1000);
    Serial.write('-');
  }
}

SYS_enable_preemptive
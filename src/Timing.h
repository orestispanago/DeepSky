#ifndef timing_h
#define timing_h

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Connection.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void waitForNextMinute()
{
  while (!statusOK())
    ;
  timeClient.begin();
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }
  while (timeClient.getSeconds() != 0)
    ;
}

#endif



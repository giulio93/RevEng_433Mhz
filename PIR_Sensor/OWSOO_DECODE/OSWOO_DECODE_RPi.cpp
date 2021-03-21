/* Convert RF signal into bits (temperature sensor version) 
 * Written by : Ray Wang (Rayshobby LLC)
 * http://rayshobby.net/?p=8827
 * Update: adapted to RPi using WiringPi 
 */
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define RING_BUFFER_SIZE 256
#define SYNC_LENGTH 13125
#define SEP_LENGTH 450
#define BIT1_LENGTH 1395
#define BIT0_LENGTH 1395
#define DATA_PIN 2 // D2 is interrupt GPI027 Pi.rev2
unsigned long timings[RING_BUFFER_SIZE];
unsigned int syncIndex1 = 0;
unsigned int syncIndex2 = 0;
bool received = false;

bool isSync(unsigned int idx)
{
  unsigned long t0 = timings[(idx + RING_BUFFER_SIZE - 1) % RING_BUFFER_SIZE];
  unsigned long t1 = timings[idx];
  if (t0 > (SEP_LENGTH - 100) && t0 < (SEP_LENGTH + 100) && t1 > (SYNC_LENGTH - 1000) && t1 < (SYNC_LENGTH + 1000) && digitalRead(DATA_PIN) == HIGH)
  {
    printf("SYNC");

    return true;
  }
  return false;
}

void handler()
{
  static unsigned long duration = 0;
  static unsigned long lastTime = 0;
  static unsigned int ringIndex = 0;
  static unsigned int syncCount = 0;
  if (received == true)
  {
    return;
  }
  long time = micros();
  duration = time - lastTime;
  lastTime = time;
  ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;
  timings[ringIndex] = duration;
  if (isSync(ringIndex))
  {
    syncCount++;
    if (syncCount == 1)
    {
      syncIndex1 = (ringIndex + 1) % RING_BUFFER_SIZE;
    }
    else if (syncCount == 2)
    {
      syncCount = 0;
      syncIndex2 = (ringIndex + 1) % RING_BUFFER_SIZE;
      unsigned int changeCount = (syncIndex2 < syncIndex1) ? (syncIndex2 + RING_BUFFER_SIZE - syncIndex1) : (syncIndex2 - syncIndex1);
      if (changeCount != 66)
      {
        received = false;
        syncIndex1 = 0;
        syncIndex2 = 0;
      }
      else
      {
        received = true;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  if (wiringPiSetup() == -1)
  {
    printf("no wiring pi detected\n");
    return 0;
  }

  wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);

  while (true)
  {
    if (received == true)
    {
      for (unsigned int i = syncIndex1; i != syncIndex2; i = (i + 2) % RING_BUFFER_SIZE)
      {
        unsigned long t0 = timings[i], t1 = timings[(i + 1) % RING_BUFFER_SIZE];
        if (t0 > (SEP_LENGTH - 200) && t0 < (SEP_LENGTH + 200))
        {
          if (t1 > (BIT1_LENGTH - 1000) && t1 < (BIT1_LENGTH + 1000))
          {
            printf("1");
          }
          else if (t1 > (BIT0_LENGTH - 1000) && t1 < (BIT0_LENGTH + 1000))
          {
            printf("0");
          }
          else
          {
            printf("SYNC");
          }
        }
        else
        {
          //printf("?%d?", int(t0));
        }

        if (t0 > (SEP_LENGTH - 100) && t0 < (SEP_LENGTH + 100))
        {
          //Bit 1 stay up around 1.3ms
          if (t1 > (BIT1_LENGTH - 100) && t1 < (BIT1_LENGTH + 100))
          {
            printf("1");
          }
          else
          {
            printf("SYNC");
          }
        }
        //Bit 0 stay down around 1.3ms
        else if (t0 > (BIT0_LENGTH - 100) && t0 < (BIT0_LENGTH + 100))
        {
          if (t1 > (SEP_LENGTH - 100) && t1 < (SEP_LENGTH + 100))
          {
            printf("0");
          }
        }
      }
      printf("\n");
      received = false;
      syncIndex1 = 0;
      syncIndex2 = 0;
    }
  }
  exit(0);
}
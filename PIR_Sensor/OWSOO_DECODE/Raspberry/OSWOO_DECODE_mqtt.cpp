/* Convert RF signal into bits (temperature sensor version) 
 * Written by : Ray Wang (Rayshobby LLC)
 * http://rayshobby.net/?p=8827
 * Update: adapted to RPi using WiringPi 
 * Modified and adapted to the PIR sensor 433mhz signal by Giulio Pilotto
 * The singal here has a seprator + long off for 1 and long on + separator for 0.
 * So BIT1_LENGTH  and BIT0_LENGTH are the same, what change is the separator disposition.
 * Example: -___-___-___---_---_-___-___-___---_-___ ==> 1110011101
 */
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <mosquitto.h>

 
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

int send_message_mqtt()
{
    int rc;
    struct mosquitto *mosq;
    mosquitto_lib_init();

    mosq = mosquitto_new("OSWOO", true, NULL);
    rc = mosquitto_connect(mosq, "localhost", 1883, 60);

    mosquitto_publish(mosq, NULL, "PIR", 10, "detected", 0, false);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
    return 0;
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
      //printf("recived");
      //system("gpio edge 2 none");
      //wiringPiISR(1, INT_EDGE_BOTH, &handler);
      fprintf(stdout,"recived");
      fprintf(stdout,"\n");
      send_message_mqtt();
      //delay(100);
      //wiringPiISR(DATA_PIN, INT_EDGE_BOTH, &handler);
      received = false;
      syncIndex1 = 0;
      syncIndex2 = 0;
    }
  }
  exit(0);
}
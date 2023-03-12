/* Convert RF signal into bits (temperature sensor version)
 * Written by : Ray Wang (Rayshobby LLC)
 * Edit by: Giulio Pilotto
 * http://rayshobby.net/?p=8827
 */

//All this parameters come from the Audacity signal analysis
//Ring buffer size has to be large enough to fit data between two successive sync signals
#define RING_BUFFER_SIZE 256

//The length of a SYNC signal
#define SYNC_LENGTH 3900
//The length of separation edge between 1 or 0 signal
#define SEP_LENGTH 450
//The length of the edge Bit 1
#define BIT1_LENGTH 1900
//The length of the egde Bit 0
#define BIT0_LENGTH 950
//Digital interrupt PIN
#define DATAPIN 3

//Initialize the buffer
unsigned long timings[RING_BUFFER_SIZE];
//Index of the first sync signal
unsigned int syncIndex1 = 0;
//Index of the second sync signal
unsigned int syncIndex2 = 0;

bool received = false;
bool sync1 = false;
bool sync2 = false;
//Change count, how many times the interrupt has been triggered
unsigned int cc;

void setup()
{
  Serial.begin(9600);
  Serial.println("Started.");
  //TODO check INPUT_PULLUP if it helps to get cleaner readings
  pinMode(DATAPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DATAPIN), handler, CHANGE);
}

//ISR function
void handler()
{
  static unsigned long duration = 0;
  static unsigned long lastTime = 0;
  static unsigned int ringIndex = 0;
  static unsigned int syncCount = 0;

  //This control avoid to overwrite the buffer if the signal is correctly received and is being written in the serial
  if (received == true)
  {
    return;
  }
  // calculating timing since last change
  long time = micros();
  duration = time - lastTime;
  lastTime = time;

  //Store edge lenght in a ring buffer, % RING_BUFFER_SIZE works as saw tooth function
  ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;
  timings[ringIndex] = duration;

  //Check if SYNC singnal is present in the ringIndex
  if (isSync(ringIndex))
  {
    syncCount++;

    // first time sync is seen, record buffer index
    if (syncCount == 1)
    {
      syncIndex1 = (ringIndex + 1) % RING_BUFFER_SIZE;
      sync1 = true;
    }
    else if (syncCount == 2)
    {
      // second time sync is seen, start bit conversion
      sync2 = true;
      syncCount = 0;
      sync1 = false;
      sync2 = false;
      syncIndex2 = (ringIndex + 1) % RING_BUFFER_SIZE;
      unsigned int changeCount = (syncIndex2 < syncIndex1) ? (syncIndex2 + RING_BUFFER_SIZE - syncIndex1) : (syncIndex2 - syncIndex1);
      cc = changeCount;
      // changeCount must be 74 -- 36  bits x 2 + 2 for sync
      if (changeCount != 74)
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

//Detect if a SYNC signal is present
bool isSync(unsigned int idx)
{
  unsigned long t0 = timings[(idx + RING_BUFFER_SIZE - 1) % RING_BUFFER_SIZE];
  unsigned long t1 = timings[idx];

  //Check if the first signal is a separator
  if (t0 > (SEP_LENGTH - 100) && t0 < (SEP_LENGTH + 100)){
    //The sync signal is roughtly 4.0ms. Here we check if it is between 3.0ms and 5.0ms
    if (t1 > (SYNC_LENGTH - 1000) && t1 < (SYNC_LENGTH + 1000) && digitalRead(DATAPIN) == HIGH)
    {
      return true;
    }
  }
  return false;
}
//This function is used to convert duration stored in the buffer to a bit string
int t2b(unsigned int t0, unsigned int t1)
{
  //Separation gap between two rising edge is 0.45ms
  if (t0 > (SEP_LENGTH - 100) && t0 < (SEP_LENGTH + 100))
  {
    if (t1 > (BIT1_LENGTH - 100) && t1 < (BIT1_LENGTH + 100))
    { //Bit 1 stay up around 1.9ms

      return 1;
    }
    else if (t1 > (BIT0_LENGTH - 100) && t1 < (BIT0_LENGTH + 100))
    { //Bit 0 stay up around 0.95ms
      return 0;
    }
    else
    {
      return -1;
    }
  }
  else
  {
    return -1;
  }
}

void loop()
{

  if (received == true)
  {
    //Disable interrupt to avoid new data corrupting the buffer
    detachInterrupt(1);

    //toSerial()

    unsigned int startIndex, stopIndex;
    unsigned long channel = 0;
    bool fail = false;


    decodingChannel(startIndex,stopIndex,channel,fail);

    decodingTemperature(startIndex,stopIndex,channel,fail);

    decodingHumidity(startIndex,stopIndex,channel,fail);



    // delay for 1 second to avoid repetitions
    delay(1000);
    received = false;
    syncIndex1 = 0;
    syncIndex2 = 0;

    // re-enable interrupt
    attachInterrupt(1, handler, CHANGE);
  }
}

void toSerial(){
    // loop over buffer data ==> This is no longer needed, since now we want to print out the converted measure.
       int c=0;
       for(unsigned int i=syncIndex1,c =0; i!=syncIndex2; i=(i+2)%RING_BUFFER_SIZE,c++) {
         unsigned long t0 = timings[i], t1 = timings[(i+1)%RING_BUFFER_SIZE];
         if (t0>(SEP_LENGTH-100) && t0<(SEP_LENGTH+100)) {
          if (t1>(BIT1_LENGTH-100) && t1<(BIT1_LENGTH+100)) {
            Serial.print("1");
          } else if (t1>(BIT0_LENGTH-100) && t1<(BIT0_LENGTH+100)) {
            Serial.print("0");
          } else {
            Serial.print("SYNC");  // sync signal
          }
          } else {
          Serial.print("?");  // undefined timing
          }
          if (c%8==7) Serial.print(" ");
       }
       Serial.println("");
}

void decodingChannel(unsigned int startIndex,unsigned int stopIndex,unsigned long channel,bool fail){
  //Decoding Channels
    //Channel is write in the very first part of the first and second byte
    //01110000 10100000 01110110 11110011 0010SYNC  11.8C 50%
    //01110000 1010<== Channel bits
    startIndex = (syncIndex1 + (1 * 8 + 0) * 2) % RING_BUFFER_SIZE;
    stopIndex = (syncIndex1 + (1 * 8 + 8) * 2) % RING_BUFFER_SIZE;
    for (int i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
    {
      int bit = t2b(timings[i], timings[(i + 1) % RING_BUFFER_SIZE]);
      channel = (channel << 1) + bit;
      if (bit < 0)
        fail = true;
    }

    if (fail)
    {
      Serial.println("Decoding error.");
    }
    else
    {
      Serial.print("Channel: ");
      Serial.println((int)(((channel) / 8) - 16) / 2);
    }

}

void decodingTemperature(unsigned int startIndex,unsigned int stopIndex,unsigned long channel,bool fail){
    //Decoding Temperature
    //Humidity is write in the middle byte
    //01110000 10100000 01110110 11110011 0010SYNC  11.8C 50%
    //             0000 01110110<== Temperature
    unsigned long temp = 0;
    fail = false;
    // most significant 4 bits
    startIndex = (syncIndex1 + (1 * 8 + 4) * 2) % RING_BUFFER_SIZE;
    stopIndex = (syncIndex1 + (2 * 8 + 8) * 2) % RING_BUFFER_SIZE;
    for (int i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
    {
      int bit = t2b(timings[i], timings[(i + 1) % RING_BUFFER_SIZE]);
      temp = (temp << 1) + bit;
      if (bit < 0)
        fail = true;
    }

    if (fail)
    {
      Serial.println("Decoding error.");
    }
    else
    {
      Serial.print("Temperature: ");
      Serial.println((int)((temp)));
    }
}

void decodingHumidity(unsigned int startIndex,unsigned int stopIndex,unsigned long channel,bool fail){
   //Decoding Humidity
    //Humidity is write in the last byte plus other four bit before a SYNC
    //01110000 10100000 01110110 11110011 0010SYNC  11.8C 50%
    //                           11110011 0010<== Humidity
    unsigned long humidity = 0;
    fail = false;
    startIndex = (syncIndex1 + (3 * 8 + 4) * 2) % RING_BUFFER_SIZE;
    stopIndex = (syncIndex1 + (3 * 8 + 12) * 2) % RING_BUFFER_SIZE;

    for (int i = startIndex; i != stopIndex; i = (i + 2) % RING_BUFFER_SIZE)
    {
      int bit = t2b(timings[i], timings[(i + 1) % RING_BUFFER_SIZE]);
      humidity = (humidity << 1) + bit;
      if (bit < 0)
        fail = true;
    }
    if (fail)
    {
      Serial.println("Decoding error.");
    }
    else
    {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println("\% / ");
    }
}

/* Convert RF signal into bits (temperature sensor version) 
 * Written by : Ray Wang (Rayshobby LLC)
 * http://rayshobby.net/?p=8827
 */

// ring buffer size has to be large enough to fit
// data between two successive sync signals
#define RING_BUFFER_SIZE  256

#define SYNC_LENGTH  3900
#define SEP_LENGTH   450
#define BIT1_LENGTH  1900
#define BIT0_LENGTH  950

#define DATAPIN  3  // D3 is interrupt 1

unsigned long timings[RING_BUFFER_SIZE];
unsigned int syncIndex1 = 0;  // index of the first sync signal
unsigned int syncIndex2 = 0;  // index of the second sync signal
bool received = false;

bool sync1 = false;
bool sync2 = false;
unsigned int cc;


// detect if a sync signal is present
bool isSync(unsigned int idx) {
  unsigned long t0 = timings[(idx+RING_BUFFER_SIZE-1) % RING_BUFFER_SIZE];
  unsigned long t1 = timings[idx];

  // on the temperature sensor, the sync signal
  // is roughtly 9.0ms. Accounting for error
  // it should be within 8.0ms and 10.0ms
  if (t0>(SEP_LENGTH-100) && t0<(SEP_LENGTH+100) &&
    t1>(SYNC_LENGTH-1000) && t1<(SYNC_LENGTH+1000) &&
    digitalRead(DATAPIN) == HIGH) {
    return true;
  }
  return false;
}

/* Interrupt 1 handler */
void handler() {
  static unsigned long duration = 0;
  static unsigned long lastTime = 0;
  static unsigned int ringIndex = 0;
  static unsigned int syncCount = 0;

  // ignore if we haven't processed the previous received signal
  if (received == true) {
    return;
  }
  // calculating timing since last change
  long time = micros();
  duration = time - lastTime;
  lastTime = time;

  // store data in ring buffer
  ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;
  timings[ringIndex] = duration;

  // detect sync signal
  if (isSync(ringIndex)) {
    syncCount ++;
   
      
    // first time sync is seen, record buffer index
    if (syncCount == 1) {
      syncIndex1 = (ringIndex+1) % RING_BUFFER_SIZE;
      sync1 = true;
    } 
    else if (syncCount == 2) {
      // second time sync is seen, start bit conversion
      sync2 = true;
      syncCount = 0;
      sync1 = false;
      sync2 = false;
      syncIndex2 = (ringIndex+1) % RING_BUFFER_SIZE;
      unsigned int changeCount = (syncIndex2 < syncIndex1) ? (syncIndex2+RING_BUFFER_SIZE - syncIndex1) : (syncIndex2 - syncIndex1);
      cc = changeCount;
      // changeCount must be 66 -- 32 bits x 2 + 2 for sync
      if (changeCount != 74 ) {
        received = false;
        syncIndex1 = 0;
        syncIndex2 = 0;
        
      } 
      else {
        received = true;
    
      }
    }

  }
}


void setup() {
  Serial.begin(9600);
  Serial.println("Started.");
  pinMode(3, INPUT);
  attachInterrupt(1, handler, CHANGE);

}

void loop() {
  
    int i;
    for(i=0; i<257 ; i++)
    {
      //Serial.print("i :");
      //Serial.println(i);
      // Serial.println(timings[i]);
    }
    
  if (received == true) {
    // disable interrupt to avoid new data corrupting the buffer
    detachInterrupt(1);
    // loop over buffer data
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

    // delay for 1 second to avoid repetitions
    delay(1000);
    received = false;
    syncIndex1 = 0;
    syncIndex2 = 0;

    // re-enable interrupt
    attachInterrupt(1, handler, CHANGE);
  }
}


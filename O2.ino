#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
MAX30105 particleSensor;

double avered = 0; double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100;
String state = "Reading Data.";

double ESpO2 = 100.00;
double FSpO2 = 0.7; 
double frate = 0.95; 
#define TIMETOBOOT 3000 
#define SCALE 88.0 
#define SAMPLING 5
#define FINGER_ON 30000 


const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;

#define USEFIFO
void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");
  
  while (!particleSensor.begin(Wire, I2C_SPEED_FAST)) 
  {
    Serial.println("Sensor not detected!!! ");
    state = "Sensor error!!!";
    //while (1);
  }
  state = "Reading Data.";
  byte ledBrightness = 0x7F; 
  byte sampleAverage = 4; 
  byte ledMode = 2; 
  int sampleRate = 800; 
  int pulseWidth = 411; 
  int adcRange = 16384; 
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 

  particleSensor.enableDIETEMPRDY();
}

void loop()
{
  
  uint32_t ir, red , green;
  double fred, fir;
  double SpO2 = 0; 
  
  
#ifdef USEFIFO
  particleSensor.check(); 

while (particleSensor.available()) {
#ifdef MAX30105
#else
    red = particleSensor.getFIFOIR(); 
    ir = particleSensor.getFIFORed(); 
#endif

    //routine for hearbeat rate
    long irValue = particleSensor.getIR();

      if (checkForBeat(irValue) == true)
   {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
      rates[rateSpot++] = (byte)beatsPerMinute; 
      rateSpot %= RATE_SIZE; 
      }
   }
    
    i++;
    fred = (double)red;
    fir = (double)ir;
    avered = avered * frate + (double)red * (1.0 - frate);
    aveir = aveir * frate + (double)ir * (1.0 - frate);
    sumredrms += (fred - avered) * (fred - avered);
    sumirrms += (fir - aveir) * (fir - aveir);
    if ((i % SAMPLING) == 0) {
      if ( millis() > TIMETOBOOT) {
        
        if (ir < FINGER_ON) 
          { Serial.print("Finger not detected!!"); 
          state = "Finger not detected!"; 
          Serial.println(); }
        else {
        state = "Reading Data.";
       
        Serial.print(" Oxygen % = ");
        Serial.print(ESpO2);
        //Serial.print(" Avg BPM= ");
        //Serial.print(beatAvg);
        Serial.print(" BPM=");
        Serial.print(beatsPerMinute);
        Serial.println();
        }
      }
    }
    if ((i % Num) == 0) {
      double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
      
      SpO2 = -23.3 * (R - 0.4) + 100; 
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;
      
      sumredrms = 0.0; sumirrms = 0.0; i = 0;
      break;
    }
    particleSensor.nextSample(); 
  }
}
#endif

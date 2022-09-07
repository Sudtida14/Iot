#include <TridentTD_LineNotify.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
MAX30105 particleSensor;
double avered = 0; double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100;//calculate SpO2 by this sampling interval
String state = "Reading Data.";

#define USEFIFO
#define FINGER_ON 30000
double ESpO2 = 100.00;//initial value of estimated SpO2
double FSpO2 = 0.7; //filter factor for estimated SpO2
double frate = 0.95; //low pass filter for IR/red LED value to eliminate AC component
#define TIMETOBOOT 3000 // wait for this time(msec) to output SpO2
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
float temperature;
#define SCALE 88.0 //adjust to display heart beat and SpO2 in the same scale
#define SAMPLING 5 
#define SSID        "Ak"  // ชื่อไวไฟ
#define PASSWORD    "12345678"  // รหัสไวไฟ
#define LINE_TOKEN  "o9Kj0vCJPYd49CUq0UT2CMgfDu48D1p8X9xky8Dbdwh"  // token line
 uint32_t ir, red , green;
 double fred, fir;
  double SpO2 = 0; //raw SpO2 before low pass filtered

void setup() {
  byte ledBrightness = 0x7F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
  int sampleRate = 800; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384
  Serial.begin(115200); 
  Serial.println();
  Serial.println(LINE.getVersion());

  WiFi.begin(SSID, PASSWORD);
  Serial.printf("WiFi connecting to %s\n",  SSID);
  while(WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(400); }
  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());  
 
  LINE.setToken(LINE_TOKEN);
  Serial.begin(115200);
  Serial.println("Initializing pulse oximeter..");
  while (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) //Use default I2C port, 400kHz speed
  {
    Serial.println("Sensor not detected!!! ");
    state = "Sensor error!!!";
    //while (1);
  }
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

  particleSensor.enableDIETEMPRDY();
   red = particleSensor.getFIFOIR(); //why getFOFOIR output Red data by MAX30102 on MH-ET LIVE breakout board
    ir = particleSensor.getFIFORed(); //why getFIFORed output IR data by MAX30102 on MH-ET LIVE breakout board
}
void loop(){  
  long irValue = particleSensor.getIR();

      if (checkForBeat(irValue) == true)
   {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
      {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable
      }
   }
    
    i++;
    fred = (double)red;
    fir = (double)ir;
    avered = avered * frate + (double)red * (1.0 - frate);//average red level by low pass filter
    aveir = aveir * frate + (double)ir * (1.0 - frate); //average IR level by low pass filter
    sumredrms += (fred - avered) * (fred - avered); //square sum of alternate component of red level
    sumirrms += (fir - aveir) * (fir - aveir);//square sum of alternate component of IR level
    if ((i % SAMPLING) == 0) {//slow down graph plotting speed for arduino Serial plotter by thin out
      
      if ( millis() > TIMETOBOOT) {
       if (ir < FINGER_ON){
        Serial.print("Finger not detected!!"); 
        Serial.println(); 
        }
      }
    
  float O = particleSensor.getFIFOIR();
  float B = particleSensor.getFIFORed();
  float T = particleSensor.readTemperature();
 
   if (isnan(ESpO2) || isnan(beatsPerMinute) || isnan(temperature)) {
      Serial.println();
    return;
}

    LINE.notify("Oxigen : "+String(ESpO2)+"%\n"+"BPM : " + String(beatsPerMinute)+"\n"+"Temp : " + String(temperature) + "องศา");
   
    Serial.print(" Oxygen % = ");
    Serial.print(ESpO2);
    Serial.print(" BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(" Temp=");
    Serial.print(temperature);
    Serial.println();
    

    if ((i % Num) == 0) {
      double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
      
      SpO2 = -23.3 * (R - 0.4) + 100; //http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;//low pass filter
      
      sumredrms = 0.0; sumirrms = 0.0; i = 0;
    }
     particleSensor.nextSample(); //We're finished with this sample so move to next sample

    delay(1000);
  }
}

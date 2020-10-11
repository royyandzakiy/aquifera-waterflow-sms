#include <Arduino.h>
#include <SoftwareSerial.h>
#include <virtuabotixRTC.h>
#include "credentials.h"

#define LED_PIN 4
#define FLOWSENSOR_PIN 2

#define SET_TIME

volatile int flow_frequency;
unsigned int l_min;
unsigned long currentTime;
unsigned long cloopTime;
unsigned int volumeDailyTotal;
bool setTime = false; 

int volumeDayArray[7];
int DayCount = 0;

virtuabotixRTC myRTC(6,7,8);

//SIM800 TX is connected to Arduino Nano D3/Arduino Uno GPIO3
//SIM800 RX is connected to Arduino Nano D2/Arduino Uno GPIO2
#define SIM800_TX_PIN 10
#define SIM800_RX_PIN 9

//Create software serial object to communicate with SIM800L
SoftwareSerial simSerial(SIM800_TX_PIN, SIM800_RX_PIN); //SIM800L Tx & Rx is connected to Arduino #3 & #2
char message[150];

void initSerial();
void initRTC();
void initSIM();
void initFlowSensor();

void flowLoop();
void volumeDataLoop();
void ATecho();

void flowFreqIncr();
void volumeDayArrayReset();
void printVolumeTotal();
void sendSMS(char *, char *);

void setup()
{
  initSerial();
  initRTC();
  initSIM();
  initFlowSensor();
  // volumeDayArrayReset();

  pinMode(LED_PIN,OUTPUT);

  Serial.println("SETUP DONE!");
  delay(1000);
}

void loop()
{
  currentTime = millis();

  // flowLoop();
  // volumeDataLoop();

  ATecho();
}

// GSM
void initSerial() {
  //Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  
  //Begin serial communication with Arduino and SIM800L
  simSerial.begin(9600);
}

void initRTC() {
  Serial.println("--- RTC Time Check Start! ---");
  if(setTime) {
    Serial.println("RTC Set Time");
    myRTC.setDS1302Time(30,35,22,6,3,8,2020);
    setTime = false;
  }

  int secondsStart;
  
  Serial.println("Check Time #1: Initial");
  myRTC.updateTime();
  Serial.println(myRTC.seconds);
  Serial.println(myRTC.minutes);
  Serial.println(myRTC.hours);
  Serial.println(myRTC.month);
  Serial.println(myRTC.year);
  secondsStart = myRTC.seconds;

  Serial.println("Check Time #2: +3 sec");
  delay(3000);
  myRTC.updateTime();
  Serial.println(myRTC.seconds);
  Serial.println(myRTC.minutes);
  Serial.println(myRTC.hours);
  Serial.println(myRTC.month);
  Serial.println(myRTC.year);

  Serial.println("Check Time #3: +5 sec");
  delay(5000);
  myRTC.updateTime();
  Serial.println(myRTC.seconds);
  Serial.println(myRTC.minutes);
  Serial.println(myRTC.hours);
  Serial.println(myRTC.month);
  Serial.println(myRTC.year);
  if (secondsStart == myRTC.seconds) {
    setTime = true;
    initRTC();
  } else {
    Serial.println("--- RTC Time Check Done! ---");
  }
}

void ATecho()
{
  delay(500);
  while (Serial.available()) 
  {
    simSerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(simSerial.available()) 
  {
    Serial.write(simSerial.read());//Forward what Software Serial received to Serial Port
  }
}

void initSIM() {  
  Serial.println("Initializing...");
  delay(1000);
  
  simSerial.println("AT"); //Once the handshake test is successful, it will back to OK
  ATecho();
  simSerial.println("AT+CSQ"); //Signal quality test, value range is 0-31, 31 is the best
  ATecho();
  simSerial.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  ATecho();
  simSerial.println("AT+CREG?"); //Check whether it has registered in the network
  ATecho();
  
  Serial.println(); 
  Serial.println("Initializing Done."); 
}

void sendSMS(char *_destPhoneNo, char *_message) {
  Serial.println("Sending SMS...");
  
  simSerial.println("AT+CMGF=1"); // Configuring TEXT mode
  ATecho();
  simSerial.println("AT+CNMI=1,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
  ATecho();
  simSerial.print("AT+CMGS=\"");
  simSerial.print(_destPhoneNo);
  simSerial.println("\"");
  ATecho();
  simSerial.print(_message); //text content
  ATecho();
  simSerial.write(26);

  Serial.println();
  Serial.println("Sending SMS Done.");
}

// FLOW SENSOR
void volumeDayArrayReset() {
  for (int i=0; i<7; i++) {
    volumeDayArray[i] = 0;
  }
  DayCount = 0;
}

void initFlowSensor() {
  pinMode(FLOWSENSOR_PIN, INPUT);
  digitalWrite(FLOWSENSOR_PIN, HIGH);
  attachInterrupt(0,flowFreqIncr,RISING);
  sei();
}

void flowFreqIncr()
{
  flow_frequency++;
}

void flowLoop() {
  bool tenSecondLater = (myRTC.seconds + 1) % 10 == 0;
  bool sendNow = tenSecondLater;

  if(sendNow) {
    // every one second, do this loop
    l_min = (flow_frequency/7.5)*1000*2.6;
    volumeDailyTotal +=(l_min/60);
    flow_frequency = 0;
    String _pesan = "Malam Aquifers, Total Daily Volume: " + (String) volumeDailyTotal + " mL";
    // String _pesan = "Halo Fiyyan, aku Jarvis, Artificial Intelligence yang akan menghidupkan Aquifera!";
    Serial.println(_pesan);
    strcpy(message, _pesan.c_str());
    sendSMS(destPhoneNo, message);
    while(1); // agar tidak mengirimkan sms secara brutal dan menghabiskan pulsa :(
  }
}

void volumeDataLoop() {
  //RTC Part
  myRTC.updateTime();

  bool every3Seconds = (myRTC.seconds + 1) % 3 == 0;
  bool every23Hours = myRTC.hours % 23 == 0;

  if(every3Seconds) {
    // after one day, save current volume data
    Serial.print("Time: ");
    Serial.println(myRTC.seconds);
    digitalWrite(LED_PIN, HIGH);
    // volumeDayArray[DayCount] = volumeDailyTotal;
    // if(DayCount == 8){
    //   volumeDayArrayReset();
    // }
    // sendSMS(destPhoneNo, message); // sending per-day

    // daily reset
    // volumeDailyTotal = 0;
    // DayCount = DayCount + 1;
  } else {
    digitalWrite(LED_PIN,LOW);
  }
}

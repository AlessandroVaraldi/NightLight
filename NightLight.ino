#include <Wire.h>
#include "string.h"
#include "RTClib.h"

#define IN_PIN 2
#define OUT_PIN 13

volatile _Bool detectedMovement = 0;

_Bool isNight = 0;
_Bool old_isNight = 0;
_Bool Ly = 0;
_Bool DST = 0;
_Bool LightOn = 0;

int today = 0;
int minuteOfTheDay = 0;
int sunrise = 0;
int sunset = 0;

char s[27] = "It's 00:00, so it's night\n";

RTC_DS3231 RTC;

void setup() {
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(IN_PIN), blink, RISING);
  pinMode(OUT_PIN, OUTPUT);
  digitalWrite(OUT_PIN, LOW);
  
  if(!RTC.begin()) Serial.print("Something went wrong!");
  else {
    if(RTC.lostPower()){
      RTC.adjust(DateTime(F(__DATE__),F(__TIME__)));
    }
  }
  RTC.disableAlarm(2); RTC.clearAlarm(2); 
  RTC.setAlarm2(DateTime(2022, 4, 13, 0, 0, 0), DS3231_A2_PerMinute);
}

void loop() {
  DateTime now = RTC.now();

  if (RTC.alarmFired(2) == true){
    RTC.clearAlarm(2); 
    
    // check for Leap year
    if(now.year()%400 == 0) Ly = 1;
    else if(now.year()%100 == 0) Ly = 0;
    else if(now.year()%4 == 0) Ly = 1;
    else Ly = 0;
  
    // adjust for Leap year
    if(now.month()<3) today = now.dayOfTheYear();
    else today = now.dayOfTheYear() - Ly;

    if((now.dayOfTheYear() >= (now.dayOfTheYear() - now.dayOfTheWeek() - 84)%7 + 83) && (now.dayOfTheYear() == (now.dayOfTheYear() - now.dayOfTheWeek() - 297)%7 + 297)) DST = 1;
    else DST = 0;
    
    // compute current minute, sunrise minute and sunset minute
    minuteOfTheDay = now.hour()*60 + now.minute();
    sunrise = -4.01317660230277e-14*pow(today,7) + 5.24982453899571e-11*pow(today,6) - 2.6453211602479e-08*pow(today,5) + 6.20876769151114e-06*pow(today,4) - 0.000614821950564693*pow(today,3) + 0.013505383167923*pow(today,2) - 0.638217752720913*today + 487.68564970024;
    sunset = -1.299988936115779e-14*pow(today,7) + 1.21921295910395e-11*pow(today,6) - 3.1302679693138e-09*pow(today,5) - 4.09286191252496e-08*pow(today,4) + 7.64371777209269e-05*pow(today,3) - 0.006500207203979*pow(today,2) + 1.495020112089128*today + 1008.0836395843;
    
    // check for night
    if(minuteOfTheDay < sunrise || minuteOfTheDay > sunset){
      old_isNight = isNight;
      isNight = 1;
      if(old_isNight != isNight) {
        Serial.println("Sunset!");
        digitalWrite(OUT_PIN,HIGH);
        delay(500);
        digitalWrite(OUT_PIN,LOW);
        delay(500);
        digitalWrite(OUT_PIN,HIGH);
        delay(500);
        digitalWrite(OUT_PIN,LOW);
      }
      sprintf(s,"It's %02d:%02d, so it's night\n", now.hour(), now.minute());
      Serial.print(s);
    }
    else{
      isNight = 0;
      sprintf(s,"It's %02d:%02d, so it's day\n", now.hour(), now.minute());
      Serial.print(s);
    }
  }

  if(detectedMovement){
      Serial.println("Detected movement!");
      detectedMovement = 0;
    if(isNight){
      LightOn = 1;
      digitalWrite(OUT_PIN,HIGH);
      Serial.println("Light is ON");
      RTC.disableAlarm(1); RTC.clearAlarm(1);
      RTC.setAlarm1(now + TimeSpan(0, 0, 0, 50), DS3231_A1_Second);
    }
  }

  if(LightOn && RTC.alarmFired(1) == true){
    RTC.disableAlarm(1); RTC.clearAlarm(1);
    digitalWrite(OUT_PIN,LOW);
    LightOn = 0; 
    Serial.println("Light is OFF");
  }
}

void blink(){
  detectedMovement = 1;
}

/*  HTTPS Web server on ESP8266
    https://www.onetransistor.eu/2019/04/https-server-on-esp8266-nodemcu.html
*/
#include <Arduino.h>
#include "EspMQTTClient.h"
#include <time.h>                   // time() ctime()
#include "secrets.h"

#define secondsOnTime 25
#define statusSeconds 5


time_t onTime;
time_t klingelDebounceTimestamp;
time_t lastStatus;
bool onState = false;
bool silentMode = false;
bool klingelDebounce = false;
int onTimeSec = 300;

// https://werner.rothschopf.net/201802_arduino_esp8266_ntp.htm
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

EspMQTTClient mqttclient(
  WIFI_AP_SSID,
  WIFI_AP_PASS,
  MQTT_SERVER_IP,
  MQTT_CLIENT_USER,
  MQTT_CLIENT_PASS,
  MQTT_CLIENT_ID,
  1883
);


time_t now;                         // this is the epoch
struct tm tm;        

void showTime() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  //Serial.print("year:");
  //Serial.print(tm.tm_year + 1900);  // years since 1900
  //Serial.print("\tmonth:");
  //Serial.print(tm.tm_mon + 1);      // January = 0 (!)
  //Serial.print("\tday:");
  //Serial.print(tm.tm_mday);         // day of month
  //Serial.print("\thour:");
  //Serial.print(tm.tm_hour);         // hours since midnight  0-23
  //Serial.print("\tmin:");
  //Serial.print(tm.tm_min);          // minutes after the hour  0-59
  //Serial.print("\tsec:");
  //Serial.print(tm.tm_sec);          // seconds after the minute  0-61*
  //Serial.print("\twday");
  //Serial.print(tm.tm_wday);         // days since Sunday 0-6
  if (tm.tm_isdst == 1){             // Daylight Saving Time flag
    //Serial.print("\tDST");
  }else{

  }
    //Serial.print("\tstandard");
  //Serial.println();
}

void sendStatus(){
  mqttclient.publish(String(MQTT_CHANNEL)+"/"+String(MQTT_CLIENT_ID)+"/"+String("Status"),(onState == true ? "1" : "0"));
}

void sendMessage(String message){
  mqttclient.publish(String(MQTT_CHANNEL)+"/"+String(MQTT_CLIENT_ID),message);
}

void onConnectionEstablished()
{
  mqttclient.subscribe(String(MQTT_CHANNEL)+"/"+String(MQTT_CLIENT_ID), [](const String & payload) {
    if ( payload.equals("on") ){
      digitalWrite(D1, HIGH);
    }
    if ( payload.equals("off") ){
      digitalWrite(D1, LOW);
    }
    if ( payload.equals("status") ){
      //Serial.println("Turning off");
      sendStatus();
    }
    if ( payload.equals("time") ){
      showTime();
    }
    
  });
  sendMessage( MQTT_CLIENT_ID + String(" online"));
}

IRAM_ATTR void klingelDetect() {
  if ( !klingelDebounce ){
    time(&klingelDebounceTimestamp);
    klingelDebounce = true;
    onState = true;
    sendStatus();
  }
}


void setup() {
  // LED IO2
  pinMode(D4,OUTPUT);
  digitalWrite(D4, HIGH);
  // Klingel Detekt IO4
  pinMode(D2,INPUT);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(D2), klingelDetect, FALLING);
  // Relais Schalter IO5
  pinMode(D1,OUTPUT);
  digitalWrite(D1, HIGH);
  
  
  //mqttclient.enableOTA("secureup!", 8448);
  mqttclient.enableDebuggingMessages(false);
  mqttclient.setWifiReconnectionAttemptDelay(10000);
  mqttclient.enableHTTPWebUpdater();
  configTime(MY_TZ, "pool.ntp.org", "time.nist.gov");  
  time(&lastStatus);
}

void loop() {
  mqttclient.loop();
  
  time_t now;
  time(&now);
  double seconds = difftime(now, lastStatus);
  if ( seconds > statusSeconds ){
    sendStatus();
    time(&lastStatus);
  }
  if ( klingelDebounce ){
    seconds = difftime(now, klingelDebounceTimestamp);
    if ( seconds > 5 ){
      klingelDebounce = false;
      onState = false;
    }
  }
}
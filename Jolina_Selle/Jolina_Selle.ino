/* Code nga mu control sa relay each connected to D2, D4, and D6
   which are controlled by the buttons connected in A3, A4, A5 ,A7 
*/

#include <Wire.h>
#include <Servo.h>
#include <RTClib.h>

Servo myServo;
DS3231 rtc;

int currentAngle = 0;

// Relay declaration
#define RELAY1 2
#define RELAY2 4
#define RELAY3 6

// Buzzer

#define BUZZER 12

//Button declaration
#define BTN1 A3
#define BTN2 A4
#define BTN3 A5
#define BTN4 A6 

bool state1 = false, state2 =false, state3 = false, state4 = false;

void setup() {
  // put your setup code here, to run once:
    pinMode(RELAY1, OUTPUT);
    pinMode(RELAY2, OUTPUT);
    pinMode(RELAY3, OUTPUT);

    pinMode(BUZZER, OUTPUT);
    myServo.write(10);
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    pinMode(BTN3, INPUT_PULLUP);
    pinMode(BTN4, INPUT_PULLUP);

    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    digitalWrite(RELAY3, HIGH);

    if(!rtc.begin()){
      Serial.println("Couldn't find RTC");  
      while(1);
    }

    
}

void loop() {
  // put your main code here, to run repeatedly:
  DateTime now = rtc.now();
  // IF ON SI BUTTON 1 MU ON SI RELAY 1
    if (digitalRead(BTN1) == LOW) {
      delay(200);
      state1 = !state1;
      digitalWrite(RELAY1, state1 ? LOW : HIGH);
    }
 // IF ON SI BUTTON 2 MU ON SI RELAY 2
    if (digitalRead(BTN2) == LOW) {
      delay(200);
      state2 = !state2;
      digitalWrite(RELAY2, state2 ? LOW : HIGH);
    }

// IF ON SI BUTTON 3 MU ON SI RELAY 3
    if (digitalRead(BTN3) == LOW) {
      delay(200);
      state3 = !state3;
      digitalWrite(RELAY3, state3 ? LOW : HIGH);
    }


    if (now.hour() == 22 && now.minute() == 0){ // 10PM check
      if (currentAngle + 25 <= 180){ // Prevents over-rotation
          currentAngle += 25;
    } else 
    {
      currentAngle = 0;
    }
  }
  myServo.write(currentAngle);

  if (now.hour() == 20 && now.minute() == 0){ // if 8PM then BUZZER AND UVLIGHT WILL TURN ON  FOR 5 MINS
      digitalWrite(RELAY1,HIGH);
      digitalWrite(BUZZER,HIGH);
      delay(300000);
  }

}



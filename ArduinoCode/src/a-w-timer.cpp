#include <Arduino.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

/*
 *  Arduino Pro Micro Clone
 *  DS3231 Breakout clone
 *  OLED 128x32 breakout
 *  10 k Poti
 *  1 Pushbutton
 */

#define OLED_RESET 4
#define COUNT_MIN 40
#define COUNT_MAX 300
#define NOTE_LENGTH 95
#define STEPS 5

#define potPin1 A1
#define potPin2 A0
#define trimPin A2

#define speakerPin 5

#define startTimer1Pin 15
#define startTimer2Pin 14
#define startHangboard 10

#define startPin1 4
#define startPin2 6
#define startPin3 7
#define startPin4 8
#define startPin5 9
#define startPin6 16

#define counterPin1 15
#define counterPin2 30
#define counterPin3 45
#define counterPin4 60
#define counterPin5 120
#define counterPin6 300

#define MELODY1 1
#define MELODY2 2
#define MELODY3 3

#define COUNTDOWN 5

#define numNotes 4
const int notes[numNotes] = {262, 294, 330, 349};

unsigned long us_test = 0;
unsigned long us_now = 0;

int countersetting = 0;
int delaysetting = 0;

int counter1;
int counter2;
int minuten1;
int minuten2;
int sekunden1;
int sekunden2;

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// Init OLED
Adafruit_SSD1306 display(OLED_RESET);

// Init the DS3231 using the hardware interface
RTC_DS3231 rtc;

void countdown_done(int melody)
{

  if (melody == MELODY1){
    for (int i=0; i<= (numNotes-1);i++){
      tone(speakerPin, notes[i],NOTE_LENGTH);
      delay(NOTE_LENGTH + 5);
    }

  } else if (melody == MELODY2) {
    for (int i=numNotes-1; i>= 0;i--){
      tone(speakerPin, notes[i],NOTE_LENGTH);
      delay(NOTE_LENGTH + 5);
    }
  }
  noTone(speakerPin);
}


void display_remaining_time(char text[], int counter, Adafruit_SSD1306 display){
  int sekunden;
  int minuten;
  // display remaining time
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(text);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  minuten = floor (counter/60);
  sekunden = counter % 60;
  if (sekunden < 10) {
    display.print("  ");display.print(minuten);display.print(":0"); display.println(sekunden);
  } else {
    display.print("  ");display.print(minuten);display.print(":"); display.println(sekunden);
  }
  display.display();
  display.clearDisplay();
}

void countdown(char text[],
                int counter,
                Adafruit_SSD1306 display,
                RTC_DS3231 rtc,
                unsigned long us_test,
                unsigned long us_now,
                int melody){
  DateTime now = rtc.now();
  while (counter >= 0) {
    if(counter<= COUNTDOWN && counter > 0){
      tone(speakerPin, notes[3],NOTE_LENGTH);
      delay(NOTE_LENGTH);
      noTone(speakerPin);

    }
    if(counter != 0){
      display_remaining_time(text,counter, display);
      us_test = now.unixtime();
    do {
      now = rtc.now();
      us_now = now.unixtime();
    } while (us_now == us_test);

    counter = counter - 1;
  }else {
    display_remaining_time(text,counter, display);
    countdown_done(melody);
    counter = counter - 1;
  }
  }
  noTone(speakerPin);
}

void timer(Adafruit_SSD1306 display,
            RTC_DS3231 rtc,
            unsigned long us_test,
            unsigned long us_now,
            int melody){

  int counter = 0;
  DateTime now = rtc.now();
  Serial.println(digitalRead(startPin6));
  while (digitalRead(startPin6) == LOW) {
    if(counter%60 == 0){
      tone(speakerPin, notes[3],NOTE_LENGTH);
      delay(NOTE_LENGTH);
      noTone(speakerPin);
    }
    display_remaining_time("Timer:",counter, display);
    us_test = now.unixtime();
    do {
      now = rtc.now();
      us_now = now.unixtime();
    } while (us_now == us_test);
    counter = counter + 1;
  }
  display_remaining_time("Timer stopped at:",counter, display);
  countdown_done(melody);
  noTone(speakerPin);
  delay(2000);
}

void countdown_hangboard(unsigned long us_test, unsigned long us_now, RTC_DS3231 rtc, Adafruit_SSD1306 display) {

  int counter = 0;
  int sets = 2;
  for (int i = 1; i<=sets ; i++) {
    counter = 10;
    countdown("HOLD", counter, display, rtc, us_test, us_now, MELODY1);

    counter = 10;
    countdown("REST", counter, display, rtc, us_test, us_now, MELODY2);

    counter = 10;
    countdown("HOLD", counter, display, rtc, us_test, us_now, MELODY1);

    if (i != sets){
      counter = 120;
      countdown("REST", counter, display, rtc, us_test, us_now, MELODY2);
    }
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 1);
  display.println("Finished");
  display.display();
  display.clearDisplay();
  countdown_done(MELODY2);
  counter = counter - 1;
}

void setup() {

  // Setup Serial connection
  Serial.begin(115200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // intenally, this will display the splashscreen.
  display.display();


  //rtc.begin();
  if (! rtc.begin()) {
    display.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    display.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  delay(2000);
  pinMode(startTimer1Pin, INPUT);
  pinMode(startTimer2Pin, INPUT);
  pinMode(startHangboard, INPUT);

  pinMode(startPin1, INPUT);
  pinMode(startPin2, INPUT);
  pinMode(startPin3, INPUT);
  pinMode(startPin4, INPUT);
  pinMode(startPin5, INPUT);
  pinMode(startPin6, INPUT);
}

void loop() {

  noTone(speakerPin);
  countersetting = analogRead(potPin1);
  delay(10);
  int val = map(countersetting, 0, 1024, COUNT_MIN, COUNT_MAX+10);
  counter1 = (round(val / STEPS)) * STEPS;

  countersetting = analogRead(potPin2);
  delay(10);
  val = map(countersetting, 0, 1024, COUNT_MIN, COUNT_MAX+10);
  counter2 = (round(val/STEPS))*STEPS;

  delaysetting = analogRead(trimPin);
  delay(10);
  val = map(delaysetting, 0, 1024, 1,9);

  // Display Timer 1
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Set Timer 1:");
  minuten1 = floor (counter1/60);
  sekunden1 = counter1 % 60;
  if (sekunden1 < 10) {
    display.print("        ");display.print(minuten1);display.print(":0"); display.println(sekunden1);
  } else {
    display.print("        ");display.print(minuten1);display.print(":"); display.println(sekunden1);
  }

  //Display Timer 2
  display.println("Set Timer 2:");
  minuten2 = floor (counter2/60);
  sekunden2 = counter2 % 60;
  if (sekunden2 < 10) {
    display.print("        ");display.print(minuten2);display.print(":0"); display.println(sekunden2);
  } else {
    display.print("        ");display.print(minuten2);display.print(":"); display.println(sekunden2);
  }
  display.display();
  display.clearDisplay();
  if (digitalRead(startTimer1Pin) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time 1", counter1 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startTimer2Pin) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time 2", counter2 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startHangboard) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown_hangboard(us_test, us_now, rtc, display);
  } else if (digitalRead(startPin1) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time", counterPin1 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startPin2) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time", counterPin2 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startPin3) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time", counterPin3 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startPin4) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time", counterPin4 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startPin5) == HIGH) {
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    countdown("Remaining Time", counterPin5 ,display, rtc, us_test, us_now, MELODY2);
  } else if (digitalRead(startPin6) == HIGH) {
    Serial.println(digitalRead(startPin6));
    countdown("Start in:", val, display, rtc, us_test, us_now, MELODY1);
    Serial.println(digitalRead(startPin6));
    delay(500);
    Serial.println(digitalRead(startPin6));
    timer(display, rtc, us_test, us_now, MELODY1);
  }
  delay(NOTE_LENGTH+5);
  noTone(speakerPin);
}

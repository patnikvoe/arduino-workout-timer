#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <time.h>

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

// define Analog Pins
#define potPin1 A1
#define potPin2 A0
#define trimPin A2
#define speakerPin 5

// define Countdown Values in Seconds
#define COUNTER1 15
#define COUNTER2 30
#define COUNTER3 45
#define COUNTER4 60
#define COUNTER5 120
#define COUNTER6 300

// define Melodys for choosing
#define MELODY1 1
#define MELODY2 2
#define MELODY3 3

// define first initial countdown in seconds
#define STARTIN 5
// define Break between Exercices
#define breakEx 15

// define Notes
#define numNotes 4
const int notes[numNotes] = {262, 294, 330, 349};

// OLED
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16
static const unsigned char PROGMEM logo16_glcd_bmp[] = {
    B00000000, B11000000, B00000001, B11000000, B00000001, B11000000, B00000011,
    B11100000, B11110011, B11100000, B11111110, B11111000, B01111110, B11111111,
    B00110011, B10011111, B00011111, B11111100, B00001101, B01110000, B00011011,
    B10100000, B00111111, B11100000, B00111111, B11110000, B01111100, B11110000,
    B01110000, B01110000, B00000000, B00110000};

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// define Variables
unsigned long us_test = 0;
unsigned long us_now = 0;

int countersetting = 0;
int counter1;
int counter2;
int minuten1;
int minuten2;
int sekunden1;
int sekunden2;

DateTime startTime;
DateTime endTime;
DateTime startTimeSet;
DateTime endTimeSet;

char pause[] = "Pause";
char go[] = "GO! GO! GO!";
char remainingTime[] = "Remaining Time";
char finished[] = "Finished!            On to the next       Exercise!";
char finished2[] = "Finished the workout!Good Job!";

// Init OLED
Adafruit_SSD1306 display(OLED_RESET);

// Init the DS3231 using the hardware interface
RTC_DS3231 rtc;

#define COLMAX 3
#define ROWMAX 4

int row[ROWMAX]; // Stores the pins for the rows

int col[COLMAX]; // Stores the Pins for the Columns

int key[ROWMAX][ROWMAX];

// addin two Matricies
void AddMatrix(int MAT1[ROWMAX][COLMAX], int MAT2[ROWMAX][COLMAX]);
// reset all keys
void resetKeys();
// check if button pressed return number
int ButtonPressed();
int keypress(int r, int c);
// Define functions
void playMelody(int melody);

void stufenintervall();

void displayTime(char text[], uint32_t unixtime);

/*void countdown(char text[], int counter, unsigned long us_test,
               unsigned long us_now, int melody);*/

void timer(unsigned long us_test, unsigned long us_now, int melody);

void countdownHangboard(unsigned long us_test, unsigned long us_now);

void displayTime4Rows(char text1[], TimeSpan counter1, char text2[],
                      TimeSpan counter2);

void startInXseconds(uint32_t unixtime, uint8_t seconds);

void waitUntilSecondPassed(uint32_t unixtime);

void stufenintervall();

/*
For Debugging Purposes only
void showDate(const char *txt, const DateTime &dt) {
  Serial.print(txt);
  Serial.print(' ');
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(' ');
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);

  Serial.print(" = ");
  Serial.print(dt.unixtime());
  Serial.print("s / ");
  Serial.print(dt.unixtime() / 86400L);
  Serial.print("d since 1970");

  Serial.println();
}

void showTimeSpan(const char *txt, const TimeSpan &ts) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.print(ts.days(), DEC);
  Serial.print(" days ");
  Serial.print(ts.hours(), DEC);
  Serial.print(" hours ");
  Serial.print(ts.minutes(), DEC);
  Serial.print(" minutes ");
  Serial.print(ts.seconds(), DEC);
  Serial.print(" seconds (");
  Serial.print(ts.totalseconds(), DEC);
  Serial.print(" total seconds)");
  Serial.println();
}
*/

void setup() {
  // Setup Serial connection
  Serial.begin(115200);

  row[0] = 3; // 1. Row
  row[1] = 4; // 2. Row
  row[2] = 5; // 3. Row
  row[3] = 6; // 4. Row
  col[0] = 9; // 1. Column
  col[1] = 9; // 2. Column
  col[2] = 9; // 3. Column

  for (int i = 0; i < ROWMAX; i++) {
    pinMode(row[i], OUTPUT);
  }
  for (int i = 0; i < ROWMAX; i++) {
    pinMode(col[i], INPUT);
    digitalWrite(col[i], HIGH);
  }

  // by default, we'll generate the high voltage from the 3.3v line internally!
  // (neat!)
  display.begin(SSD1306_SWITCHCAPVCC,
                0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  // init done

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // intenally, this will display the splashscreen.
  display.display();

  // rtc.begin();
  if (!rtc.begin()) {
    display.println("Couldn't find RTC");
    while (1)
      ;
  }

  if (rtc.lostPower()) {
    display.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  Serial.println(rtc.now().unixtime());
  delay(2000);
}

void loop() {
  /*
  Serial.println(rtc.now().unixtime());
  noTone(speakerPin);
  countersetting = analogRead(potPin1);
  delay(10);
  int val = map(countersetting, 0, 1024, COUNT_MIN, COUNT_MAX + 10);
  counter1 = (round(val / STEPS)) * STEPS;

  countersetting = analogRead(potPin2);
  delay(10);
  val = map(countersetting, 0, 1024, COUNT_MIN, COUNT_MAX + 10);
  counter2 = (round(val / STEPS)) * STEPS;

    // Display Timer 1

    /*  display.println("Set Timer 1:");
      minuten1 = floor(counter1 / 60);
      sekunden1 = counter1 % 60;
      if (sekunden1 < 10) {
        display.print("        ");
        display.print(minuten1);
        display.print(":0");
        display.println(sekunden1);
      } else {
        display.print("        ");
        display.print(minuten1);
        display.print(":");
        display.println(sekunden1);
      }

      // Display Timer 2
      display.println("Set Timer 2:");
      minuten2 = floor(counter2 / 60);
      sekunden2 = counter2 % 60;
      if (sekunden2 < 10) {
        display.print("        ");
        display.print(minuten2);
        display.print(":0");
        display.println(sekunden2);
      } else {
        display.print("        ");
        display.print(minuten2);
        display.print(":");
        display.println(sekunden2);
      }
      display.display();
      display.clearDisplay();*/
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(ButtonPressed());
  display.display();
  display.clearDisplay();
  delay(100);
  /*
    if (digitalRead(startTimer1Pin) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      stufenintervall();
    } else if (digitalRead(startTimer2Pin) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdown("Remaining Time 2", counter2, us_test, us_now, MELODY2);
    } else if (digitalRead(startHangboard) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdownHangboard(us_test, us_now);
    } else if (digitalRead(startPin1) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdown("Remaining Time", counterPin1, us_test, us_now, MELODY2);
    } else if (digitalRead(startPin2) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdown("Remaining Time", counterPin2, us_test, us_now, MELODY2);
    } else if (digitalRead(startPin3) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdown("Remaining Time", counterPin3, us_test, us_now, MELODY2);
    } else if (digitalRead(startPin4) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdown("Remaining Time", counterPin4, us_test, us_now, MELODY2);
    } else if (digitalRead(startPin5) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      // countdown("Remaining Time", counterPin5, us_test, us_now, MELODY2);
    } else if (digitalRead(startPin6) == HIGH) {
      startInXseconds(now.unixtime(), STARTIN);
      timer(us_test, us_now, MELODY1);
    }
    delay(NOTE_LENGTH + 5);
    noTone(speakerPin);
    */
}

void playMelody(int melody) {
  if (melody == MELODY1) {
    for (int i = 0; i <= (numNotes - 1); i++) {
      tone(speakerPin, notes[i], NOTE_LENGTH);
      delay(NOTE_LENGTH + 5);
    }

  } else if (melody == MELODY2) {
    for (int i = numNotes - 1; i >= 0; i--) {
      tone(speakerPin, notes[i], NOTE_LENGTH);
      delay(NOTE_LENGTH + 5);
    }
  } else if (melody == MELODY3) {
    tone(speakerPin, notes[0], NOTE_LENGTH);
    delay(NOTE_LENGTH + 5);
    tone(speakerPin, notes[2], NOTE_LENGTH);
    delay(NOTE_LENGTH + 5);
    tone(speakerPin, notes[1], NOTE_LENGTH);
    delay(NOTE_LENGTH + 5);
    tone(speakerPin, notes[3], NOTE_LENGTH);
    delay(NOTE_LENGTH + 5);
  }
  noTone(speakerPin);
}

void displayTime(char text[], uint32_t unixtime) {
  DateTime now = DateTime(unixtime);
  while (rtc.now().unixtime() == now.unixtime())
    ;
  // display remaining time
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(text);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  if (now.second() - rtc.now().second() < 10) {
    display.print("  ");
    display.print(now.minute() - rtc.now().minute());
    display.print(":0");
    display.println(now.second() - rtc.now().second());
  } else {
    display.print("  ");
    display.print(now.minute() - rtc.now().minute());
    display.print(":");
    display.println(now.second() - rtc.now().second());
  }
  display.display();
  display.clearDisplay();
}
/*
void countdown(char text[], int counter, unsigned long us_test,
               unsigned long us_now, int melody) {
  DateTime now = rtc.now();
  waitUntilSecondPassed(now.unixtime());
  now = rtc.now();
  while (counter >= 0) {
    if (counter <= STARTIN && counter > 0) {
      tone(speakerPin, notes[3], NOTE_LENGTH);
      delay(NOTE_LENGTH);
      noTone(speakerPin);
    }
    if (counter != 0) {
      displayTime(text, counter);
      us_test = now.unixtime();
      do {
        now = rtc.now();
        us_now = now.unixtime();
      } while (us_now == us_test);

      counter = counter - 1;
    } else {
      displayTime(text, counter);
      playMelody(melody);
      counter = counter - 1;
    }
  }
  noTone(speakerPin);
}
*/
/*
void timer(unsigned long us_test, unsigned long us_now, int melody) {

  int counter = 0;
  DateTime now = rtc.now();
  while (digitalRead(startPin6) == LOW) {
    if (counter % 60 == 0) {
      tone(speakerPin, notes[3], NOTE_LENGTH);
      delay(NOTE_LENGTH);
      noTone(speakerPin);
    }
    displayTime("Timer:", counter);
    us_test = now.unixtime();
    do {
      now = rtc.now();
      us_now = now.unixtime();
    } while (us_now == us_test);
    counter = counter + 1;
  }
  displayTime("Timer stopped at:", counter);
  playMelody(melody);
  noTone(speakerPin);
  delay(2000);
}
*/
void countdownHangboard(unsigned long us_test, unsigned long us_now) {
  int counter = 0;
  int sets = 2;
  for (int i = 1; i <= sets; i++) {
    counter = 10;
    // countdown("HOLD", counter, us_test, us_now, MELODY1);

    counter = 10;
    // countdown("REST", counter, us_test, us_now, MELODY2);

    counter = 10;
    // countdown("HOLD", counter, us_test, us_now, MELODY1);

    if (i != sets) {
      counter = 120;
      // countdown("REST", counter, us_test, us_now, MELODY2);
    }
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 1);
  display.println("Finished");
  display.display();
  display.clearDisplay();
  playMelody(MELODY2);
  counter = counter - 1;
}

void displayTime4Rows(char text1[], TimeSpan counter1, char text2[],
                      TimeSpan counter2) {
  int sekunden;
  int minuten;
  // display remaining time
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(text1);
  minuten = counter1.minutes();
  sekunden = counter1.seconds();
  if (sekunden < 10) {
    display.print("  ");
    display.print(minuten);
    display.print(":0");
    display.println(sekunden);
  } else {
    display.print("  ");
    display.print(minuten);
    display.print(":");
    display.println(sekunden);
  }
  display.println(text2);
  minuten = counter2.minutes();
  sekunden = counter2.seconds();
  if (sekunden < 10) {
    display.print("  ");
    display.print(minuten);
    display.print(":0");
    display.println(sekunden);
  } else {
    display.print("  ");
    display.print(minuten);
    display.print(":");
    display.println(sekunden);
  }
  display.display();
  display.clearDisplay();
}

void waitUntilSecondPassed(uint32_t unixtime) {
  while (rtc.now().unixtime() == unixtime)
    ;
}

void startInXseconds(uint32_t unixtime, uint8_t seconds) {
  while (rtc.now().unixtime() != unixtime + seconds) {
    displayTime("Start in: ", unixtime + seconds);
    if (rtc.now().unixtime() + STARTIN >= unixtime + seconds) {
      tone(speakerPin, notes[3], NOTE_LENGTH);
      delay(NOTE_LENGTH);
      noTone(speakerPin);
      waitUntilSecondPassed(rtc.now().unixtime());
    }
  }
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Start      NOW!");
  display.display();
  display.clearDisplay();
  waitUntilSecondPassed(rtc.now().unixtime());
}
/*
void stufenintervall() {
  // 7.5 minuten
  // Taste
  // Dauer von Wiederholungen -> Pause
  //
  int button = startHangboard;
  int buttonState; // 1: Excersice / 0: Break

  const TimeSpan exerciceTime = TimeSpan(0, 0, 7, 30); // 0 h 7 min 30 s

  for (int i = 0; i < 4; i += 1) {
    startTime = rtc.now();
    // waitUntilSecondPassed(startTime.unixtime());
    startTime = rtc.now();
    endTime = startTime + exerciceTime;
    startTimeSet = startTime;
    buttonState = 1;
    while (rtc.now().unixtime() <= endTime.unixtime()) {
      TimeSpan timeEx;
      TimeSpan timeBreak;
      TimeSpan timeGo;
      while (buttonState == 1) {
        timeEx = endTime - rtc.now();
        timeGo = rtc.now() - startTimeSet;

        displayTime4Rows(remainingTime, timeEx, go, timeGo); // Go Go Go!

        if (digitalRead(button) == 1) {
          buttonState = 0;
          endTimeSet = DateTime(rtc.now().unixtime() + rtc.now().unixtime() -
                                startTimeSet.unixtime());
          displayTime4Rows(remainingTime, timeEx, go, timeGo); // Go Go Go!
          break;
        }
        if (rtc.now().unixtime() == endTime.unixtime()) {

          break;
        }
      }
      while (buttonState == 0) {
        timeEx = endTime - rtc.now();
        timeBreak = endTimeSet - rtc.now();
        if (rtc.now().unixtime() + STARTIN >= endTimeSet.unixtime() &&
            rtc.now().unixtime() <= endTimeSet.unixtime()) {
          tone(speakerPin, notes[3], NOTE_LENGTH);
          delay(NOTE_LENGTH);
          noTone(speakerPin);
          if (rtc.now().unixtime() == endTimeSet.unixtime()) {
            buttonState = 1;
            startTimeSet = rtc.now();
            displayTime4Rows(remainingTime, timeEx, pause, timeBreak);
            break;
          }
        }
        displayTime4Rows(remainingTime, timeEx, pause, timeBreak); // Go Go Go!
        waitUntilSecondPassed(rtc.now().unixtime());
      }
    }
    if (i < 3) {
      // countdown("Next exercice in:", 15, us_test, us_now, MELODY3);
    }
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(finished2);
  display.display();
  display.clearDisplay();
  playMelody(MELODY3);
  noTone(speakerPin);
  delay(2000);
}
*/
void resetKeys() {
  for (int x = 0; x < COLMAX; x++) {
    for (int y = 0; y < ROWMAX; x++) {
      key[x][y] = 0;
    }
  }
}

void AddMatrix(int MAT1[4][3], int MAT2[4][3]) {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 3; y++) {
      key[x][y] = MAT1[x][y] + MAT2[x][y];
    }
  }
}

int ButtonPressed() {
  /*
     1   2   3
     4   5   6
     7   8   9
    10  11  12
  */
  int col_scan;
  for (int i = 0; i < ROWMAX; i++) {
    digitalWrite(row[0], HIGH);
    digitalWrite(row[1], HIGH);
    digitalWrite(row[2], HIGH);
    digitalWrite(row[3], HIGH);
    digitalWrite(row[i], LOW);
    for (int j = 0; j < COLMAX; j++) {
      col_scan = digitalRead(col[j]);
      if (col_scan == LOW) {
        return keypress(i, j);
        delay(200);
      } else {
        return 0;
      }
    }
  }
}

int keypress(int r, int c) {
  if (r == 0 && c == 0) {
    return 1;
  }
  if (r == 0 && c == 1) {
    return 2;
  }
  if (r == 0 && c == 2) {
    return 3;
  }
  if (r == 1 && c == 0) {
    return 4;
  }
  if (r == 1 && c == 1) {
    return 5;
  }
  if (r == 1 && c == 2) {
    return 6;
  }
  if (r == 2 && c == 0) {
    return 7;
  }
  if (r == 2 && c == 1) {
    return 8;
  }
  if (r == 2 && c == 2) {
    return 9;
  }
  if (r == 3 && c == 0) {
    return 10;
  }
  if (r == 3 && c == 1) {
    return 11;
  }
  if (r == 3 && c == 2) {
    return 12;
  }
}

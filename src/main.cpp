#include <Arduino.h>
#include <ESP32Encoder.h>
#include <TM1637Display.h>
#include <max6675.h>
#include "OneButton.h"

/*
HARDWARE LIST:
- 74HC595D (2 x 74HC595) 7-segment display
- TM1637 7-segment display
- MAX6675 thermocouple x 4
- 1 encoder
- 2 channel relay module
*/

// BUTTON PORT
#define PIN_INPUT 9

// ENCODER PORT
#define ENCODER1_A 16
#define ENCODER1_B 18

// RELAY PORT
#define RELAY1 21
#define RELAY2 17

// THERMOCOUPLE PORTS
#define THERMO_SCK 40
#define THERMO_SO 38

#define THERMO1_CS 39
#define THERMO2_CS 37
#define THERMO3_CS 35

#define THERMO_DELAY 250
#define THERMO_AVERAGE 10

// DISPLAY PORTS
#define DISPLAY1_CLK 2
#define DISPLAY1_DIO 3

#define DISPLAY2_CLK 4
#define DISPLAY2_DIO 5

#define DISPLAY3_CLK 6
#define DISPLAY3_DIO 7

// OBJECTS
OneButton button(PIN_INPUT, true, true);

ESP32Encoder encoder;

TM1637Display display1(DISPLAY1_CLK, DISPLAY1_DIO);
TM1637Display display2(DISPLAY2_CLK, DISPLAY2_DIO);
TM1637Display display3(DISPLAY3_CLK, DISPLAY3_DIO);

MAX6675 thermocouple1(THERMO_SCK, THERMO1_CS, THERMO_SO);
MAX6675 thermocouple2(THERMO_SCK, THERMO2_CS, THERMO_SO);
MAX6675 thermocouple3(THERMO_SCK, THERMO3_CS, THERMO_SO);

USBCDC serial;

// FUNCTIONS
void update_display();
void update_sensors();
void update_serial();
void update_io();

float get_avg(float *buff, float *corr);
void add_temp(float *buff, float temp);
void update_correction();
void reset_correction();

// VARIABLES
bool relay1_state = 0;
bool relay2_state = 0;

float temp1_buff[THERMO_AVERAGE];
float temp2_buff[THERMO_AVERAGE];
float temp3_buff[THERMO_AVERAGE];

float temp1_average = 0;
float temp2_average = 0;
float temp3_average = 0;

float temp1_corr = 0;
float temp2_corr = 0;
float temp3_corr = 0;

float t_avg = 0;

unsigned long last_update = 0;

uint8_t firts_start = 0;

void setup()
{
  serial.begin(115200);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);

  button.attachClick(update_correction);
  button.attachLongPressStart(reset_correction);
  button.setClickMs(200);
  button.setLongPressIntervalMs(1000);

  uint8_t blank[] = {0x00, 0x00, 0x00, 0x00};
  display1.setSegments(blank);
  display2.setSegments(blank);
  display3.setSegments(blank);

  display1.setBrightness(4);
  display2.setBrightness(4);
  display3.setBrightness(4);

  delay(1000);
}

void loop()
{
  update_sensors();
  update_display();
  update_serial();
  update_io();
}

void update_correction()
{
  float f = temp1_average + temp2_average + temp3_average;
  f = f / 3.0;
  temp1_corr = temp1_corr + f - temp1_average;
  temp2_corr = temp2_corr + f - temp2_average;
  temp3_corr = temp3_corr + f - temp3_average;
}

void reset_correction()
{
  temp1_corr = 0;
  temp2_corr = 0;
  temp3_corr = 0;
}

float get_avg(float *buff, float *corr)
{
  float sum = 0;
  for (int i = 0; i < THERMO_AVERAGE; i++)
  {
    sum += buff[i];
  }
  sum = sum / THERMO_AVERAGE;
  return sum + *corr;
}

void add_temp(float *buff, float temp)
{
  if (firts_start < 3)
  {
    for (int i = 0; i < THERMO_AVERAGE; i++)
    {
      buff[i] = temp;
    }
    firts_start++;
  }
  else
  {
    for (int i = 0; i < THERMO_AVERAGE - 1; i++)
    {
      buff[i] = buff[i + 1];
    }
    buff[THERMO_AVERAGE - 1] = temp;
  }
}

void update_display()
{
  display1.showNumberDecEx(temp1_average * 10, (0x80 >> 2), false, 4, 0);
  display2.showNumberDecEx(temp2_average * 10, (0x80 >> 2), false, 4, 0);
  display3.showNumberDecEx(temp3_average * 10, (0x80 >> 2), false, 4, 0);
}

void update_sensors()
{
  if (millis() - last_update > THERMO_DELAY)
  {
    add_temp(temp1_buff, thermocouple1.readCelsius());
    add_temp(temp2_buff, thermocouple2.readCelsius());
    add_temp(temp3_buff, thermocouple3.readCelsius());

    temp1_average = get_avg(temp1_buff, &temp1_corr);
    temp2_average = get_avg(temp2_buff, &temp2_corr);
    temp3_average = get_avg(temp3_buff, &temp3_corr);

    t_avg = (temp1_average + temp2_average + temp3_average) / 3.0;

    last_update = millis();
  }
}

void update_io()
{
  button.tick();
  //
}

void update_serial()
{
  // serial.println("t1 = " + String(temp1) + "\tt2 = " + String(temp2) + "\tt3 = " + String(temp3) + "\tt4 = " + String(temp4));
  // for (int i = 0; i < THERMO_AVERAGE; i++)
  // {
  //   serial.print(temp1_buff[i]);
  //   serial.print(" ");
  // }
  // serial.println();
  serial.printf("t_avg = %.2f\t1_d = %.2f\tt2_d = %.2f\tt3_d = %.2f\tt1_c = %.2f\tt2_c = %.2f\tt3_c = %.2f\n", t_avg, t_avg - temp1_average, t_avg - temp2_average, t_avg - temp3_average, temp1_corr, temp2_corr, temp3_corr);
}

/*
#include <Arduino.h>
#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK 2
#define DIO 3

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   2000

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
  };

TM1637Display display(CLK, DIO);

void setup()
{
}

void loop()
{
  int k;
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };
  display.setBrightness(0x0f);

  // All segments on
  display.setSegments(data);
  delay(TEST_DELAY);

  // Selectively set different digits
  data[0] = display.encodeDigit(0);
  data[1] = display.encodeDigit(1);
  data[2] = display.encodeDigit(2);
  data[3] = display.encodeDigit(3);
  display.setSegments(data);
  delay(TEST_DELAY);


  // for(k = 3; k >= 0; k--) {
  // display.setSegments(data, 1, k);
  // delay(TEST_DELAY);
  // }


  display.clear();
  display.setSegments(data+2, 2, 2);
  delay(TEST_DELAY);

  display.clear();
  display.setSegments(data+2, 2, 1);
  delay(TEST_DELAY);

  display.clear();
  display.setSegments(data+1, 3, 1);
  delay(TEST_DELAY);


  // Show decimal numbers with/without leading zeros
  display.showNumberDec(0, false); // Expect: ___0
  delay(TEST_DELAY);
  display.showNumberDec(0, true);  // Expect: 0000
  delay(TEST_DELAY);
  display.showNumberDec(1, false); // Expect: ___1
  delay(TEST_DELAY);
  display.showNumberDec(1, true);  // Expect: 0001
  delay(TEST_DELAY);
  display.showNumberDec(301, false); // Expect: _301
  delay(TEST_DELAY);
  display.showNumberDec(301, true); // Expect: 0301
  delay(TEST_DELAY);
  display.clear();
  display.showNumberDec(14, false, 2, 1); // Expect: _14_
  delay(TEST_DELAY);
  display.clear();
  display.showNumberDec(4, true, 2, 2);  // Expect: __04
  delay(TEST_DELAY);
  display.showNumberDec(-1, false);  // Expect: __-1
  delay(TEST_DELAY);
  display.showNumberDec(-12);        // Expect: _-12
  delay(TEST_DELAY);
  display.showNumberDec(-999);       // Expect: -999
  delay(TEST_DELAY);
  display.clear();
  display.showNumberDec(-5, false, 3, 0); // Expect: _-5_
  delay(TEST_DELAY);
  display.showNumberHexEx(0xf1af);        // Expect: f1Af
  delay(TEST_DELAY);
  display.showNumberHexEx(0x2c);          // Expect: __2C
  delay(TEST_DELAY);
  display.showNumberHexEx(0xd1, 0, true); // Expect: 00d1
  delay(TEST_DELAY);
  display.clear();
  display.showNumberHexEx(0xd1, 0, true, 2); // Expect: d1__
  delay(TEST_DELAY);

  // Run through all the dots
  for(k=0; k <= 4; k++) {
    display.showNumberDecEx(0, (0x80 >> k), true);
    delay(TEST_DELAY);
  }

  // Brightness Test
  for(k = 0; k < 4; k++)
  data[k] = 0xff;
  for(k = 0; k < 7; k++) {
    display.setBrightness(k);
    display.setSegments(data);
    delay(TEST_DELAY);
  }

  // On/Off test
  for(k = 0; k < 4; k++) {
    display.setBrightness(7, false);  // Turn off
    display.setSegments(data);
    delay(TEST_DELAY);
    display.setBrightness(7, true); // Turn on
    display.setSegments(data);
    delay(TEST_DELAY);
  }


  // Done!
  display.setSegments(SEG_DONE);

  while(1);
}
*/

/*
// this example is public domain. enjoy!
// https://learn.adafruit.com/thermocouple/

#include <max6675.h>
#include <LiquidCrystal.h>
#include <Wire.h>

int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
int vccPin = 3;
int gndPin = 2;

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// make a cute degree symbol
uint8_t degree[8]  = {140,146,146,140,128,128,128,128};

void setup() {
  Serial.begin(9600);
  // use Arduino pins
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  lcd.begin(16, 2);
  lcd.createChar(0, degree);

  // wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  // basic readout test, just print the current temp
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MAX6675 test");

  // go to line #1
  lcd.setCursor(0,1);
  lcd.print(thermocouple.readCelsius());
#if ARDUINO >= 100
  lcd.write((byte)0);
#else
  lcd.print(0, BYTE);
#endif
  lcd.print("C ");
  lcd.print(thermocouple.readFahrenheit());
#if ARDUINO >= 100
  lcd.write((byte)0);
#else
  lcd.print(0, BYTE);
#endif
  lcd.print('F');

  delay(1000);
}
*/

/*
#include <ESP32Encoder.h>

ESP32Encoder encoder;
ESP32Encoder encoder2;

// timer and flag for example, not needed for encoders
unsigned long encoder2lastToggled;
bool encoder2Paused = false;

void setup(){

  Serial.begin(115200);
  // Enable the weak pull down resistors

  //ESP32Encoder::useInternalWeakPullResistors = puType::down;
  // Enable the weak pull up resistors
  ESP32Encoder::useInternalWeakPullResistors = puType::up;

  // use pin 19 and 18 for the first encoder
  encoder.attachHalfQuad(19, 18);
  // use pin 17 and 16 for the second encoder
  encoder2.attachHalfQuad(17, 16);

  // set starting count value after attaching
  encoder.setCount(37);

  // clear the encoder's raw count and set the tracked count to zero
  encoder2.clearCount();
  Serial.println("Encoder Start = " + String((int32_t)encoder.getCount()));
  // set the lastToggle
  encoder2lastToggled = millis();
}

void loop(){
  // Loop and read the count
  Serial.println("Encoder count = " + String((int32_t)encoder.getCount()) + " " + String((int32_t)encoder2.getCount()));
  delay(100);

  // every 5 seconds toggle encoder 2
  if (millis() - encoder2lastToggled >= 5000) {
    if(encoder2Paused) {
      Serial.println("Resuming Encoder 2");
      encoder2.resumeCount();
    } else {
      Serial.println("Paused Encoder 2");
      encoder2.pauseCount();
    }

    encoder2Paused = !encoder2Paused;
    encoder2lastToggled = millis();
  }
}
*/

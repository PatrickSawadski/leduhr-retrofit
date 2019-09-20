#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include "clock.h"

#define LEDUHR_SEC_LED0 A1
#define LEDUHR_MIN_LED0 A2
#define LEDUHR_HOU_LED0 A3
#define LEDUHR_SEC_DAT  2
#define LEDUHR_SEC_CLK  4
#define LEDUHR_SEC_RST  5
#define LEDUHR_MIN_DAT  6
#define LEDUHR_MIN_CLK  7
#define LEDUHR_MIN_RST  8
#define LEDUHR_HOU_DAT  9
#define LEDUHR_HOU_CLK  10
#define LEDUHR_HOU_RST  11

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Clock clock(3, 58, 0);

uint8_t bufhours[2];
uint8_t bufminutes[8];
uint8_t bufseconds[8];

volatile uint8_t pwmsync = 0;

void sendBufferToLeduhr(
  uint8_t *buf,
  uint8_t len,
  uint8_t datpin,
  uint8_t clkpin,
  uint8_t rstpin,
  uint8_t led0pin
) {

  digitalWrite(clkpin, LOW);
  
  pwmsync = 1;
  while (pwmsync) { delayMicroseconds(10); } // wait for pwmcycle to finish

  digitalWrite(rstpin, HIGH);
  digitalWrite(rstpin, LOW);

  for (uint8_t i = (len-1); i > 0; i--) { // Die erste LED hängt nicht am Schieberegister
    uint8_t state = (buf[i/8] >> (i%8)) & 0x01;
    digitalWrite(datpin, state);
    digitalWrite(clkpin, HIGH);
    digitalWrite(clkpin, LOW);
  }
  digitalWrite(led0pin, buf[0] & 0x01);
}

void leduhrUpdateSec() {
  sendBufferToLeduhr(bufseconds, 60, LEDUHR_SEC_DAT, LEDUHR_SEC_CLK, LEDUHR_SEC_RST, LEDUHR_SEC_LED0);
}

void leduhrUpdateMin() {
  sendBufferToLeduhr(bufminutes, 60, LEDUHR_MIN_DAT, LEDUHR_MIN_CLK, LEDUHR_MIN_RST, LEDUHR_MIN_LED0);
}
void leduhrUpdateHou() {
  sendBufferToLeduhr(bufhours, 12, LEDUHR_HOU_DAT, LEDUHR_HOU_CLK, LEDUHR_HOU_RST, LEDUHR_HOU_LED0);
}


void printBuffer(
  Adafruit_SSD1306 *disp,
  uint8_t x,
  uint8_t y,
  uint8_t r,
  uint8_t len,
  uint8_t *buf
) {
  float rot = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t x1, y1;
    x1 = x + r * sin(rot);
    y1 = y - r * cos(rot);
    
    if ( ( buf[i/8] >> (i%8) ) & 0x01 ) {
      disp->drawCircle(x1, y1, 1, WHITE);
    }
    disp->drawPixel(x1, y1, WHITE);
    rot += 2.0 * PI / len;
  }

}

void printHourBuffer( Adafruit_SSD1306 *disp, uint8_t x, uint8_t y ) {
  printBuffer(disp, x, y, 22, 12, bufhours);
}

void printMinuteBuffer( Adafruit_SSD1306 *disp, uint8_t x, uint8_t y ) {
  printBuffer(disp, x, y, 26, 60, bufminutes);
}

void printSecondBuffer( Adafruit_SSD1306 *disp, uint8_t x, uint8_t y ) {
  printBuffer(disp, x, y, 30, 60, bufseconds);
}

void oledclockUpdate(void) {
  /* Print clock buffer to oled screen */
  display.clearDisplay();
  printHourBuffer(&display, 63, 31);
  printMinuteBuffer(&display, 63, 31);
  printSecondBuffer(&display, 63, 31);
  display.display();
}

ISR(TIMER2_OVF_vect) {
  pwmsync = 0;
}

void setup() {
  /* Setup (Fast-)PWM */
  DDRD |= (1 << DDD3);
  
  OCR2A = 100;
  OCR2B = 100 - 2; // Helligkeit

  TCCR2A = (1 << COM2B1) | (0 << COM2B0) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (1 << WGM22) | (1 << CS22) | (1 << CS21);
  TIMSK2 = (1 << TOIE2);
  sei();
  

  /* Setup for real Leduhr output */
  pinMode(LEDUHR_SEC_LED0, OUTPUT);
  pinMode(LEDUHR_MIN_LED0, OUTPUT);
  pinMode(LEDUHR_HOU_LED0, OUTPUT);
  pinMode(LEDUHR_SEC_DAT, OUTPUT);
  pinMode(LEDUHR_SEC_CLK, OUTPUT);
  pinMode(LEDUHR_SEC_RST, OUTPUT);
  pinMode(LEDUHR_MIN_DAT, OUTPUT);
  pinMode(LEDUHR_MIN_CLK, OUTPUT);
  pinMode(LEDUHR_MIN_RST, OUTPUT);
  pinMode(LEDUHR_HOU_DAT, OUTPUT);
  pinMode(LEDUHR_HOU_CLK, OUTPUT);
  pinMode(LEDUHR_HOU_RST, OUTPUT);

  /* Setup for OLED output */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void clearHouBuf(void) {
  bufhours[0] = 0;
  bufhours[1] = 0;
}

void clearMinBuf(void) {
  for (uint8_t i = 0; i < 8; i++) {
    bufminutes[i] = 0;
  }
}

void clearSecBuf(void) {
  for (uint8_t i = 0; i < 8; i++) {
    bufseconds[i] = 0;
  }
}

void loop() {
  /* simple clock display function, can be far more complicated ;-) */
  static uint8_t lastHour = 0xFF;
  static uint8_t lastMinute = 0xFF;
  static uint8_t lastSecond = 0xFF;
  static uint32_t lastmillis = 0;

  uint32_t actmillis = millis();
  clock.tick(actmillis - lastmillis);
  lastmillis = actmillis;

  uint32_t hour = clock.getHours();
  uint32_t minute = clock.getMinutes();
  uint32_t second = clock.getSeconds();

  /* Update seconds */
  if (lastSecond != second) {
    for (uint8_t i = 0; i <= 60; i++) {
      uint32_t sec = second + i;
      while (sec > 59) sec -= 60;
      clearSecBuf();
      bufseconds[sec/8] = 1 << sec%8;
      leduhrUpdateSec();
      delay(5);
    }
  }
  
  /* Update minutes */
  if (lastMinute != minute) {
    for (uint8_t i = 0; i <= 60; i++) {
      uint32_t min = minute + i;
      while (min > 59) min -= 60;
      clearMinBuf();
      bufminutes[min/8] = 1 << min%8;
      leduhrUpdateMin();
      delay(5);
    }
  }

  /* Update hours */
  if (lastHour != hour) {
    clearHouBuf();
    bufhours[hour/8] = 1 << hour%8;
    leduhrUpdateHou();
  }
  
  lastSecond = second;
  lastMinute = minute;
  lastHour = hour;

  oledclockUpdate(); // oled ist zu langsam fuer so krasse animationen

  delay(1); // just some delay ja ja
}
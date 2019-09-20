#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include "clock.h"

#define DDR_LED0        DDRC
#define PORT_LED0       PORTC
#define PIN_LED0_S      PC1
#define PIN_LED0_M      PC2
#define PIN_LED0_H      PC3
#define DDR_CLK_DAT     DDRB
#define PORT_CLK_DAT    PORTB
#define PIN_CLK_MS      PB0
#define PIN_CLK_H       PB1
#define PIN_DAT_S       PB2
#define PIN_DAT_M       PB3
#define PIN_DAT_H       PB4
#define DDR_RST         DDRD
#define PORT_RST        PORTD
#define PIN_RST         PD2

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Clock clock(3, 58, 0);

uint8_t bufH[2];
uint8_t bufM[8];
uint8_t bufS[8];

uint8_t outbuf[60];

volatile uint8_t outBufReady = 0;

void leduhrInitHardware(void) {
  DDR_LED0 |=
    (1 << PIN_LED0_H) |
    (1 << PIN_LED0_M) |
    (1 << PIN_LED0_S);
  DDR_CLK_DAT |=
    (1 << PIN_CLK_MS) | 
    (1 << PIN_CLK_H) | 
    (1 << PIN_DAT_S) | 
    (1 << PIN_DAT_M) | 
    (1 << PIN_DAT_H);
  DDR_RST |= (1 << PIN_RST);
}

void leduhrPrepareOutBuf(void) {
  if (outBufReady) return; // last buffer isn't send
  
  uint8_t *pbuf = &outbuf[59];

  for (uint8_t i = 1; i < 60; i++) {
    if (i < 12) {
      *pbuf = ((bufH[i/8] >> (i%8)) & 0x01) << PIN_DAT_H;
    } else {
      *pbuf = (1 << PIN_CLK_H);
    }
    *pbuf |= (((bufS[i/8] >> (i%8)) & 0x01) << PIN_DAT_S) |
            (((bufM[i/8] >> (i%8)) & 0x01) << PIN_DAT_M);
    pbuf--;
  }
  // first led is connected directly, prepare output register
  *pbuf = ((bufH[0] & 0x01) << PIN_LED0_H) |
          ((bufM[0] & 0x01) << PIN_LED0_M) |
          ((bufS[0] & 0x01) << PIN_LED0_S);

  outBufReady = 1;  // Signal to PWM, so it can output at next high to low transition
}

void leduhrSendOutBuf(void) {
  uint8_t mask;
  uint8_t *pdat = outbuf;

  PORT_RST |= (1 << PIN_RST);
  PORT_RST &= ~(1 << PIN_RST);
  
   // first Led is connected directly to controller pin
  mask = (1 << PIN_LED0_H)|(1 << PIN_LED0_M)|(1 << PIN_LED0_S);
  PORT_LED0 = (PORT_LED0 & ~mask) | *pdat;
  pdat++;

  // all others via shift register
  mask =  (1 << PIN_DAT_H) |
          (1 << PIN_DAT_M) |
          (1 << PIN_DAT_S) |
          (1 << PIN_CLK_MS) |
          (1 << PIN_CLK_H);
  for (uint8_t i = 1; i < 60; i++) {
    PORT_CLK_DAT = (PORT_CLK_DAT & ~mask) | *pdat;
    PORT_CLK_DAT |= (1 << PIN_CLK_MS) | (1 << PIN_CLK_H);
    pdat++;
  }
  PORT_CLK_DAT &= ~((1 << PIN_CLK_MS) | (1 << PIN_CLK_H));
}

void leduhrUpdate() {
  leduhrPrepareOutBuf();
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
  printBuffer(disp, x, y, 22, 12, bufH);
}

void printMinuteBuffer( Adafruit_SSD1306 *disp, uint8_t x, uint8_t y ) {
  printBuffer(disp, x, y, 26, 60, bufM);
}

void printSecondBuffer( Adafruit_SSD1306 *disp, uint8_t x, uint8_t y ) {
  printBuffer(disp, x, y, 30, 60, bufS);
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
  if (outBufReady) {
    leduhrSendOutBuf();
    outBufReady = 0;
  }
}

void setup() {
  /* Setup (Fast-)PWM */
  DDRD |= (1 << DDD3);
  
  OCR2A = 100;
  OCR2B = 100 - 4; // Helligkeit

  TCCR2A = (1 << COM2B1) | (0 << COM2B0) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (1 << WGM22) | (1 << CS21) | (1 << CS20);
  TIMSK2 = (1 << TOIE2);
  sei();

  /* Setup for real Leduhr output */
  leduhrInitHardware();

  /* Setup for OLED output */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
}

void clearBufH(void) {
  bufH[0] = 0;
  bufH[1] = 0;
}

void clearBufM(void) {
  for (uint8_t i = 0; i < 8; i++) {
    bufM[i] = 0;
  }
}

void clearBufS(void) {
  for (uint8_t i = 0; i < 8; i++) {
    bufS[i] = 0;
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
      clearBufS();
      bufS[sec/8] = 1 << sec%8;
      leduhrUpdate();
      delay(5);
    }
  }
  
  /* Update minutes */
  if (lastMinute != minute) {
    for (uint8_t i = 0; i <= 60; i++) {
      uint32_t min = minute + i;
      while (min > 59) min -= 60;
      clearBufM();
      bufM[min/8] = 1 << min%8;
      leduhrUpdate();
      delay(5);
    }
  }

  /* Update hours */
  if (lastHour != hour) {
    clearBufH();
    bufH[hour/8] = 1 << hour%8;
    leduhrUpdate();
  }
  
  lastSecond = second;
  lastMinute = minute;
  lastHour = hour;

  oledclockUpdate(); // oled ist zu langsam fuer so krasse animationen

  delay(1); // just some delay ja ja
}
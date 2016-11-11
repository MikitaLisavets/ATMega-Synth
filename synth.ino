#include <frequencyToNote.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/cos2048_int8.h>
#include <LowPassFilter.h>
#include <mozzi_rand.h>

#include <SPI.h>
#include "Ucglib.h"

#define CONTROL_RATE 64
#define BTNS_COUNT 4

#include <EventDelay.h>

float freq = 0;
int pressedBtn = -1;
int btnCount = 0;

int pot = A0;

int rowPins[BTNS_COUNT] = {5, 4, 3, 2};
int colPins[BTNS_COUNT] = {A2, A3, A4, A5};
boolean btnState[BTNS_COUNT][BTNS_COUNT] = {
 {false, false, false, false},
 {false, false, false, false},
 {false, false, false, false},
 {false, false, false, false}
};

float notes[16] = {freqC3, freqD3, freqE3, freqF3, freqG3, freqA3, freqB3,
                   freqC4, freqD4, freqE4, freqF4, freqG4, freqA4, freqB4,
                   freqC5, freqD5};

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc(SIN2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);
Ucglib_ST7735_18x128x160_SWSPI ucg(/*sclk=*/ 13, /*data=*/ 11, /*cd=*/ 6 , /*cs=*/ 8, /*reset=*/ 7);
LowPassFilter lpf;
EventDelay eventDelay;

void setup(){
  for(int i = 0; i < BTNS_COUNT; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], HIGH);
  }
  startMozzi(CONTROL_RATE);

  lpf.setResonance(100);
  eventDelay.set(1000);
  ucg.begin(UCG_FONT_MODE_NONE);
  ucg.clearScreen();
}

void updateControl() {
  readPots();
  readBtns();
  checkState();
  osc.setFreq(freq);
}

int updateAudio(){
  char asig = lpf.next(osc.next());
  return (int) asig;
}

void loop(){
  audioHook();
}

void draw() {
  ucg.setColor(0, 255, 0, 0);
  ucg.setColor(1, 0, 255, 0);
  ucg.setColor(2, 0, 0, 255);
  ucg.setColor(3, 0, 0, 0);
  ucg.drawGradientBox(0, 0, ucg.getWidth(), ucg.getHeight());
}

void readPots() {
  int val = mozziAnalogRead(pot);
  //Serial.println(val);

  val = map(val, 0, 1023, 0, 10);

  kFilterMod.setFreq((float) val);
  byte cutoff_freq = 100 + kFilterMod.next()/2;
  lpf.setCutoffFreq(cutoff_freq);

}

void readBtns() {
  int val, result = -1;
  btnCount = 0;
  for(int i = 0; i < BTNS_COUNT; i++) {
    digitalWrite(colPins[i], LOW);
    for(int j = 0; j < BTNS_COUNT; j++) {
      val = digitalRead(rowPins[j]);
      if (val == 0) {
        btnState[i][j] = true;
        btnCount++;
      } else {
        btnState[i][j] = false;
      }
    }
    digitalWrite(colPins[i], HIGH);
  }
}

void checkState() {
  boolean isPressed = false;
  int num;
  for(int i = 0; i < BTNS_COUNT; i++) {
    for(int j = 0; j < BTNS_COUNT; j++) {
      num = BTNS_COUNT*j+i;
      if (btnState[i][j] == true) {
        isPressed = true;
        if (btnCount == 1) {
          pressedBtn = btnState[i][j];
          freq = notes[num];
        } else {
          if (pressedBtn != btnState[i][j]) {
            freq = notes[num];
          }
        }
      }
    }
  }
  if (isPressed == false) {
    freq = 0;
    pressedBtn = -1;
    if(eventDelay.ready()){
      draw();
      eventDelay.start();
    }
  }
}

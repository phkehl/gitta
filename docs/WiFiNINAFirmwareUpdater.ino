/*
  WiFiNINAFirmwareUpdater - Firmware Updater for the 
  Arduino MKR WiFi 1010, Arduino MKR Vidor 4000, and Arduino UNO WiFi Rev.2.
  
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// /home/flip/.arduino15/packages/arduino/hardware/samd/1.6.19/variants/mkrwifi1010/variant.cpp

// must change in variant.cpp to make the interrupts work..
//  { PORTA, 14, PIO_DIGITAL,    (PIN_ATTR_NONE                                ), No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_14 /*EXTERNAL_INT_NONE*/ }, // SS:   as GPIO
//  { PORTA, 15, PIO_SERCOM,     (PIN_ATTR_NONE                                ), No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_15 /*EXTERNAL_INT_NONE*/ }, // MISO: SERCOM2/PAD[3]
// see EIC / EXTINT assignement in chapter "7.1 Multiplexed Signals"


// NINA output --> Arduino Input
#define NINA_CLK_PIN 29 // PA15 = NINA_SCK
#define NINA_DO_PIN  28 // PA14 = labelled as NINA_CS, but it's NINA_MOSI (SPIV_DO/GPIO19)
//#define NINA_DO_PIN  26 // PA12 = NINA_MOSI (wrong in schematics)


// Ardino output
#define MKR_MOSI_PIN 8 // PA16
#define MKR_SCK_PIN  9 // PA17

// direct pin access via registers, https://forum.arduino.cc/index.php?topic=334073.0

         uint32_t  mkrMosiPin    = digitalPinToBitMask(MKR_MOSI_PIN);
//volatile uint32_t *mkrMosiOut    = portOutputRegister(digitalPinToPort(MKR_MOSI_PIN)); // &(digitalPinToPort(MKR_MOSI_PIN)->OUT.reg);
volatile uint32_t *mkrMosiOutSet = &(digitalPinToPort(MKR_MOSI_PIN)->OUTSET.reg);
volatile uint32_t *mkrMosiOutClr = &(digitalPinToPort(MKR_MOSI_PIN)->OUTCLR.reg);

         uint32_t  mkrSckPin     = digitalPinToBitMask(MKR_SCK_PIN);
//volatile uint32_t *mkrSckOut     = portOutputRegister(digitalPinToPort(MKR_SCK_PIN)); // &(digitalPinToPort(MKR_SCK_PIN)->OUT.reg);
volatile uint32_t *mkrSckOutSet  = &(digitalPinToPort(MKR_SCK_PIN)->OUTSET.reg);
volatile uint32_t *mkrSckOutClr  = &(digitalPinToPort(MKR_SCK_PIN)->OUTCLR.reg);

         uint32_t  ninaDoPin     = digitalPinToBitMask(NINA_DO_PIN);
volatile uint32_t *ninaDoIn      = portInputRegister(digitalPinToPort(NINA_DO_PIN)); // &(digitalPinToPort(NINA_DO_PIN)->IN.reg);

         uint32_t  ninaClkPin    = digitalPinToBitMask(NINA_CLK_PIN);
volatile uint32_t *ninaClkIn     = portInputRegister(digitalPinToPort(NINA_CLK_PIN)); // &(digitalPinToPort(NINA_CLK_PIN)->IN.reg);

unsigned long baud = 921600;

void setup()
{
  SerialUSB.begin(baud);
  SerialNina.begin(baud);
  
  pinMode(NINA_GPIO0, OUTPUT); // 30, PA14
  pinMode(NINA_RESETN, OUTPUT); // 

  pinMode(NINA_CLK_PIN, INPUT_PULLUP);
  pinMode(NINA_DO_PIN, INPUT_PULLUP);
  pinMode(MKR_MOSI_PIN, OUTPUT);
  pinMode(MKR_SCK_PIN, OUTPUT);
  
  // doesn't work unless variant.cpp is modified, it will work more or less fine up to about 150kHz
  attachInterrupt(NINA_CLK_PIN, clkIsr, CHANGE);
  attachInterrupt(NINA_DO_PIN, doIsr, CHANGE);
}

void clkIsr(void)
{
  //digitalWrite(MKR_SCK_PIN, digitalRead(NINA_CLK_PIN));
  //if (digitalRead(NINA_CLK_PIN)) { *mkrSckOutSet = mkrSckPin; } else { *mkrSckOutClr = mkrSckPin; }
  if ((*ninaClkIn & ninaClkPin) != 0) { *mkrSckOutSet = mkrSckPin; } else { *mkrSckOutClr = mkrSckPin; }
}

void doIsr(void)
{
  //digitalWrite(MKR_MOSI_PIN, digitalRead(NINA_DO_PIN));
  //if (digitalRead(NINA_DO_PIN)) { *mkrMosiOutSet = mkrMosiPin; } else { *mkrMosiOutClr = mkrMosiPin; }
  if ((*ninaDoIn & ninaDoPin) != 0) { *mkrMosiOutSet = mkrMosiPin; } else { *mkrMosiOutClr = mkrMosiPin; }
}

void loop() {

  // handle reset and booting ESP32
#if 0
  static int oldRts = 0;
  static int oldDtr = 0;
  int rts = SerialUSB.rts();
  int dtr = SerialUSB.dtr();
  if (rts != oldRts)
  {
    SerialUSB.print("rts "); SerialUSB.print(rts); SerialUSB.println();
  }
  if (dtr != oldDtr)
  {
    SerialUSB.print("dtr "); SerialUSB.print(dtr); SerialUSB.println();
  }
  oldRts = rts;
  oldDtr = dtr;
  digitalWrite(NINA_RESETN, rts);
  digitalWrite(NINA_GPIO0, (dtr == 0) ? HIGH : LOW);
#else
  digitalWrite(NINA_RESETN, SerialUSB.rts());
  digitalWrite(NINA_GPIO0, (SerialUSB.dtr() == 0) ? HIGH : LOW);
#endif


  // check if the USB virtual serial wants a new baud rate
  if (SerialUSB.baud() != baud)
  {
    baud = SerialUSB.baud();
    SerialNina.begin(baud);
  }

  // forward data serial: Arduino <--> EPS32
#if 1
  /*while*/ if (SerialUSB.available()) {
    uint8_t c = SerialUSB.read();
    SerialNina.write(c);
  }

  /*while*/ if (SerialNina.available()) {
    uint8_t c = SerialNina.read();
    SerialUSB.write(c);
  }
#else
  int go = 2;
  while (go > 0)
  {
    go = 0;
    if (SerialUSB.available())  { uint8_t c = SerialUSB.read();  SerialNina.write(c); go++; }
    if (SerialNina.available()) { uint8_t c = SerialNina.read(); SerialUSB.write(c);  go++; }
  }
#endif

  // forward SPI: ESP32 --> Arduino
#if 0
  bool newClkState = digitalRead(CLK_PIN);
  if (clkState != newClkState)
  {
    clkState = newClkState;
    if (clkState) { SerialUSB.println("clk: high"); } else { SerialUSB.println("clk: low"); }
  }
  bool newDoState = digitalRead(DO_PIN);
  if (doState != newDoState)
  {
    doState = newDoState;
    if (doState) { SerialUSB.println("do: high"); } else { SerialUSB.println("do: low"); }
  }
#else
  //digitalWrite(MOSI_PIN, digitalRead(DO_PIN));
  //digitalWrite(SCK_PIN,  digitalRead(CLK_PIN));
#endif

}

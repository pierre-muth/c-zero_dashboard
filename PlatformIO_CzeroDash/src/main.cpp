#include <Arduino.h>
#include <CAN.h>
#include <U8g2lib.h> 

U8G2_SSD1305_128X64_ADAFRUIT_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 3, 2);

uint8_t data[8] = {0x11, 0x22, 0x88, 0x99, 0xAA, 0xBB, 0xEE, 0xFF}; 
uint16_t cellVoltages[80];
double battAmp = -1.33, battVolt = 318.4, powerKW = -0.4;
uint32_t powHistoryIndex = 0;
float powHistory[80];
double cellMinVoltage = 3.71, cellMaxVoltage = 4.09;
double soc1 = 90, soc2 = 89;
uint8_t drawing = 0;

void onReceive(int packetSize) {
  if (drawing) return;
  uint8_t index = 0;
  int8_t indexOffsetV = 0;

  if (packetSize) {
    // received a packet
    if (CAN.packetId() == 0x6E1) {
      CAN.readBytes(data, 8);
      index = data[0]-1;
      if (index > 5) {
        indexOffsetV = -4;
      }
      cellVoltages[index*8 +0 +indexOffsetV] = data[4]*256 + data[5];
      cellVoltages[index*8 +1 +indexOffsetV] = data[6]*256 + data[7];
    }

    if (CAN.packetId() == 0x6E2) {
      CAN.readBytes(data, 8);
      index = data[0]-1;
      if (index > 5) {
        indexOffsetV = -4;
      }
      cellVoltages[index*8 +2 +indexOffsetV] = data[4]*256 + data[5];
      cellVoltages[index*8 +3 +indexOffsetV] = data[6]*256 + data[7];
    }

    if (CAN.packetId() == 0x6E3) {
      CAN.readBytes(data, 8);
      index = data[0]-1;
      if (index > 5) {
        indexOffsetV = -4;
      }
      if (index == 5 || index == 11) return;
      cellVoltages[index*8 +4 +indexOffsetV] = data[4]*256 + data[5];
      cellVoltages[index*8 +5 +indexOffsetV] = data[6]*256 + data[7];
    }

    if (CAN.packetId() == 0x6E4) {
      CAN.readBytes(data, 8);
      index = data[0]-1;
      if (index > 5) {
        indexOffsetV = -4;
      }
      if (index == 5 || index == 11) return;
      cellVoltages[index*8 +6 +indexOffsetV] = data[4]*256 + data[5];
      cellVoltages[index*8 +7 +indexOffsetV] = data[6]*256 + data[7];
    }

    if (CAN.packetId() == 0x373) {
      CAN.readBytes(data, 8);
      cellMaxVoltage = (data[0] + 210) / 100.0;
      cellMinVoltage = (data[1] + 210) / 100.0;
      battAmp = (data[2] * 256.0) + data[3];
      battAmp -= 32768.0;
      battAmp /= -100.0;
      battVolt = (data[4] * 256.0) + data[5];
      battVolt *= 0.1;
      powerKW = (battAmp*battVolt)/1000.0;

    }

    if (CAN.packetId() == 0x374) {
      CAN.readBytes(data, 8);
      soc1 = (data[0] - 10.0) / 2.0;
      soc2 = (data[1] - 10.0) / 2.0;
    }
  }
}

void setup() {
  // start serial
  Serial.begin(9600);
  delay(3000);

  // init vars
  uint8_t i;
  for(i=0; i<80; i++){
    cellVoltages[i] = 410 - ((i+3)%6);
    powHistory[i] = 20 - (i%30);
  }
  
  // start display
  u8g2.begin();
  u8g2.clear();
  u8g2.clearBuffer();
  u8g2.clearDisplay();
  delay(100);
    
  // start the CAN bus at 500 kbps
  CAN.setClockFrequency(8e6);
  CAN.setPins(10, 7);
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  } else {
    Serial.println("CAN started");
  }

  // register the receive callback
  CAN.onReceive(onReceive);
}

uint32_t x, y;
uint32_t loopCount = 0;
uint16_t cellMin = 0;

void loop() {
  loopCount++;
  drawing = 1;

  // log power history
  powHistory[powHistoryIndex%80] = powerKW/3.0;
  powHistoryIndex++;

  // start drawing
  u8g2.setFont(u8g2_font_amstrad_cpc_extended_8r);
  u8g2.firstPage();
  do {
    if (loopCount%2==0) u8g2.drawPixel(0,0);
    
    u8g2.setCursor(8, 9);
    u8g2.print("C-Zero  ");
    u8g2.print(soc2, 1);
    u8g2.print("%");

    u8g2.setCursor(85, 28);
    u8g2.print(powerKW, 1);
    u8g2.setCursor(85, 37);
    u8g2.print("   KW");

    for(x=0; x<80; x++){
      y = 32-(powHistory[(x+powHistoryIndex)%80]);
      if (y<8) y=8;
      if (y>45) y=45;
      u8g2.drawLine(x, 32, x, y);
    }

    cellMin = 1000;
    for(x=0; x<80; x++){
      if (cellVoltages[x] < cellMin) cellMin =  cellVoltages[x];
    }
    cellMin -= 2;
    for(x=0; x<79; x++){
      y = 64-(cellVoltages[x]-cellMin);
      if (y<54) y=54;
      u8g2.drawLine(x, 64, x, y);
    }

    u8g2.setCursor(85, 54);
    u8g2.print(cellMaxVoltage, 2);
    u8g2.print("v");
    u8g2.setCursor(85, 63);
    u8g2.print(cellMinVoltage, 2);
    u8g2.print("v");

  } while ( u8g2.nextPage() );
  drawing = 0;
  
  delay(311);
}
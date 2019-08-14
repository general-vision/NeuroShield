// -----------------------------------------------------------------------
// Test_SPIcomm_Level 1
// Copyright 2018 General Vision Inc.
//
// Updated 09/24/2018
//
// Use of the SPI library
// Test of Read/Write to global registers
// Modules and Registers accessible through SPI are defined in NeuroMemSPI.h
//
// Loop with increasing MINIF while decreasing MAXIF
//
// -----------------------------------------------------------------------

#include <SPI.h>

#define HW_NEUROSHIELD 2 // neuron access through SPI_CS pin 7
#define HW_NEUROSTAMP 3 // neuron access through SPI_CS pin 10

// SPI chip select
#define NM_CS_NEUROSHIELD 7
#define NM_CS_NEUROSTAMP 10

// SPI speed
#define CK_NEUROSHIELD 2000000 // 2 Mhz
#define CK_NEUROSTAMP 2000000 // 2 Mhz

int SPISelectPin;
int SPIClockDiv;

int iteration=0, error=0, data=0, value1=1, value2=65535;

void setup() {
  Serial.begin(115200);
  while (!Serial);  // wait for the serial port to open
    Serial.print("\nTest SPI Level1 with no GV lib dependencies\n");
  int NMplatform=2;
  Serial.print("\nSelect your NeuroMem platform:\n\t2 - NeuroShield\n\t3 - NeuroStamp\n");
  while(Serial.available() <1);

  NMplatform = Serial.read() - 48;
  Serial.print("\nPlatform "); Serial.println(NMplatform);
  SPI.begin();
  switch(NMplatform)
  {
    case HW_NEUROSHIELD:
      SPIClockDiv= SPI_CLOCK_DIV8;
      SPI.setClockDivider(SPIClockDiv);
      SPISelectPin = NM_CS_NEUROSHIELD;
      pinMode (SPISelectPin, OUTPUT);
      digitalWrite(SPISelectPin, HIGH);
      pinMode (5, OUTPUT);
      digitalWrite(5, LOW);
      pinMode (6, OUTPUT);
      digitalWrite(6,HIGH);
      delay(100);
      break;
    case HW_NEUROSTAMP:
      SPIClockDiv= SPI_CLOCK_DIV16;
      SPI.setClockDivider(SPIClockDiv);
      SPISelectPin = NM_CS_NEUROSTAMP;
      pinMode (SPISelectPin, OUTPUT);
      digitalWrite(SPISelectPin, HIGH);
      delay(100);
      break;
  }  
  //
  // Test RW global registers
  //
  spiWriteNM(15,0);
  error=0;
  data= spiReadNM(6);
  Serial.print("\nMINIF=");Serial.print(data);
  if (data!= 2) {error=1; Serial.print("\tis Incorrect");}
  data= spiReadNM(7);
  Serial.print("\nMAXIF="); Serial.print(data);
  if (data!= 16384) {error=1; Serial.print("\tis Incorrect");}

  value1=33, value2=55;
  spiWriteNM(6, value1);
  spiWriteNM(7, value2);
  
  data= spiReadNM(6);
  Serial.print("\nMINIF=");Serial.print(data);
  if (data!= value1) {error=1; Serial.print("\tis Incorrect");}

  data= spiReadNM(7);
  Serial.print("\nMAXIF=");Serial.print(data);
  if (data!= value2) {error=1; Serial.print("\tis Incorrect MAXIF");}
  
  if (error==0) Serial.print("\n\nPass"); 
  else
  {
    Serial.println("\n\nError during Setup!");
    while(1);
  }

  //
  // Test counting the neurons
  //
  CountNeurons(NMplatform);
  
  // temporary
  // while(1);

  value1=1, value2=65535;
}

void loop()
{
    error=0;    
    spiWriteNM(6,value1);
    spiWriteNM(7,value2);
    data=spiReadNM(6);
    if (data!=value1) error=1;     
    data=spiReadNM(7);
    if (data!=value2) error=2;
    if (error ==0)
    {
      iteration++;
      if (iteration % 100 ==0)
      { 
          Serial.print("\nIteration ");Serial.print(iteration); Serial.println(" Pass");
      }
      value1++; if (value1>65535) value1=0;
      value2--; if (value2<0) value2= 65535;
    }
    else
    {
      if (error==1)
      { 
        Serial.print("\nMinif, expected="); Serial.print(value1);
        Serial.print("\treturned="); Serial.print(data);
      }
      else if (error==2)
      {
        Serial.print("\nMaxif, expected="); Serial.print(value2);
        Serial.print("\treturned="); Serial.print(data);
      }
      Serial.print("\nReset counter of successful RW");
      iteration=0;
    }
}

void CountNeurons(int Platform)
{
  Serial.print("\n\nCounting neurons...");
  int navail = 0;
  int cat=0xFFFF;
  int catL=88;
  error=0;
  spiWriteNM(15, 0);
  spiWriteNM(13, 16);
  spiWriteNM(9, catL);
  spiWriteNM(12, 0);
  while (1) {
    cat = spiReadNM(4);
    if (cat == 0xFFFF)
      break;
    if (cat != catL)
    {
      error=3;
      break;
    }
    navail++;
  }
  spiWriteNM(13, 0);
  spiWriteNM(15, 0);
  Serial.print(navail);
  if (error==0)
  {   
    int ncorrect=0;
    switch(Platform)
    {
      case HW_NEUROSHIELD:
        ncorrect=576;
        break;
      case HW_NEUROSTAMP:
        ncorrect=4032;
        break;
    }     
    if (navail!=ncorrect) Serial.println("\tis Incorrect");
  }
  else
  {
    Serial.println("\n\nError counting due to incorrect category ");Serial.println(cat);
  }
}

int spiReadNM(int reg)
{
  digitalWrite(SPISelectPin, LOW);
  SPI.transfer(1);  // Dummy for ID
  SPI.transfer(1); // Module 1 for access to NM500
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(reg); // MINIF register
  SPI.transfer(0); // length[23-16]
  SPI.transfer(0); // length [15-8]
  SPI.transfer(1); // length [7-0]
  int data = SPI.transfer(0); // Send 0 to push upper data out
  data = (data << 8) + SPI.transfer(0); // Send 0 to push lower data out
  digitalWrite(SPISelectPin, HIGH);
  return(data);
}

void spiWriteNM(int reg, int data)
{
  digitalWrite(SPISelectPin, LOW);
  SPI.transfer(1);  // Dummy for ID
  SPI.transfer(0x81); // Write to NM500
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(reg);
  SPI.transfer(0); // length[23-16]
  SPI.transfer(0); // length[15-8]
  SPI.transfer(1); // length[7-0]
  SPI.transfer((unsigned char)(data >> 8)); // upper data
  SPI.transfer((unsigned char)(data & 0x00FF)); // lower data
  digitalWrite(SPISelectPin, HIGH);
}

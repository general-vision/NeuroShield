//********************************************************************
// NeuroShield_ArduCAM
// Copyrights 2018, General Vision Inc.
//
// Object learning and classification
// using three different features
// with optional novelty detection
//
// Tutorial at  https://www.general-vision.com/techbriefs
//
// WARNING#1
// ------------------------------------------------------------
// If you get an error when uploading this script
// you may have to disconnect the ArduCam Shield for the upload
// 
// WARNING#2
// ------------------------------------------------------------
// Insert the SD card slot of the ArduCAM shield
// Absence of SD card slows down the execution speed
//
// Hardware requirements
// ------------------------------------------------------------
// Arduino board with >90 Kbytes program storage space
// ArduCam Shield V2 with camera module
// Arduino101 or NeuroMem shield board (BrainCard, NeuroShield, NeuroStamp)
//
// Library requirements
// ------------------------------------------------------------
// requires the following libraries
// ArduCAM at https://github.com/ArduCAM/Arduino
// UTFT4ArduCAM_SPI at https://github.com/ArduCAM/Arduino
//
// General Vision's CurieNeurons library for Arduino/Genuino101 or,
// General Vision's NeuroMem library for BrainCard, NeuroShield, NeuroStamp
//
// Specifications
// ------------------------------------------------------------
// Feature/Context #1= subsampling, by averaging internal blocks of 11x11 pixels
// Feature/Context #2= rgb histogram
// Feature/Context #3= composite profile
// Decision rule= positive recognition if 2 out of 3 contexts agree on the classification
// ------------------------------------------------------------

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <UTFT_SPI.h>
#include "memorysaver.h"

//*********************************************************
//
// Choose your NeuroMem network
//
// NM500 on NeuroShield board
#include <NeuroMemAI.h>
NeuroMemAI hNN; // use hNN.connect instead of hNN.begin
//
//or
//
// CurieNeurons on Arduino101 board
//#include <CurieNeurons.h>
//CurieNeurons hNN;
//
//*********************************************************

//
// Access to Camera
//
const int SPI_CS_CAM = 10;
ArduCAM myCAM(OV2640, SPI_CS_CAM);
int fw = 320;
int fh = 240;
uint8_t fifo_burst_line[320 * 2];

//
// Access to the LCD display
//
UTFT myGLCD(SPI_CS_CAM);
String StrL;
String StrR;

//
// Variables for the learning and recognition operations
//
int rw, rh, bw, bh, rleft, rtop; // initialized in setup
int featID=0;
int normalized = 0;

// Highest memory capacipy of the NeuroMem neurons, all chips combined
// This MAX is used to format the knowledge files and ensure compatibility across all chips
#define MAXVECLENGTH 256
int subsampleFeat[MAXVECLENGTH];
int vlen = 0;
int rgbhistoFeat[MAXVECLENGTH];
int vlen2 = 0;
int cprofileFeat[MAXVECLENGTH];
int vlen3 =0;
int prevcat = 0, ncount = 0, catLearn = 1;
int displaylen=6;
#define DEF_MAXIF 0x8000

//
// Options
//
String inputString = "";         // string to hold incoming data (from serial port)
boolean stringComplete = false;  // whether the string is complete

int error = 0;

//-----------------------------------------------------
// Display the legend of the serial commands
//-----------------------------------------------------
void showInstructions()
{
  Serial.print("\n\nType a command + ENTER");
  Serial.print("\n\t h = help");
  Serial.print("\nLearning:");
  Serial.print("\n\t 0 = Select the category to forget");
  Serial.print("\n\t 1 to 9 = Select the category # to learn");
  Serial.print("\n\t Press shutter button to learn the selected category");
  Serial.print("\nProject:");
  Serial.print("\n\t cfg = show configuration");
  Serial.print("\n\t ckn = clear neurons");
  Serial.print("\n\t vkn = view neurons");
  Serial.print("\n\t skn = save to SD card");
  Serial.print("\n\t rkn = restore from SD card");
  Serial.print("\n");
}

void setup()
{
  // reserve bytes for the inputString:
  inputString.reserve(4);

  Wire.begin();
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open before starting the initialization
  
  //
  // initialize SPI:
  //
  pinMode(SPI_CS_CAM, OUTPUT);
  SPI.begin();
  Serial.println("Welcome to the NeuroMem_Smart ArduCAM demo!");
  //
  // Check if the ArduCAM SPI bus is OK
  //
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  uint8_t temp = myCAM.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.println("SPI interface Error!");
    Serial.println("Check your wiring, make sure using the correct SPI port and chipselect pin");
  }
  //
  // Initialize the TFT LCD
  //
  myGLCD.InitLCD();
  extern uint8_t BigFont[];
  myGLCD.setFont(BigFont);
  myGLCD.setColor(255, 0, 225);
  //
  // Initialize the ArduCAM
  //
  myCAM.InitCAM();
  //
  // Check if the camera module type is OV2640
  //
  uint8_t vid = 0, pid = 0;
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42))) {
    Serial.println("Can't find OV2640 module!");
  } else {
    Serial.println("OV2640 detected.");
  }  
  //
  // Initialize the neurons and project settings
  // (platform input is discarded if using CurieNeurons library)
  //
  if (hNN.begin(2) != 0) //platform  1= braincard, 2= neuroshield, 3= neurostamp 
  {
    Serial.print("\nNeuroMem board NOT found! Check your connection.\n");
    while (1);
  }
  else
  {
    Serial.print("\nNeuroMem neurons detected! ");
    Serial.print("\nNeurons available = "); Serial.print(hNN.navail); Serial.print(" neurons\n");
    Serial.print("Setting default values");
    hNN.setContext(1, 2, DEF_MAXIF);
    bw = 12, bh = 12;
    rw = bw * 10, rh = bh * 10;
    rleft = (fw - rw) / 2;
    rtop = (fh - rh) / 2;
  }
  showInstructions();
  Serial.println("\nClick the shutter button to learn\n");
}

void loop()
{
    if (stringComplete)
    {
      processSerialEvent();
      stringComplete = false;
    }

    if (!myCAM.get_bit(ARDUCHIP_TRIG, VSYNC_MASK)) 
    {
      myCAM.set_mode(MCU2LCD_MODE);
      myGLCD.resetXY();
      myCAM.set_mode(CAM2LCD_MODE);
      while (!myCAM.get_bit(ARDUCHIP_TRIG, VSYNC_MASK)); 
      recognize(rleft, rtop);
    } 
    else if (myCAM.get_bit(ARDUCHIP_TRIG, SHUTTER_MASK))
    {         
      while (myCAM.get_bit(ARDUCHIP_TRIG, SHUTTER_MASK))
      {     
       Serial.print("\n\nLearning Cat "); Serial.print(catLearn);   
       learn(rleft, rtop, catLearn);
      }
    }
}

//-----------------------------------------------------
// Detect a serial command
//-----------------------------------------------------
void serialEvent() {
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') stringComplete = true;
  }
}


//-----------------------------------------------------
// Extract three feature vectors from the ROI position
// (Left, Top) in the last frame
// No option to normalize the feature vectors in this example
//-----------------------------------------------------
void getFeatureVectors(int Left, int Top) 
{
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  myCAM.read_fifo(); // Read the first dummy byte
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  
  // Read captured image as BMP565 format (320x240x 2 bytes from FIFO)
  // extract the three features on the fly
  int hb = rw / bw, vb = rh / bh;
  vlen= hb*vb;
  vlen2=MAXVECLENGTH;
  int profdiv=1; // increment if (rw + rh > 256)
  int hb3= rw / profdiv, vb3= rh / profdiv;
  vlen3= hb3+vb3;
  
  char VH, VL;
  int indexFeat1X, indexFeat1Y, indexFeat3X, indexFeat3Y, index = 0, color;
  uint8_t r, g, b, greylevel;
  for (int i = 0; i < MAXVECLENGTH; i++) subsampleFeat[i] = 0; 
  for (int i = 0; i < MAXVECLENGTH; i++) rgbhistoFeat[i] = 0; 
  for (int i = 0; i < MAXVECLENGTH; i++) cprofileFeat[i] = 0; 
  
  for (int y = 0 ; y < fh ; y++)
  {
    SPI.transfer(fifo_burst_line, fw*2);//read one line from spi 
    for (int x = 0 ; x < fw ; x++)
    {
      VH = fifo_burst_line[x * 2];
      VL = fifo_burst_line[(x * 2) + 1];
      color = (VH << 8) + VL;

      if ((y >= Top) && (y < Top + rh))
      {       
        indexFeat1Y= (y - Top) / bh;
        indexFeat3Y = (y - Top) / profdiv;
        if (x >= Left && x < Left + rw)
        {             
          r = ((color >> 11) & 0x1F);
          g = ((color >> 6) & 0x1F);
          b = (color & 0x001F);
          greylevel = (r + g + b) << 1;

          rgbhistoFeat[r]++;
          rgbhistoFeat[g + 85]++;
          rgbhistoFeat[b + 170]++;
          
          indexFeat1X= (x - Left) / bw;
          indexFeat3X = (x - Left) / profdiv;
          if ((indexFeat1X < hb) & (indexFeat1Y < vb))
          {
              index = (indexFeat1Y * hb) + indexFeat1X;
              subsampleFeat[index] = subsampleFeat[index] + greylevel;
          }
          if ((indexFeat3X < hb3) & (indexFeat3Y < vb3))
          {
              cprofileFeat[indexFeat3X] = cprofileFeat[indexFeat3X] + greylevel;
              cprofileFeat[indexFeat3Y + hb3] = cprofileFeat[indexFeat3Y + hb3] + greylevel;
          }
        }
      }
    }
  }

  myCAM.CS_HIGH();
    
  for (int i = 0; i < vlen; i++) subsampleFeat[i] = subsampleFeat[i] / (bw * bh);
  for (int i = 0; i < vlen2; i++) rgbhistoFeat[i] = rgbhistoFeat[i] * 255 / (rw * rh);
  for (int i = 0; i < hb3; i++) cprofileFeat[i] = cprofileFeat[i] / hb3;
  for (int i = 0; i < vb3; i++) cprofileFeat[i + hb3] = cprofileFeat[i + hb3] / vb3;
}
//--------------------------------------------------------
// Recognize the feature vector  and display result on LCD
//--------------------------------------------------------
void recognize(int Left, int Top)
{
  int dist = 0xFFFF, cat=0xFFFF, cat1 = 0, cat2=0, cat3=0, nid = 0; 
  char tmpStr[10];
  char StrR[40] = {""};
  getFeatureVectors(Left, Top);
  // recognize feature vector #1 or subsample vector
  hNN.GCR(1);
  hNN.classify(subsampleFeat, vlen, &dist, &cat1, &nid);
  // recognize feature vector #2 or histogram rgb
  hNN.GCR(2);
  hNN.classify(rgbhistoFeat, vlen2, &dist, &cat2, &nid);
  // recognize feature vector #3 or composite profile
  hNN.GCR(3);
  hNN.classify(cprofileFeat, vlen3, &dist, &cat3, &nid);
  
  int matches=0;
  if ((cat1==cat2) & (cat1==cat3)) { cat=cat1; matches=3;}
  else if ((cat1==cat2) | (cat1==cat3)) {cat=cat1; matches=2;}
  else if (cat2==cat3) { cat=cat2; matches=2;}
  else cat=0xFFFF;

  if (cat != 0xFFFF)
  {
    strcat(StrR,"Cat=");
    itoa (cat & 0x7FFF, tmpStr, 10); // mask the degenerated flag (bit 15 of CAT register)
    strcat(StrR, tmpStr);
  }
  else
  {
    strcat(StrR,"Unknown");
  }
  displayLCD_res(StrR, 5,5);
  if (cat != prevcat)
  {
    Serial.println(StrR);
    prevcat = cat;
  }
}

//-------------------------------------------------------------------------
// Learn the ROI
//-------------------------------------------------------------------------
void learn(int Left, int Top, int Category)
{
  char tmpStr[10];
  char StrL[40] = {""};
  Serial.print("\n\nLearning object category "); Serial.println(Category);
  getFeatureVectors(Left, Top);

  // learn feature vector #1 or subsample vector  
  hNN.GCR(1);
  hNN.learn(subsampleFeat, vlen, Category);
  // learn feature vector #2 or histogram rgb
  hNN.GCR(2);
  hNN.learn(rgbhistoFeat, vlen2, Category);
  // learn feature vector #3 or composite profile
  hNN.GCR(3);  
  int ncount = hNN.learn(cprofileFeat, vlen3, Category);
  
  strcat(StrL,"Neurons=");
  itoa (ncount, tmpStr, 10);
  strcat(StrL, tmpStr);
  Serial.println(StrL);
  displayLCD_res(StrL, 5, 220);
}

//-----------------------------------------------------
// Display text and outline on LCD
//-----------------------------------------------------
void displayLCD_res(char* Str, int x, int y)
{
  myCAM.set_mode(MCU2LCD_MODE);
  //myGLCD.setColor(255, 0, 225);
  myGLCD.print(Str,x,y,0);
  //myGLCD.setColor(255, 0, 225);
  myGLCD.drawRect(rleft, rtop, rleft+rw, rtop+rh);
  delay(50); // to sustain the display
}

//--------------------------------------------------------
// Process the command received on serial port
//--------------------------------------------------------
void processSerialEvent()
{
  if (inputString == "h\n") // ********* Help
  {
    showInstructions();
  }
  else if (inputString == "cfg\n") // ********** View configuration
  {
    showSettings();
  }
  else if (inputString == "ckn\n") // ********* Clear knowledge
  {
    hNN.forget(DEF_MAXIF);
    catLearn = 1;
    Serial.print("\n\nKnowledge cleared");
  }
  else if (inputString == "vkn\n")  // ********* View knowledge
  {
    displayNeurons(displaylen);
  }
  else if (inputString == "skn\n")  // ********* Save project
  {
    char filename[10] = {"User.knf"};
    error = hNN.saveKnowledge_SDcard(filename); // format compatible with CogniPat SDK and neuroMem KB
    switch (error)
    {
      case 1: Serial.println("\nSD card not found\n"); break;
      case 2: Serial.println("\nCannot open file\n"); break;
      default: Serial.print("\nProject saved\n"); break;
    }
  }
  else if (inputString == "rkn\n")  // ********* Restore project
  {
    char filename[12] = {"User.knf"}; // format compatible with CogniPat SDK and neuroMem KB
    error = hNN.loadKnowledge_SDcard(filename);
    switch (error)
    {
      case 1: Serial.println("\nSD card not found"); break;
      case 2: Serial.println("\nCannot open file"); break;
      default: Serial.print("\nProject restored"); catLearn = 1; break;
    }
  }
  else  // ********* Learning
  {
    int tmp = inputString.toInt();
    if ((tmp >= 0) & ( tmp <= 9))
    {
      catLearn=tmp;
      Serial.print("\nNext category to learn ="); Serial.println(catLearn);
    }
  }
  inputString = "";
}

//-----------------------------------------------------
// Display the content of the committed neurons
//-----------------------------------------------------
void displayNeurons(int displayLength)
{
  int ncr, cat, minif, aif, ncount;
  int model[hNN.NEURONSIZE];
  ncount = hNN.NCOUNT();
  if (ncount == 0xFFFF) ncount = hNN.navail;
  Serial.print("\n\nNeurons content, count="); Serial.print(ncount);
  for (int i = 0; i < ncount; i++)
  {
    hNN.readNeuron(i, model, &ncr, &minif, &aif, &cat);
    if (ncr == 1) Serial.print("\nModel"); else Serial.print("\nAnomaly");
    for (int k = 0; k < displayLength; k++)
    {
      Serial.print("\t"); Serial.print(model[k]);
    }
    Serial.print("...");
    Serial.print("\taif="); Serial.print(aif);
    Serial.print("\tcat="); Serial.print(cat & 0x7FFF);
  }
  Serial.println("");
}

//-----------------------------------------------------
// Display the project settings on serial monitor
//-----------------------------------------------------
void showSettings()
{
  Serial.print("\n\nProject Settings");
  Serial.print("\nROI ["); Serial.print(rw); Serial.print(" , "); Serial.print(rh);Serial.print("]");
  Serial.print("\nCommitted neurons = "); Serial.print(hNN.NCOUNT());
  Serial.print("\nLength Feat1 = "); Serial.print(vlen);
  Serial.print("\nLength Feat2 = "); Serial.print(vlen2);
  Serial.print("\nLength Feat3 = "); Serial.print(vlen3);
}

//********************************************************************
// NeuroMem_ArduCAM
// Copyrights 2018, General Vision Inc.
//
// Object learning and classification
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
// ------------------------------------------------------------

#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <SD.h> // used in saveImage function
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
//String StrL[10];
//String StrR;

//
// Variables for the learning and recognition operations
//
int rw, rh, bw, bh, rleft, rtop; // initialized in setup
int vlen = 0;
int featID=0;
int normalized = 0;

// max length of models saved in the family of NeuroMem chips,
// used to format the knowledge and project files across all platforms
#define MAXVECLENGTH 256
#define DEF_MAXIF 0x8000
int subsample[MAXVECLENGTH]; // buffer to calculate feature vector
int vector[MAXVECLENGTH]; // vector for the neurons; integer values but mapped between [0-256] for the NM500

int dist = 0, cat = 0, nid = 0, prevcat = 0, ncount = 0;
int catLearn = 1;
int displaylen=6;
int fileIndex=0;
//
// Options
//
bool catIncrement= 0;
bool noveltyFlag = false; // to learn novelties under a context #2
bool counterExample = false; // to automatically learn background examples around the learned location
bool adjacentExample = true; // to automatically learn conservative adjacent examples around the learned location
bool demo= false;
char demoNames[4][10]{"cheetah", "lion", "buffalo", "elephant"};

String inputString = "";         // string to hold incoming data (from serial port)
bool stringComplete = false;  // whether the string is complete

bool MicDetected=false;

int error = 0;

//
// Access to Text to Speech Parallax
//
// include the SoftwareSerial library so we can use it to talk to the Emic 2 module
#include <SoftwareSerial.h>
//#define rxPin   12  // Serial input (connects to Emic 2's SOUT pin)
//#define txPin   11  // Serial output (connects to Emic 2's SIN pin)
#define rxPin   5  // Serial input (connects to Emic 2's SOUT pin)
#define txPin   4  // Serial output (connects to Emic 2's SIN pin)
SoftwareSerial emicSerial =  SoftwareSerial(rxPin, txPin);

//-----------------------------------------------------
// Display the legend of the serial commands
//-----------------------------------------------------
void showInstructions()
{
  Serial.print("\n\nType a command + ENTER");
  Serial.print("\n\t h = help");
  Serial.print("\n\t dmo = demo on/off");
  Serial.print("\n\t lc = learn constant category"); 
  Serial.print("\n\t li = learn & auto_increment category"); 
  //Serial.print("\n\t r = recognize");
  //Serial.print("\n\t rn = recognize + record novelty");
  Serial.print("\nLearning:");
  Serial.print("\n\t 0 = Select the category to forget");
  Serial.print("\n\t 1 to 9 = Select the category # to learn");
  Serial.print("\n\t Press shutter button to learn the selected category");
  Serial.print("\n\t abe = automatic background examples");
  Serial.print("\n\t aae = automatic adjacent examples");
  Serial.print("\n\t nae = no automatic examples");
  //Serial.print("\n\t + = Increase region of interest");
  //Serial.print("\n\t - = Decrease region of interest");
  //Serial.print("\n\t v = show vector");
  Serial.print("\nProject:");
  Serial.print("\n\t cfg = show configuration");
  Serial.print("\n\t ckn = clear neurons");
  Serial.print("\n\t vkn = view neurons");
  Serial.print("\n\t skn = save to SD card");
  Serial.print("\n\t rkn = restore from SD card");
  Serial.print("\nVideo:");
  Serial.print("\n\t sim = save image to SD card");
  Serial.print("\n");
}

void setup()
{
  // reserve bytes for the inputString:
  inputString.reserve(4);

  Wire.begin();
  Serial.begin(115200); // initialize Serial communication
  //while (!Serial);    // wait for the serial port to open before starting the initialization
  
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
  // Initialize Parallax Text2Speech
  //
  int t0=millis();
  int t=0;
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  emicSerial.begin(9600);
  emicSerial.print('\n');             // Send a CR in case the system is already up
  // When the Emic 2 has initialized and is ready, it will send a single ':' character, so wait here until we receive it
  while ((emicSerial.read() != ':') & (t<1000))
  {
    t=millis() - t0;
    //Serial.print("\nt_speaker="); Serial.print(t);
  }
  if (t<1000) 
  {
    MicDetected=true;
    delay(10);                          // Short delay
    emicSerial.flush();                 // Flush the receive buffer
    emicSerial.print("N8\n");            // Select Voice (0 to 8)
    emicSerial.print("V15\n");           // Set Volume (-48 to 18)
    emicSerial.print("L0\n");            // Set Language (0 to 2)
    emicSerial.print("P1\n");            // Select Parser (0 DECtalk, 1 Epson)
    emicSerial.print("W200\n");          // Set Speaking Rate (75 to 600)
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
    Serial.print("\nNeurons available = "); Serial.print(hNN.navail); Serial.print(" neurons");
    Serial.print("\nNeurons memory = "); Serial.print(hNN.NEURONSIZE); Serial.print(" bytes\n");
    char filename[12] = {"Default.csp"};
    // csp = format compatible with CogniSight SDK and Image KB
    error = hNN.loadProject_SDcard(filename, &rw, &rh, &bw, &bh, &featID, &normalized);
    if (error == 0)
    {
      Serial.print("Project loaded from SD card");
      hNN.NSR(16);
      hNN.RESETCHAIN();
      for (int i=0; i<hNN.NCOUNT(); i++)
      {
        cat=hNN.CAT();
        if (cat > catLearn) catLearn=cat; 
      }
      hNN.NSR(0);
      catLearn= catLearn+catIncrement;
    }
    else
    {
      hNN.setContext(1, 2, DEF_MAXIF);
      bw = 12, bh = 12;
      rw = bw * 10, rh = bh * 10;
    }
    rleft = (fw - rw) / 2;
    rtop = (fh - rh) / 2;
  }
  showSettings();
  showInstructions();
  Serial.println("\nClick the shutter button to learn\n");
  if (MicDetected==true)
  {
    emicSerial.print('S');
    emicSerial.print("Ready to learn and recognize");
    emicSerial.print('\n');
    while (emicSerial.read() != ':');
  }
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
      int t0 = millis();        
      while (myCAM.get_bit(ARDUCHIP_TRIG, SHUTTER_MASK))
      {
       int t= millis()-t0;
       //Serial.print("\nlapse="); Serial.print(t); 
       if (t < 200)
       {  
         Serial.print("\n\nLearning Cat "); Serial.print(catLearn);   
         learn(rleft, rtop, catLearn);
         catLearn=catLearn+1;
       }
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
// Grab a new frame and extract the feature vector of
// the ROI positioned at (Left, Top)
//-----------------------------------------------------
void getSubsample(int Left, int Top)
{
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  myCAM.read_fifo(); // Read the first dummy byte
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  //Read captured image as BMP565 format (320x240x 2 bytes from FIFO)
  int indexFeatX, indexFeatY, index = 0;
  int hb = rw / bw, vb = rh / bh;
  vlen= hb*vb;
  char VH, VL;
  int color = 0, r = 0, g = 0, b = 0, greylevel = 0;

  //int maxR=0, maxG=0, maxB=0;
  //int minR=0xFFFF, minG=0xFFFF, minB=0xFFFF;
  for (int i = 0; i < MAXVECLENGTH; i++) subsample[i] = 0;
  for (int y = 0 ; y < fh ; y++)
  {
    SPI.transfer(fifo_burst_line, fw * 2);
    for (int x = 0 ; x < fw ; x++)
    {
      VH = fifo_burst_line[x * 2];
      VL = fifo_burst_line[(x * 2) + 1];
      //
      // detect if the pixel belongs to the region of interest [rleft, rtop, rw, rh]
      //
      if ((y >= Top) && (y < Top + rh))
      {
        indexFeatY = (y - Top) / bh;
        if ((x >= Left) && (x < Left + rw))
        {
          color = (VH << 8) + VL;
          r = ((color >> 11) & 0x1F); //if (r>maxR) maxR=r; if (r<minR) minR=r;
          g = ((color >> 6) & 0x1F); //if (g>maxG) maxG=g;  if (g<minG) minG=g;
          b = (color & 0x001F); //if (b>maxB) maxB=b;  if (b<minB) minB=b;
          greylevel = (r + g + b) << 1;
//          r = ((color >> 11) & 0x1F);
//          g = ((color >> 5) & 0x3F);
//          b = (color & 0x001F);
//          greylevel= r+g+b;
          indexFeatX = hb -1 - ((x - Left) / bw);
          if ((indexFeatX < hb) & (indexFeatY < vb))
          {
            index = (indexFeatY * hb) + indexFeatX;
            subsample[index] = subsample[index] + greylevel;
          }
        }        
      }
    }
  }
  myCAM.CS_HIGH();

  int maxgrey = 0x0000, mingrey = 0xFFFF;
  for (int i = 0; i < vlen; i++)
  {
    vector[i] = subsample[i] / (bw * bh);
    if (vector[i] > maxgrey) maxgrey = vector[i];
    else if (vector[i] < mingrey) mingrey = vector[i];
  }
  if (normalized == 1)
  {
    if (mingrey < maxgrey)
    {
      for (int i = 0; i < vlen; i++)
      {
        vector[i] = ((vector[i] - mingrey)* 255) / (maxgrey - mingrey) ;
      }
    }
  }
//  Serial.print("\nMaxR= "); Serial.print(maxR);
//  Serial.print("\tMaxG= "); Serial.print(maxG);
//  Serial.print("\tMaxB= "); Serial.print(maxB);
//  Serial.print("\nMinR= "); Serial.print(minR);
//  Serial.print("\tMinG= "); Serial.print(minG);
//  Serial.print("\tMinB= "); Serial.print(minB);
//  Serial.print("\nMaxGrey= "); Serial.print(maxgrey);
//  Serial.print("\tMinGrey= "); Serial.print(mingrey);
}

//--------------------------------------------------------
// Recognize the feature vector  and display result on LCD
//--------------------------------------------------------
void recognize(int Left, int Top)
{
  char tmpStr[14];
  char StrR[14] = {""};
  getSubsample(Left, Top);
  hNN.classify(vector, vlen, &dist, &cat, &nid);
  if (dist != 0xFFFF)
  {
    if (demo==true)
    {
      strcat(StrR,demoNames[cat-1]);
    }
    else
    {
      strcat(StrR,"Cat ");
      itoa (cat & 0x7FFF, tmpStr, 10); // mask the degenerated flag (bit 15 of CAT register)
      strcat(StrR, tmpStr);
    }
  }
  else
  {
    strcat(StrR,"Unknown");
    if (noveltyFlag == true)
    {
      strcat(StrR," but learning novelty");
      hNN.setContext(2, 2, DEF_MAXIF/2);  // Set Context 2 and small maxif to learn min novelty events
      ncount = hNN.learn(vector, vlen, 1);
      hNN.setContext(1, 2, DEF_MAXIF); // Restore Context 1
    }
  }
  displayLCD_res(StrR, 5,5);
  if (cat != prevcat)
  {
    prevcat = cat;
    Serial.println(StrR);
    if ((MicDetected==true) & (cat !=65535))
    {
      emicSerial.print('S');
      emicSerial.print(StrR);  // Send the desired string to convert to speech
      emicSerial.print('\n');
      while (emicSerial.read() != ':');
    }
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
  if (MicDetected==true)
  {
    emicSerial.print('S');
    emicSerial.print("learning category");
    emicSerial.print(Category);
    emicSerial.print('\n');
    while (emicSerial.read() != ':');
  }
  getSubsample(Left, Top);
  hNN.learn(vector, vlen, Category);
  if (Category !=0)
  {
    if (counterExample == true)
    {
      Serial.println(" with automatic background examples");
      getSubsample(Left - rw / 2, Top - rh / 2) ;
      hNN.learn(vector, vlen, 0);
      getSubsample(Left + rw / 2, Top + rh / 2) ;
      hNN.learn(vector, vlen, 0);
    }
    else if (adjacentExample == true)
    {
      int divi= 4;
      Serial.println(" with automatic adjacent examples");
      getSubsample(Left - rw / divi, Top - rh / divi) ;
      hNN.learn(vector, vlen, 0);
      hNN.learn(vector, vlen, Category);
      getSubsample(Left + rw / divi, Top + rh / divi) ;
      hNN.learn(vector, vlen, 0);
      hNN.learn(vector, vlen, Category);
      getSubsample(Left - rw / divi, Top + rh / divi) ;
      hNN.learn(vector, vlen, 0);
      hNN.learn(vector, vlen, Category);
      getSubsample(Left + rw / divi, Top - rh / divi) ;
      hNN.learn(vector, vlen, 0);
      hNN.learn(vector, vlen, Category);
    }
  }
  ncount = hNN.NCOUNT();
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
  if (inputString == "dmo\n") // ********* demo
  {
    if (demo==true) demo=false;
    else demo=true;
  }
  else if (inputString == "lc\n") // ********* Constant category until editied on screen
  {
    catIncrement=0;
    Serial.println("\n\nLearning with fixed category");
  }
  else if (inputString == "li\n") // ********* Category auto-incremented
  {
    catIncrement=1;
    Serial.println("\n\nLearning with category increment");
  }
  else if (inputString == "r\n") // ********* Simple recognition
  {
    noveltyFlag = false;
    Serial.println("\n\nRecognition without novelty recording");
  }
  else if (inputString == "rn\n") // ********* Recognition with Novelty Recording
  {
    noveltyFlag = true;
    Serial.println("\n\nRecognition with novelty recording");
  }
  else if (inputString == "abe\n") // ********* Automatic background examples around learned object
  {
    counterExample = true;
    adjacentExample = false;
    Serial.println("\n\nLearning background examples around learned object automatically");
  }
  else if (inputString == "aae\n") // ********* Automatic adjacent examples around learned object
  {
    adjacentExample = true;
    counterExample = false;
    Serial.println("\n\nLearning background examples around learned object automatically");
  }
  else if (inputString == "nae\n") // ********* No automatic examples around learned object
  {
    counterExample = false;
    adjacentExample = false;
    Serial.println("\n\nLearning object with no automatic background examples");
  }
  else if (inputString == "-\n") // ********* Decrease ROI
  {
    int hb = rw / bw, vb = rh / bh;
    bw = bw - 1; bh = bh - 1;
    rw = bw * hb; rh = bh * vb;
    rleft = (fw - rw) / 2; rtop = (fh - rh) / 2;
    Serial.print("\n\nDecrease region of interest");
  }
  else if (inputString == "+\n") // ********* Increase ROI
  {
    int hb = rw / bw, vb = rh / bh;
    bw = bw + 1; bh = bh + 1;
    rw = bw * hb; rh = bh * vb;
    rleft = (fw - rw) / 2; rtop = (fh - rh) / 2;
    Serial.print("\n\nIncrease region of interest");
  }
  else if (inputString == "v\n") // ********* Vector display
  {
    Serial.print("\n\nCurrent vector");
    getSubsample(rleft, rtop);
    for (int i = 0; i < 256; i++) 
    //for (int i = 0; i < displaylen; i++) 
    {
      Serial.print("\t");
      Serial.print(vector[i]);
    }
    Serial.print("...");
  }
  else if (inputString == "cfg\n") // ********** View configuration
  {
    showSettings();
  }
  else if (inputString == "ckn\n") // ********* Clear knowledge
  {
    hNN.forget(DEF_MAXIF);
    catLearn = 1;
    Serial.print("\n\nKnowledge cleared\n");
  }
  else if (inputString == "vkn\n")  // ********* View knowledge
  {
    displayNeurons(displaylen);
  }
  else if (inputString == "skn\n")  // ********* Save project
  {
    char filename[10] = {"User.csp"};
    // csp = format compatible with CogniSight SDK and Image KB
    error = hNN.saveProject_SDcard(filename, rw, rh, bw, bh, featID,normalized);
    switch (error)
    {
      case 1: Serial.println("\nSD card not found\n"); break;
      case 2: Serial.println("\nCannot open file\n"); break;
      default: Serial.print("\nProject saved\n"); break;
    }
  }
  else if (inputString == "rkn\n")  // ********* Restore project
  {
    char filename[12] = {"User.csp"};
    // csp = format compatible with CogniSight SDK and Image KB
    error = hNN.loadProject_SDcard(filename, &rw, &rh, &bw, &bh, &featID, &normalized);
    switch (error)
    {
      case 1: Serial.println("\nSD card not found"); break;
      case 2: Serial.println("\nCannot open file"); break;
      default:
        Serial.print("\nProject restored");
        rleft = (fw - rw) / 2; rtop = (fh - rh) / 2;
        catLearn = 1;
        showSettings(); break;
    }
  }
  else if (inputString == "sim\n")  // ********* Save image
  {
    error=saveImage(fileIndex);
    switch (error)
    {
      case 1: Serial.println("\nSD card not found\n"); break;
      case 2: Serial.println("\nCannot open file\n"); break;
      default: Serial.print("\nImage saved\n"); break;
    }
    fileIndex=fileIndex+1;
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
    for (int k = 0; k < hNN.NEURONSIZE; k++)
    //for (int k = 0; k < displayLength; k++)
    {
      Serial.print("\t"); Serial.print(model[k]);
    }
    Serial.print("...");
    Serial.print("\taif="); Serial.print(aif);
    Serial.print("\tcat="); Serial.print(cat & 0x7FFF);
  }
  Serial.println("\n");
}

//-----------------------------------------------------
// Display the project settings on serial monitor
//-----------------------------------------------------
void showSettings()
{
  Serial.print("\n\nProject Settings");
  Serial.print("\nROI ["); Serial.print(rw); Serial.print(" , "); Serial.print(rh);Serial.print("]");
  Serial.print(" with internal block ["); Serial.print(bw); Serial.print(" , "); Serial.print(bh);Serial.print("]");
  Serial.print("\nFeatID = "); Serial.print(featID);Serial.print("\tnormalized = "); Serial.print(normalized);

  Serial.print("\nCommitted neurons = "); Serial.print(hNN.NCOUNT());
  //Serial.print("\nMaxif = "); Serial.print(hNN.MAXIF());
  //Serial.print(", \tMinif = "); Serial.print(hNN.MINIF());
  //Serial.print(", \tGCR = "); Serial.print(hNN.GCR());
  
  Serial.print("\nNext category to learn = "); Serial.print(catLearn);
  Serial.print("\nAutomatic category increment = "); Serial.print(catIncrement);
  Serial.print("\nAutomatic counter example = "); Serial.print(counterExample);
  Serial.print("\nAutomatic adjacent example = "); Serial.print(adjacentExample);
  //Serial.print("\nNovelty recording = "); Serial.print(noveltyFlag);

  Serial.print("\nSpeaker = "); Serial.print(MicDetected);
  Serial.println("\n");
}


//-----------------------------------------------------
// Save image
//-----------------------------------------------------
int saveImage(int fileIndex)
{
 //Construct a file name
 char imgFilename[20]="img";
 char strtmp[10];
 itoa(fileIndex, strtmp, 10);
 strcat(imgFilename, strtmp);
 strcat(imgFilename, ".dat");
 
  int SD_detected=SD.begin(9); // SD from ArduCamShield
  if (!SD_detected) return(1);
  if (SD.exists(imgFilename)) SD.remove(imgFilename);
  File SDfile = SD.open(imgFilename, FILE_WRITE);
  if(! SDfile)return(2);

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  
  myCAM.read_fifo(); // Read the first dummy byte
  
  byte pixeline[320]; // fw=320
  char VH, VL;
  uint32_t color;
  int r, g, b, greylevel;
  for (int y = 0 ; y < fh ; y++)
  {
    for (int x = 0 ; x < fw ; x++)
    {
      VH = myCAM.read_fifo();
      VL = myCAM.read_fifo();
      color = (VH << 8) + VL;            
      r = ((color >> 11) & 0x1F); //if (r>maxR) maxR=r; if (r<minR) minR=r;
      g = ((color >> 6) & 0x1F); //if (g>maxG) maxG=g;  if (g<minG) minG=g;
      b = (color & 0x001F); //if (b>maxB) maxB=b;  if (b<minB) minB=b;
      greylevel = (r + g + b) << 1;
      //      r = ((color >> 11) & 0x1F);
      //      g = ((color >> 5) & 0x3F);
      //      b = (color & 0x001F);
      //      greylevel= r+g+b;
      pixeline[x] = (byte)(greylevel);        
    }
    SDfile.write(pixeline, fw);
  }
  SDfile.close();
  return(0);
 }

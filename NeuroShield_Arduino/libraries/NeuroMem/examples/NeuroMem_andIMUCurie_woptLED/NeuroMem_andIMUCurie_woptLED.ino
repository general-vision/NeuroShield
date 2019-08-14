//------------------------------------------------------------------------
// NeuroMem_andCurieIMU_woptLED
//
// Copyright 2018 General Vision Inc.
//
// Updated 02/13/2018
// Support BrainCard and NeuroShield
//
//  Motion learning and recognition using the Arduino 101 and a NeuroMem shield
//  The script uses the Bosch accel/gyro of the Curie module and the neurons of the NeuroMem shield
//
// Tutorial at https://www.general-vision.com/techbriefs/TB_NeuroMemArduino_IMUDemo.pdf
//
// commands are expected through the serial port as follows
//  ts = transmit sensor data (can be viewed under the serial monitor or serial plotter)
//  1 to 9 = start learning motion type 1 to 9
//  0 = correct false recognition until stop
//  r = recognize
//  s = stop any sustained operation like learning, recognition, transmission
//  v = show vector
//  n = show neurons
//  f = clear the knowledge
//
// IMPORTANT REMARK
//
// This example is very academic and assemble a simple sequence pattern which should be more
// sophisticated to address a real-life problem such as real-time sampling rate and calibration
// adequate for the type of motion being studied.
// More advanced feature extractions can include waveform profiles, distribution of peaks and zero crossing, etc.
//  
//-----------------------------------------------------------------------------------------

// Curie sensors
#include "CurieIMU.h"

// NeuroMem platforms
#include <NeuroMemAI.h>
NeuroMemAI hNN;

int* vector=0;
int prevncount=0, prevcat=0, vlen=0; // buffers for previous values
int dist, cat, nid, nsr, ncount; // response from the neurons
//
// Optional LED outputs to report the motion category 1 and 2
//
#define LED_RED 2
#define LED_GREEN 3

String inputString = "";         // a string to hold incoming data (from serial port)
boolean stringComplete = false;  // whether the string is complete

int VAL=0;
int sampleNbr=20; // number of samples appended into a feature vector
int channelNbr=6; // 3 for ax,ay,az; 6 for ax,ay,az,gx,gy,gz
float ax, ay, az;
float gx, gy, gz;  

bool learningFlag=false;
bool recoFlag=false;
bool transmitSensorDataFlag=false;
int minIterations=5; // minimum number of continuous examples to learn when the learning is not sustained
int iterationID=0;

#define DEF_MAXIF 0x4000 // default Maximum Influence Field of the neurons

// The collection of a new sensor dataset is sampled at a rate relevant for the type
// of motion under study
// This lapse global is used in the getSensorData function
int lapse=0;
unsigned long millisPrevious=0;
unsigned long millisPerReading = 1000 / 25;
  
void setup()
{
  // reserve 3 bytes for the inputString:
  inputString.reserve(3);
  
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open

  // initialize device
  Serial.println("Initializing IMU device...");
  CurieIMU.begin();

  Serial.println("Calibrating... Make sure your board is stable and flat until Done");
  delay(5000);
  CurieIMU.autoCalibrateGyroOffset();
  CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
  CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);
  Serial.println(" Done");
  
  //Set sample rate of the acelerometer and the gyro and the filter to 25Hz
  CurieIMU.setGyroRate(25);
  CurieIMU.setAccelerometerRate(25);
  
  // Set the accelerometer range to 2G
  CurieIMU.setAccelerometerRange(2);
  // Set the gyroscope range to 250 degrees/second
  CurieIMU.setGyroRange(250);
  
  //
  // Initialize the neurons and set a conservative Max Influence Field
  //
  int NMplatform=0;
  Serial.print("\nSelect your NeuroMem platform:\n\t1 - BrainCard\n\t2 - NeuroShield\n");
  while(Serial.available() <1);
  NMplatform = Serial.read() - 48;      
  if (hNN.begin(NMplatform) == 0) 
  {
    Serial.print("\nYour NeuroMem_Smart device is initialized! ");
    Serial.print("\nThere are "); Serial.print(hNN.navail); Serial.print(" neurons\n");    
    hNN.forget(DEF_MAXIF);
    Serial.print("\nInitial Max Influence Field = "); Serial.print(DEF_MAXIF);    
  }
  else 
  {
    Serial.print("\nYour NeuroMem_Smart device is NOT found!");
    Serial.print("\nCheck your device type and connection.\n");
    while (1);
  }

  if (sampleNbr*channelNbr > hNN.NEURONSIZE)
  {
    sampleNbr = hNN.NEURONSIZE / channelNbr;
  }
  vector= new int[sampleNbr*channelNbr];
  
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);  
  
  showInstructions();
}

void loop() 
{   
  if (stringComplete) 
  {
    if (inputString == "s\n") // report the ending of the current sustained command
    {
      if (learningFlag==true)
      {
          Serial.print("\n\nLearning stopped. \tNeurons="); Serial.println(ncount);
      }
      else if (recoFlag==true)
      {
          Serial.print("\n\nRecognition stopped.");  
      }
      else if (transmitSensorDataFlag==true)
      {
          Serial.print("\n\nSensor Data Transmission stopped.");
      }
      else Serial.print("\n\nStopping current sustained operation");
      learningFlag=false;
      recoFlag=false;
      transmitSensorDataFlag=false;   
      showInstructions();
    }
    else
    {
      //
      // decode new command other than "s" for stop
      //
      learningFlag=false;
      recoFlag=false;
      transmitSensorDataFlag=false;
      if (inputString == "f\n") // ********* Forget All
      {
        hNN.forget(DEF_MAXIF);
        Serial.print("\n\nKnowledge cleared");
        showInstructions();
      }
      else if (inputString == "v\n") // ********* Vector display
      {
        Serial.print("\n\nCurrent vector");
        vlen=extractFeatureVector();
        int compNbr=channelNbr*sampleNbr;
        compNbr=channelNbr; // override for easier display on serial monitor
        for (int i=0; i<compNbr; i++) { Serial.print("\t"); Serial.print(vector[i]);}
        Serial.print("..."); 
        showInstructions();
      }
      else if (inputString == "n\n")  // ********* Neuron display
      {
        int compNbr=channelNbr*sampleNbr;
        compNbr=channelNbr; // override for easier display on serial monitor
        displayNeurons(compNbr);
        showInstructions();
      }
      else if (inputString == "ts\n") // ********* Transmit signals
      {
        transmitSensorDataFlag=true;
        millisPrevious = millis(); // used to control lapse time between consecutive getSensordata
        Serial.println("\n\nTransmitting sensor data to serial port. Type s to stop.");
      }
      else if (inputString == "r\n") // ********* Recognition
      {
        recoFlag=true;
        Serial.println("\n\nContinuous recognition. Type s to stop.");
      }    
      else  // ********* Learning
      {
        VAL = inputString.toInt();
        if ((VAL>=0) | ( VAL<=9))
        {              
          learningFlag=true;
          iterationID=0;
          Serial.print("\n\nLearning motion category "); Serial.print(VAL); Serial.print(". Type s to stop.");      
        }
      }
    }
    inputString="";
    stringComplete=false;
  }
  //
  // execution of the sustained commands
  //
  if (transmitSensorDataFlag==true)
  {
    getSensorData();
    Serial.print(lapse);
    Serial.print("\t"); Serial.print(ax);
    Serial.print("\t"); Serial.print(ay);
    Serial.print("\t"); Serial.print(az);
    if (channelNbr==6)
    {
      Serial.print("\t"); Serial.print(gx);
      Serial.print("\t"); Serial.print(gy);
      Serial.print("\t"); Serial.print(gz);
    }
    Serial.print("\n");   
  }
  if (learningFlag==true)
  {
    vlen=extractFeatureVector();
    if (vlen==0)
    {
      Serial.print("\n\nEmpty vector");
    }
    else
    { 
      prevncount=ncount;
      ncount=hNN.learn(vector, vlen, VAL);
      iterationID= iterationID +1;
      if (ncount!=prevncount)
      {
        Serial.print("\nNeurons="); Serial.println(ncount);
      }
      if (ncount==0xFFFF)
      {
          Serial.print("\n\nLearning stopped. \tNetwork is full!");  
          learningFlag=false;         
      }
      //
      // Stop the sustained learning. Can be limited to specific categories
      // applied to all categories in example below
      //
      if ((VAL >=0) | (VAL<=9))
      {
        if (iterationID== minIterations) 
        {
          Serial.print("\n\nResuming recognition"); 
          learningFlag=false;
          recoFlag= true;
        }
      }
    }
  }
  if (recoFlag==true)
  {
    vlen=extractFeatureVector();
    if (vlen!=0)
    {
      hNN.classify(vector, vlen, &dist, &cat, &nid);
      if (cat!=prevcat)
      {
        if (cat!=0xFFFF)
        {
          Serial.print("\nMotion category #"); Serial.print(cat);
          if (cat==1)
          {
            digitalWrite(LED_RED, HIGH);
            digitalWrite(LED_GREEN, LOW);
          }
          else if (cat==2)
          {
            digitalWrite(LED_GREEN, HIGH);
            digitalWrite(LED_RED, LOW);
          }
        }
        else
        {
          Serial.print("\nNo known motion");
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_RED, LOW);   
        }
        prevcat=cat;
      }
    }
  }
  else
  {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);   
  }
}  

void showInstructions()
{
    Serial.print("\n\nType a command + Enter");
    Serial.print("\n\t 1 to 9 = start learning motion type 1 to 9");
    Serial.print("\n\t 0 = correct false recognition");
    Serial.print("\n\t r = recognize");
    Serial.print("\n\t ts = transmit sensor data");
    Serial.print("\n\t tv = transmit vector data");
    Serial.print("\n\t s = stop current sustained command");
    Serial.print("\n\t v = show vector");
    Serial.print("\n\t n = show neurons");
    Serial.print("\n\t f = clear the knowledge");
    Serial.print("\n");
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

//------------------------------------------------------
// display the content of the neurons
//------------------------------------------------------
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
    Serial.print("\nneuron#"); Serial.print(i); Serial.print("\tmodel=");
    for (int k = 0; k < displayLength; k++) 
    {
        Serial.print("\t");
        Serial.print(model[k]);
    }
    Serial.print("..."); 
    Serial.print("\taif="); Serial.print(aif);
    Serial.print("\tcat="); Serial.print(cat);
  }
}

void getSensorData()
{
  do {
  lapse = (int)(millis()- millisPrevious);
  } while (lapse > millisPerReading);
  millisPrevious = millis();
  // read raw data from CurieIMU
  int aix, aiy, aiz;
  int gix, giy, giz;
  CurieIMU.readMotionSensor(aix, aiy, aiz, gix, giy, giz);
  // convert from raw data to gravity and degrees/second units
  ax = convertRawAcceleration(aix);
  ay = convertRawAcceleration(aiy);
  az = convertRawAcceleration(aiz);
  if (channelNbr==6)
  {
    gx = convertRawGyro(gix);
    gy = convertRawGyro(giy);
    gz = convertRawGyro(giz);
  }
}

float convertRawAcceleration(int aRaw) {
  // since we are using 2G range
  // -2g maps to a raw value of -32768
  // +2g maps to a raw value of 32767   
  //float a = (aRaw * 2.0) / 32768.0;
  float a = ((aRaw * 127.0) / 32768.0) + 128;
  return a;
}

float convertRawGyro(int gRaw) {
  // since we are using 250 degrees/seconds range
  // -250 maps to a raw value of -32768
  // +250 maps to a raw value of 32767  
  //float g = (gRaw * 250.0) / 32768.0;
  float g = ((gRaw * 127.0) / 32768.0) + 128;
  return g;
}

// *****************************************************************************************/
//  The following function is very academic and assemble a pattern which
//  should be more sophisticated for real-life system taking a calibration into account,
//  integrating a sampling rate adequate for the type of motion and profiling the waveforms
//  more selectively (distances between peaks and zero crossing, etc)
// *****************************************************************************************/

//------------------------------------------------------
// assemble a feature vector from the selected channels
//------------------------------------------------------
int extractFeatureVector()
{
  millisPrevious = millis();
  float Min=255, Max=0;
  for (int j=0; j<sampleNbr; j++)
  {
    getSensorData();
    vector[(j*channelNbr)]= (int)(ax);
    vector[(j*channelNbr)+1]= (int)(ay);
    vector[(j*channelNbr)+2]= (int)(az);
    if (channelNbr==6)
    {
      vector[(j*channelNbr)+3]= (int)(gx);
      vector[(j*channelNbr)+4]= (int)(gy);
      vector[(j*channelNbr)+5]= (int)(gz);
    }
    if (ax<Min) Min=ax;
    else if (ax>Max) Max=ax;
    if (ay<Min) Min=ay;
    else if (ay>Max) Max=ay;     
    if (az<Min) Min=az;
    else if (az>Max) Max=az;    
  }
  if (Max - Min < 2)
  {      
    Serial.print("No motion...Discarded ");Serial.println(Max-Min);
    return(0);
  }
  else
  {
    return(sampleNbr*channelNbr);
  }
}

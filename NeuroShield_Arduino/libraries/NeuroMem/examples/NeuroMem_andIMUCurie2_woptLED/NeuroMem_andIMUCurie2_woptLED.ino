//------------------------------------------------------------------------
// NeuroMem_andCurieIMU2_woptLED
//
// Copyright 2018 General Vision Inc.
//
// Updated 02/13/2018
// Support BrainCard and NeuroShield
//
// Tutorial at https://www.general-vision.com/techbriefs/TB_NeuroMemArduino_IMUDemo.pdf
//
//  Motion learning and recognition using the Arduino 101 and a NeuroMem shield
//  The script uses the Bosch accel/gyro of the Curie module and the neurons of the NeuroMem shield
//
//  Motion [ax,ay,az,gx,gy,gz] is converted into a simple vector by the extractFeatureVector function.
//  Feel free to modify this function and test more sophisticated feature extrcations 
//
//  1) Make sure the board lays still on a flat table during the calibration
//  
//  2) Start moving the board with a small amplitude Up-Down
//  Type "1", or the arbitrary code for the "vertical" category
//  Learning occurs when you press Enter
//    
//  3) Start moving the board with a small amplitude Left-Right
//  Type "2", or the arbitrary code for the "horizontal" category
//  Learning occurs when you press Enter
//  
//  4) Put the board back in the same position as during calibration
//  If the reported motion is not "Unknown", Type "0", or the arbitrary code for the "still" category
//  Repeat until the Motion "unknown" appears steadily on the serial port
//
//  5) Reproduce the same vertical motion as the one shown during learning in (2)
//  If the reported motion is not "1", teach more
//
//  6) Reproduce the same horizontal motion as the one shown during learning in (4)
//  If the reported motion is not "2", teach more
//  
//  Note that this "snapshot" approach is simplistic and you will have to teach a given motion
//  several times so the neurons store models with different amplitudes, acceleration, etc.
//  For example, you will have to teach vertical Up-Down and Down-Up, etc
//
//  Please understand that this example is very academic and assemble a pattern which
//  should be more sophisticated for real-life system taking a calibration into account,
//  integrating a sampling rate adequate for the type of motion and profiling the waveforms
//  more selectively (distances between peaks and zero crossing, etc)
//  
//
// -----------------------------------------------------------------------
//

#include "CurieIMU.h"

int ax, ay, az;         // accelerometer values
int gx, gy, gz;         // gyrometer values

int calibrateOffsets = 1; // int to determine whether calibration takes place or not

#include <NeuroMemAI.h>
NeuroMemAI hNN;

#define DEF_MAXIF 500 // default Maximum Influence Field
//
// Variables used for the calculation of the feature vector
//
#define sampleNbr 10  // number of samples to assemble a vector
int raw_vector[sampleNbr*3]; // vector accumulating sensor data
int vector1[sampleNbr*3]; // vector holding the pattern context 1
int vector2[sampleNbr*3]; // vector holding the pattern context 2
int cat1, cat2;
int dist, nid, nsr, ncount;
int sampleId=0, prevcat=0; // loop variables

// Variables to normalize the vector data
int mina=0xFFFF, maxa=0, ming=0xFFFF, maxg=0;
int da, dg;

//
// Optional LED outputs to report the motion category 1 and 2
//
#define LED_RED 2
#define LED_GREEN 3

String inputString = "";         // a string to hold incoming data (from serial port)
boolean stringComplete = false;  // whether the string is complete

void setup() 
{
    // reserve 3 bytes for the inputString:
  inputString.reserve(3);
  
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open

  // initialize device
  Serial.println("Initializing IMU device...");
  CurieIMU.begin();

  // use the code below to calibrate accel/gyro offset values
  if (calibrateOffsets == 1) 
  {    
    Serial.println("About to calibrate. Make sure your board is stable and upright");
    delay(5000);
    Serial.print("Starting Gyroscope calibration and enabling offset compensation...");
    CurieIMU.autoCalibrateGyroOffset();
    Serial.println(" Done");
    Serial.print("Starting Acceleration calibration and enabling offset compensation...");
    CurieIMU.autoCalibrateAccelerometerOffset(X_AXIS, 0);
    CurieIMU.autoCalibrateAccelerometerOffset(Y_AXIS, 0);
    CurieIMU.autoCalibrateAccelerometerOffset(Z_AXIS, 1);
    Serial.println(" Done");

    CurieIMU.setAccelerometerRange(8);
    CurieIMU.setGyroRange(1000);
  }
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
  hNN.forget(DEF_MAXIF); //set a conservative  Max Influence Field prior to learning

  Serial.print("\n\nEntering loop...");
  Serial.print("\nMove the module vertically or horizontally...");
  Serial.print("\ntype 1 + Enter if vertical motion");
  Serial.print("\ntype 2 + Enter if horizontal motion");
  Serial.print("\ntype 0 + Enter to correct false recognition");
  Serial.print("\n\ntype f + Enter to forget and start over");

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);  
}

void loop() 
{   
    // WARNING: make sure the serial printer settings is "new line"
    
    extractFeatureVector1(); // acceleration vector
    extractFeatureVector2(); // gyro vector
    
    // learn upon keyboard entry of 1 digit between 0-2
    // otherwise continuously report any change of motion
  if (stringComplete) 
  {
    if (inputString == "f\n")
    {
      hNN.forget(DEF_MAXIF);
      Serial.print("\nKnowledge cleared");
    }
    else
    {
      int VAL= inputString.toInt();
      //expected category input (1-vertical, 2-horizontal, etc)
      Serial.print("\nLearning motion category "); Serial.print(VAL);
      //
      // learn 5 consecutive sample vectors
      // (make sure to keep moving the 101 board accordingly)
      //
      for (int i=0; i<5; i++)
      {
        hNN.GCR(1);
        ncount=hNN.learn(vector1, sampleNbr*3, VAL);
        // learn feature vector 1 under context 1
        hNN.GCR(2);
        ncount=hNN.learn(vector2, sampleNbr*3, VAL);
      }
      Serial.print("\tNeurons="); Serial.print(ncount);   
    }
    inputString="";
    stringComplete=false;
  }
  //
  // Recognize
  //
      // Recognize feature vector 1 under context 1
      hNN.GCR(1);
      hNN.classify(vector1, sampleNbr*3,&dist, &cat1, &nid);
      // Recognize feature vector 1 under context 1
      hNN.GCR(2);
      hNN.classify(vector2, sampleNbr*3,&dist, &cat2, &nid);
      //
      // Report results on screen and with optional LED display
      // Return a positive response if neurons from context 1 and 2
      // are in agreement with the recognized category
      //
      if (cat1 == cat2)
      {
        if (cat1!=prevcat)
        {
          prevcat=cat1;
          if (cat1!=0xFFFF)
          {
            Serial.print("\nMotion category #"); Serial.print(cat1 & 0x7FFF);
            if ((cat1 & 0x7FFF) ==1)
            {
              digitalWrite(LED_RED, HIGH);
              digitalWrite(LED_GREEN, LOW);
            }
            else if ((cat1 & 0x7FFF) ==2)
            {
              digitalWrite(LED_GREEN, HIGH);
              digitalWrite(LED_RED, LOW);
            }
          }
          else
          {
            Serial.print("\nMotion not recognized");
            digitalWrite(LED_GREEN, LOW);
            digitalWrite(LED_RED, LOW);   
          }         
        }
      }
      else
      {
        if (prevcat != 0xFFFF)
        {
            Serial.print("\nNo classification because uncertainty");
            digitalWrite(LED_GREEN, LOW);
            digitalWrite(LED_RED, LOW);
            prevcat= 0xFFFF;
        }
      }
}  


/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}


//  The following function is very academic and assemble a pattern which
//  should be more sophisticated for real-life system taking a calibration into account,
//  integrating a sampling rate adequate for the type of motion and profiling the waveforms
//  more selectively (distances between peaks and zero crossing, etc)

void extractFeatureVector1()
{
  // sensor output [ax,ay,az] is converted into a byte array as follows:
  // [ax'1, ay'1, az'1, ax'2, ay'2, az'2,  ...] over a number of time samples.
  // a' are normalized using their respective min and max values.
  //
  // the reset of the min and max values is optional depending if you want to
  // use a running min and max from the launch of the script or not
  mina=0xFFFF, maxa=0, da=0;
  
  for (int sampleId=0; sampleId<sampleNbr; sampleId++)
  {
    //Build the vector over sampleNbr and broadcast to the neurons
    CurieIMU.readAccelerometer(ax, ay, az);
    
    // update the running min/max for the a signals
    if (ax>maxa) maxa=ax; else if (ax<mina) mina=ax;
    if (ay>maxa) maxa=ay; else if (ay<mina) mina=ay;
    if (az>maxa) maxa=az; else if (az<mina) mina=az;    
    da= maxa-mina;
    
    // accumulate the sensor data
    raw_vector[sampleId*3]= ax;
    raw_vector[(sampleId*3)+1]= ay;
    raw_vector[(sampleId*3)+2]= az;
  }
  
  // normalize vector
  for(int sampleId=0; sampleId < sampleNbr; sampleId++)
  {
    for(int i=0; i<3; i++)
    {
      vector1[(sampleId*3)+i]  = (((raw_vector[(sampleId*3)+i] - mina) * 255)/da) & 0x00FF;
    }
  }
}

void extractFeatureVector2()
{
  // sensor output [gx, gy,gz] is converted into a byte array as follows:
  // [gx'1,gy'1, gz'1, gx'2, gy'2, gz'2, ...] over a number of time samples.
  // g' are normalized using their respective min and max values.
  //
  // the reset of the min and max values is optional depending if you want to
  // use a running min and max from the launch of the script or not
  ming=0xFFFF, maxg=0, dg=0;
  
  for (int sampleId=0; sampleId<sampleNbr; sampleId++)
  {
    //Build the vector over sampleNbr and broadcast to the neurons
    CurieIMU.readGyro(gx, gy, gz);
    
    // update the running min/max for the g signals
    if (gx>maxg) maxg=gx; else if (gx<ming) ming=gx;
    if (gy>maxg) maxg=gy; else if (gy<ming) ming=gy;
    if (gz>maxg) maxg=gz; else if (gz<ming) ming=gz;   
    dg= maxg-ming;

    // accumulate the sensor data
    raw_vector[(sampleId*3)]= gx;
    raw_vector[(sampleId*3)+1]= gy;
    raw_vector[(sampleId*3)+2]= gz;
  }
  
  // normalize vector
  for(int sampleId=0; sampleId < sampleNbr; sampleId++)
  {
    for(int i=0; i<3; i++)
    {
      vector2[(sampleId*3)+i]  = (((raw_vector[(sampleId*3)+i] - ming) * 255)/dg) & 0x00FF;
    }
  }
}

//------------------------------------------------------------------------
// SimpleScript
//
// Copyright 2018 General Vision Inc.
//
// Updated 02/13/2018
// Support BrainCard and NeuroShield
//
// Simple Test Script to understand how the neurons learn and recognize patterns of numbers
// Refer to http://www.general-vision.com/documentation/TM_TestNeurons_SimpleScript.pdf
//
// -----------------------------------------------------------------------
//
// The patterns are arrays of length LEN composed of identical values VAL
// They basically represent horizontal lines with different heights. This representation
// is easy to comprehend the distance between the learned lines (stored in the memory
// of the committed neurons) and a new input line
//
// In this script, the patterns to recognize are generated programatically to
// surround the learned patterns

#include <NeuroMemAI.h>
NeuroMemAI hNN;

int dist, cat, nid, nsr, ncount;

#define LEN 4
int pattern[LEN]; // values must range between 0-255

#define K 2
int dists[K], cats[K], nids[K];

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() 
{
  // reserve 3 bytes for the inputString:
  inputString.reserve(3);
  
  Serial.begin(115200);
  while (!Serial);    // wait for the serial port to open

  int NMplatform=2;
  //Serial.print("\nSelect your NeuroMem platform:\n\t2 - NeuroShield\n\t3 - NeuroStamp\n");
  //while(Serial.available() <1);
  //NMplatform = Serial.read() - 48;      
  if (hNN.begin(NMplatform) == 0) 
  {
    Serial.print("\nYour NeuroMem_Smart device is initialized! ");
    Serial.print("\nThere are "); Serial.print(hNN.navail); Serial.print(" neurons\n");    
  }
  else 
  {
    Serial.print("\nYour NeuroMem_Smart device is NOT found!");
    Serial.print("\nCheck your device type and connection.\n");
    while (1);
  }
   
  //Build knowledge by learning 3 patterns with each constant values (respectively 11, 15 and 20)
  Serial.print("\nLearning three patterns...");
  for (int i=0; i<LEN; i++) pattern[i]=11;
  hNN.learn(pattern,LEN, 55);
  for (int i=0; i<LEN; i++) pattern[i]=15;
  hNN.learn(pattern,LEN, 33);
  for (int i=0; i<LEN; i++) pattern[i]=20;
  ncount=hNN.learn(pattern,LEN, 100);
   
  // Display the content of the committed neurons
  displayNeuronsLtd(ncount, LEN);
  
  for (int VAL=12; VAL<16; VAL++)
  {
      Serial.print("\n\nRecognizing a new pattern: ");
      for (int i=0; i<LEN; i++) pattern[i]=VAL;     
      for (int i=0; i<LEN; i++) { Serial.print(pattern[i]); Serial.print(", ");}
      int responseNbr=hNN.classify(pattern, LEN, K, dists, cats, nids);
      for (int i=0; i<responseNbr; i++)
      {
          Serial.print("\nFiring neuron "); Serial.print(nids[i]);
          Serial.print(", Category="); Serial.print(cats[i]);
          Serial.print(", at Distance="); Serial.print(dists[i]);
      }
  }

  Serial.print("\n\nLearning a new example (13) falling between neuron1 and neuron2...");
  for (int i=0; i<LEN; i++) pattern[i]=13;
  ncount=hNN.learn(pattern,LEN, 100);

  // Display the content of the committed neurons
  displayNeuronsLtd(ncount, LEN);

  Serial.println("\n\nNotice the addition of neuron 4 and");
  Serial.println("the shrinking of the influence fields of neuron1 and 2");
  
  //Prompt User for input
  Serial.print("\n\nEdit a value between [10 and 30] as the amplitude of a new pattern + Enter");
}

void loop()
{
  if (stringComplete) 
  {
      Serial.println(inputString);
      int VAL= inputString.toInt();
      inputString="";
      stringComplete=false;
  
      for (int i=0; i<LEN; i++) pattern[i]=VAL;     
      Serial.print("\npattern=");
      for (int i=0; i<LEN; i++) { Serial.print(pattern[i]); Serial.print(", ");}
    
      int responseNbr=hNN.classify(pattern, LEN, K, dists, cats, nids);
      if (responseNbr==0) Serial.print("\nUnknown pattern, No firing neurons!");
      for (int i=0; i<responseNbr; i++)
      {
          Serial.print("\nFiring neuron "); Serial.print(nids[i]);
          Serial.print(", Category="); Serial.print(cats[i]);
          Serial.print(", at Distance="); Serial.print(dists[i]);
      }
      Serial.print("\n\nEdit a value between [10 and 30] as the amplitude of a new pattern + Enter");
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


void displayNeuronsLtd(int neuronNbr, int displayLength)
{
  int ncount = hNN.NCOUNT();
  if (ncount == 0xFFFF) ncount = hNN.navail;
  if (neuronNbr > ncount) neuronNbr = ncount;
  Serial.print("\nDisplay the 1st "); Serial.print(ncount); Serial.println(" neurons");
  hNN.NSR(16); // set network to Save and Restore mode
  hNN.RESETCHAIN();
  for (int i = 0; i < neuronNbr; i++)
  {
    Serial.print("\nneuron# "); Serial.print(i+1); Serial.print("\tmodel=");
    for (int k = 0; k < displayLength; k++) {
      Serial.print(hNN.COMP());
      Serial.print(", ");
    }
    Serial.print("\tncr="); Serial.print(hNN.NCR());
    Serial.print("\taif="); Serial.print(hNN.AIF());
    Serial.print("\tcat="); Serial.print(hNN.CAT());
  }
  hNN.NSR(0); // set network back to Learn and Reco mode
}

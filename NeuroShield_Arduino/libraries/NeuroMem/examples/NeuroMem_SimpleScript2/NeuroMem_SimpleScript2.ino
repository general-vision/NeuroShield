//------------------------------------------------------------------------
// SimpleScript2
//
// Copyright 2018 General Vision Inc.
//
// Updated 02/13/2018
// Support BrainCard and NeuroShield
//
// Equivalent to SimpleScript +
// use of the neurons in KNN and RBF mode,
// use of multiple contexts
// use of the LSup norm
//
// http://general-vision.com/documentation/TM_NeuroMem_Technology_Reference_Guide.pdf */
//
// -----------------------------------------------------------------------
//
// The patterns are arrays of length LEN composed of identical values VAL
// They basically represent horizontal lines with different heights. This representation
// is easy to comprehend the distance between the learned lines (stored in the memory
// of the committed neurons) and a new input line

#include <NeuroMemAI.h>
NeuroMemAI hNN;

#define LEN 4
int pattern[LEN]; // values must range between 0-255
#define K 3
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
  for (int i = 0; i < LEN; i++) pattern[i] = 11;
  hNN.learn(pattern, LEN, 55);
  for (int i = 0; i < LEN; i++) pattern[i] = 15;
  hNN.learn(pattern, LEN, 33);
  for (int i = 0; i < LEN; i++) pattern[i] = 20;
  int neuronsCommitted = hNN.learn(pattern, LEN, 100);

  displayNeuronsLtd(neuronsCommitted, LEN);

  hNN.setKNN();
  Serial.print("\n\nRecognizing pattern ");
  for (int i = 0; i < LEN; i++) pattern[i] = 12;
  for (int i = 0; i < LEN; i++) {
    Serial.print(pattern[i]);
    Serial.print(", ");
  }
  Serial.print("\nClassify in mode KNN (K=3) ");
  ClassifyPattern(K);

  hNN.setRBF();
  Serial.print("\n\nRecognizing pattern ");
  for (int i = 0; i < LEN; i++) pattern[i] = 12;
  for (int i = 0; i < LEN; i++) {
    Serial.print(pattern[i]);
    Serial.print(", ");
  }
  Serial.print("\nin mode RBF (K up to 3) ");
  ClassifyPattern(K);
  Serial.print("\n\**Notice that Neuron #3 does not fire in RBF mode\n");
  
  // change the context to witness that a 2nd sub-network can be trained independantly
  // of the 1st sub-network assigned to context value=1
  hNN.GCR(2);
  Serial.print("\n\nLearning the pattern (13) under a different Context=2 with arbitrary Category=200");
  for (int i = 0; i < LEN; i++) pattern[i] = 13;
  neuronsCommitted = hNN.learn(pattern, LEN, 200);

  displayNeuronsLtd(neuronsCommitted, LEN);
  Serial.print("\n***Notice the commitment of new neuron with context 2 and");
  Serial.print("\nno impact on the influence fields of neuron1 and 2 belonging to Context 1");

  // change the context to witness that a 3rd sub-network can be trained independantly
  // of the other two context (value 1 and 2) and can use
  // a different norm for distance calculation (bit7 of GCR set to 1 to use norm LSup
  hNN.GCR(129);
  Serial.print("\n\nLearning the pattern (13) under a different Context=129 with arbitrary Category=300");
  Serial.print("\n** Notice that bit 7 of Context=129 sets the Norm to LSup");
  for (int i = 0; i < LEN; i++) pattern[i] = 13;
  neuronsCommitted = hNN.learn(pattern, LEN, 300);

  displayNeuronsLtd(neuronsCommitted, LEN);
  Serial.print("\n*** Notice the commitment of new neuron with Context 129");
  Serial.print("\nand holding the same pattern as neuron 4 but");
  Serial.print("\nno impact on the influence fields of neurons belonging to Context 1 or 2");

  hNN.GCR(1);
  Serial.print("\n\nLearning the same example (13) under Context=1");
  for (int i = 0; i < LEN; i++) pattern[i] = 13;
  neuronsCommitted = hNN.learn(pattern, LEN, 100);

  displayNeuronsLtd(neuronsCommitted, LEN);
  Serial.print("\n*** Notice the commitment of new neuron with Context 1");
  Serial.print("\nand holding the same pattern as neuron 4 and 5, but also notice");
  Serial.print("\nthe reduction of the influence fields of neurons belonging to Context 1");

  Serial.print("\n\nRecognize the best match of a same pattern through the 3 different Contexts\n");
  for (int i = 0; i < LEN; i++) pattern[i] = 12;
  for (int i = 0; i < LEN; i++) {
    Serial.print(pattern[i]);
    Serial.print(", ");
  }
  Serial.print("\n\nActivate Context 1 and its Norm L1");
  hNN.GCR(1);
  ClassifyPattern(1);
  Serial.print("\n\nActivate Context 2 and its Norm L1");
  hNN.GCR(2);
  ClassifyPattern(1);
  Serial.print("\n\nActivate Context 129 and its Norm LSup");
  hNN.GCR(129);
  ClassifyPattern(1);
  Serial.print("\n*** Notice the different distance values. categories and status");
  Serial.print("\n\n*** This script is purely academic and describes how to manipulate contexts and norms programatically");
  Serial.print("\nRefer to the documentation for information on the powerful concept of multiple contexts");
  // http://general-vision.com/documentation/TM_NeuroMem_Technology_Reference_Guide.pdf */

  Serial.print("\n\nBack to Context 1...\n");
  hNN.GCR(1);
  Serial.print("\n\nEdit a value between [10 and 30] as the amplitude of a new pattern + Enter");
}

void loop()
{
  if (stringComplete)
  {
    Serial.println(inputString);
    int VAL = inputString.toInt();
    inputString = "";
    stringComplete = false;

    for (int i = 0; i < LEN; i++) pattern[i] = VAL;
    Serial.print("\npattern=");
    for (int i = 0; i < LEN; i++) {
      Serial.print(pattern[i]);
      Serial.print(", ");
    }
    ClassifyPattern(K);
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

void ClassifyPattern(int K_resp)
{
    // read the response of up to K firing neurons if applicable
    int responseNbr = hNN.classify(pattern, LEN, K_resp, dists, cats, nids);
    if (responseNbr == 0) Serial.print("\nUnknown pattern, No firing neurons!");
    for (int i = 0; i < responseNbr; i++)
    {
      Serial.print("\nNeuron# "); Serial.print(nids[i]);
      Serial.print("\tCategory="); Serial.print(cats[i]);
      Serial.print("\tDistance="); Serial.print(dists[i]);
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

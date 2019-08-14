//-----------------------------------------------------------------------------------------
// NeuroShield_IMUDemo
// Copyrights 2018, General Vision Inc.
//
// Motion Recognition using the Invensense accel/gyro and NM500 neurons of the NeuroShield
// with the modeling of a single decision spaces for the accelerations and gyroscsopes
//
// Tutorial at https://www.general-vision.com/techbriefs/TB_NeuroMemArduino_IMUDemo.pdf
//
//********************************************************************
//
// IMPORTANT REMARK
//
// This example is very academic and assemble a simple sequence pattern which should be more
// sophisticated to address a real-life problem such as real-time sampling rate and calibration
// adequate for the type of motion being studied.
// More advanced feature extractions can include waveform profiles, distribution of peaks and zero crossing, etc.
//  
//-----------------------------------------------------------------------------------------
//
// Dependencies
// https://playground.arduino.cc/Main/MPU-6050
// https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050
// https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/I2Cdev
//
// Info about calibration and use of the MPU6050
// https://www.invensense.com/products/motion-tracking/6-axis/mpu-6050/

#include <Wire.h>
#include <MPU6050.h>
MPU6050 mpu(0x68);

#include <NeuroMemAI.h>
NeuroMemAI hNN;
#define HW_NEUROSHIELD 2

#include <SD.h>
File SDfile;

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
int displaylen=10; // length of the data displayed on serial monitor

int VAL=0;
int sampleNbr=20; // number of samples appended into a feature vector
int channelNbr=6; // ax,ay,az,gx,gy,gz
int16_t ax, ay, az, gx, gy, gz;  

bool learningFlag=false;
bool recoFlag=false;
bool noveltyFlag=false;
bool transmitFlag=false;

#define DEF_MAXIF 1000 // default Maximum Influence Field of the neurons
int maxif= DEF_MAXIF;

// The collection of a new sensor dataset is sampled at a rate relevant for the type
// of motion under study
// This lapse global is used in the getSensorData function
unsigned long lapse=0, timeStamp=0;
unsigned long millisInitial=0;
unsigned long millisPrevious=0;

void showInstructions()
{
    Serial.print("\n\nType a command + Enter");
    Serial.print("\n\t h = help");
    Serial.print("\n\t s = stop");
    Serial.print("\n\t 1 to 9 = start learning motion type 1 to 9");
    Serial.print("\n\t 0 = start learning motion type UNKNOWN");
    Serial.print("\n\t r = recognize");
    Serial.print("\n\t rn = recognize + record novelty");
    Serial.print("\n\t t = transmit sensor data");
    Serial.print("\n\t v = show vector");
    Serial.print("\n\t cfg = show configuration");
    Serial.print("\n\t - = decrease Maxif");
    Serial.print("\n\t + = increase maxif");
    Serial.print("\n\t ckn = clear knowledge");
    Serial.print("\n\t vkn = view knowledge");
    Serial.print("\n");
}

void setup()
{
  // reserve 4 bytes for the inputString:
  inputString.reserve(4);
  
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open
  //
  // Initialize the neurons, load knowledge from SD card if any
  // or set default project settings
  //
  if (hNN.begin(HW_NEUROSHIELD) == 0) 
  {
    Serial.print("\nNeuroMem_Smart device is initialized!");
    Serial.print("\nNeuron capacity = "); Serial.print(hNN.navail);
    hNN.forget(DEF_MAXIF);
    maxif= hNN.MAXIF();
    Serial.print("\nNN Maxif = "); Serial.print(maxif); 
  }
  else 
  {
    Serial.print("\nNeuroMem_Smart device is NOT properly connected!!");
    Serial.print("\nCheck your device type and connection!\n");
    while (1);
  }
  if (sampleNbr*channelNbr > hNN.NEURONSIZE)
  {
    sampleNbr = hNN.NEURONSIZE / channelNbr;
  }

  vector = new int[sampleNbr*channelNbr];
  vlen = sampleNbr*channelNbr;
  if (displaylen > vlen) displaylen = vlen;
  
  //---------------------------------------------
  // Initialization and calibration of IMU device  
  //---------------------------------------------
  Wire.begin(); // i2c initialization to access MPU6050
  mpu.initialize();
  if (mpu.testConnection()) 
  {
    Serial.print("\n\nIMU connection successful!\n");
  }
  else
  {
    Serial.print("\n\nIMU connection failed");
    Serial.print("\nCheck the connection and Reboot again!\n");
    while (1);
  }
  Serial.print("\nCalibration...Make sure your board is stable.\tType enter when ready");
  while (Serial.available() && Serial.read());
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_1000); // gyro scale values: ±250, ±500, ±1000, and ±2000 °/sec (dps) 
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_8); // accel scale values: ±2g, ±4g, ±8g, and ±16g
  mpu6050Calibration(); // calculate the accel/gyro offsets
  Serial.print("\nCalibration terminated"); 

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);  
  
  showInstructions();
}

void loop() 
{   
  if (stringComplete) 
  {
    processSerialEvent();
    stringComplete=false;
  }
  //
  // execution of the sustained commands
  //
  if (transmitFlag==true)
  {
    getSensorData();
    Serial.print(lapse);
    Serial.print("\t"); Serial.print(ax);
    Serial.print("\t"); Serial.print(ay);
    Serial.print("\t"); Serial.print(az);
    Serial.print("\t"); Serial.print(gx);
    Serial.print("\t"); Serial.print(gy);
    Serial.print("\t"); Serial.print(gz);
    Serial.print("\n");   
  }
  else if (learningFlag==true)
  {
    prevncount=ncount;
    vlen=extractFeatureVector();
    ncount=hNN.learn(vector, vlen, VAL);
    if (ncount!=prevncount)
    {
      Serial.print("\nNeurons="); Serial.println(ncount);
    }
    if (ncount==0xFFFF)
    {
        Serial.print("\n\nLearning stopped. \tNetwork is full!");  
        learningFlag=false;         
    }
  }
  else if (recoFlag==true)
  {
    vlen=extractFeatureVector();
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
    //
    // Uncomment the next paragraph to enable the novelty detection and learning
    // under a second context
    //
    if (noveltyFlag==true)
    {
      if (dist==0xFFFF)
      {
          // Learn the Unknown vector under Context 2 with the time as Category value
          prevncount=ncount;
          timeStamp=(millis()- millisInitial)/100;
          hNN.setContext(2,2,DEF_MAXIF/2);    // Set Context 2 and low maxif to learn a minimum novelties
          ncount=hNN.learn(vector, vlen, 1);
          hNN.setContext(1,2,DEF_MAXIF);   // Restore Context 1
          if (ncount > prevncount) {Serial.print("\nLearned novelty at "); Serial.println(timeStamp);}        
      }
    }
  }
  else
  {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);   
  }
}  
//------------------------------------------------------
// Detect user input through serial monitor
//------------------------------------------------------
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
void processSerialEvent() 
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
      else if (transmitFlag==true)
      {
          Serial.print("\n\nSensor Data Transmission stopped.");
      }
      else Serial.print("\n\nStopping current sustained operation");
      learningFlag=false;
      recoFlag=false;
      transmitFlag=false;   
      showInstructions();
    }
    else
    {
      learningFlag=false;
      recoFlag=false;
      noveltyFlag=false;
      transmitFlag=false;
      if (inputString == "h\n") // ********* Help
      {
        showInstructions();
      }
      else if (inputString == "-\n") // ********* Decrease Maxif
      {
        maxif=maxif-100;
        hNN.setContext(1,2,maxif);
        Serial.print("\n\nMaxif decreased to "); Serial.print(maxif);
        showInstructions();
      }
      else if (inputString == "+\n") // ********* Decrase Maxif
      {
        maxif=maxif+100;
        hNN.setContext(1,2,maxif);
        Serial.print("\n\nMaxif increased to "); Serial.print(maxif);
        showInstructions();
      }
      else if (inputString == "v\n") // ********* Vector display
      {
        Serial.print("\n\nCurrent vector");
        vlen=extractFeatureVector();
        for (int i=0; i<displaylen; i++) { Serial.print("\t"); Serial.print(vector[i]);}
        Serial.print("..."); 
      }
      else if (inputString == "cfg\n") // ********** View configuration
      {
          Serial.print("\n\nCurrent settings");
          Serial.print("\n\tAccel Full Range = "); Serial.print(mpu.getFullScaleAccelRange());
          Serial.print("\n\tGyro Full Range = "); Serial.print(mpu.getFullScaleGyroRange());
          Serial.print("\n\tNN Maxif = "); Serial.print(maxif);       
      }
      else if (inputString == "ckn\n") // ********* Clear knowledge
      {
        hNN.forget(DEF_MAXIF);
        maxif=DEF_MAXIF;
        Serial.print("\n\nKnowledge cleared");
        showInstructions();
      }
      else if (inputString == "vkn\n")  // ********* View knowledge
      {
        displayNeurons(displaylen);
        showInstructions();
      }
      else if (inputString == "t\n") // ********* Transmit signals
      {
        transmitFlag=true;
        millisPrevious = millis();
        Serial.println("\n\nTransmitting sensor data to serial port. Type s to stop.");
      }
      else if (inputString == "r\n") // ********* Recognition
      {
        recoFlag=true;
        Serial.println("\n\nContinuous recognition. Type s to stop.");
      }
      else if (inputString == "rn\n") // ********* Recognition with Novelty logging
      {
        recoFlag=true;
        noveltyFlag=true;
        Serial.println("\n\nContinuous recognition. Recording of novelties. Type s to stop.");
      }
      else  // ********* Learning
      {
        VAL = inputString.toInt();
        if ((VAL>=0) | ( VAL<=9))
        {              
          learningFlag=true;
          Serial.print("\n\nLearning motion category "); Serial.print(VAL); Serial.print(". Type s to stop.");      
        }
      }
    }
    inputString="";
    stringComplete=false;
}
//------------------------------------------------------
// Collect and convert the raw data to gravity and degrees/second units
// according to the gyro and accelero respective full-scale ranges
// selected in the setup before the calibration
//------------------------------------------------------
void getSensorData()
{
  lapse = (int)(millis()- millisPrevious);
  int16_t aix,aiy,aiz,gix, giy, giz;
  mpu.getMotion6(&aix, &aiy, &aiz, &gix, &giy, &giz);
  ax = convertRawAcceleration(aix);
  ay = convertRawAcceleration(aiy);
  az = convertRawAcceleration(aiz);
  gx = convertRawGyro(gix);
  gy = convertRawGyro(giy);
  gz = convertRawGyro(giz);
}

float convertRawAcceleration(int aRaw) {
  // map raw values from [-maxRaw, maxRaw] to [-range, + range]
  int maxRaw = 16384; // BOSCH maxRaw = 32768
  // to be studied...instead of linear conversion below
  //int range= mpu.getFullScaleAccelRange();   
  //float a = (aRaw * range) / maxRaw;
  float a = ((aRaw * 127.0) / maxRaw) + 128;
  return a;
}

float convertRawGyro(int gRaw) {
  // map raw values from [-maxRaw, maxRaw] to [-range, + range]
  int maxRaw = 16384; // BOSCH maxRaw = 32768
  // to be studied...instead of linear conversion below
  //int range= mpu.getFullScaleGyroRange();
  //float g = (gRaw * range) / maxRaw;
  float g = ((gRaw * 127.0) / maxRaw) + 128;
  return g;
}
//------------------------------------------------------
// Assemble a feature vector from the selected channels
// by sampling sensor data over a number of iterations
//
//------------------------------------------------------
int extractFeatureVector()
{
  millisPrevious = millis();
  for (int j=0; j<sampleNbr; j++)
  {
    getSensorData();
    vector[(j*channelNbr)]= (int)(ax);
    vector[(j*channelNbr)+1]= (int)(ay);
    vector[(j*channelNbr)+2]= (int)(az);
    vector[(j*channelNbr)+3]= (int)(gx);
    vector[(j*channelNbr)+4]= (int)(gy);
    vector[(j*channelNbr)+5]= (int)(gz);   
  }
  return(sampleNbr*channelNbr);
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
//------------------------------------------------------
// MPU6050 Calibration
//------------------------------------------------------
void mpu6050Calibration()
{
  int i, j;
  int32_t sum_ax = 0, sum_ay = 0, sum_az = 0, sum_gx = 0, sum_gy = 0, sum_gz = 0;
  int16_t mean_ax, mean_ay, mean_az, mean_gx, mean_gy, mean_gz;
  int16_t ax_offset, ay_offset, az_offset, gx_offset, gy_offset, gz_offset;

  int16_t aix, aiy, aiz, gix, giy, giz;  
  for (i = 0; i < 100; i++) {
    mpu.getMotion6(&aix, &aiy, &aiz, &gix, &giy, &giz);
  }

  for (j = 0; j < 3; j++)
  {
    for (i = 0; i < 100; i++)
    {
      mpu.getMotion6(&aix, &aiy, &aiz, &gix, &giy, &giz);
      sum_ax += ax;
      sum_ay += ay;
      sum_az += az;
      sum_gx += gx;
      sum_gy += gy;
      sum_gz += gz;
    }
  
    mean_ax = sum_ax / 100;
    mean_ay = sum_ay / 100;
    mean_az = sum_az / 100;
    mean_gx = sum_gx / 100;
    mean_gy = sum_gy / 100;
    mean_gz = sum_gz / 100;
  
    // MPU6050_GYRO_FS_1000 : offset = (-1) * mean_g
    // MPU6050_ACCEL_FS_8   : offset = (-0.5) * mean_a
    ax_offset = (-mean_ax) / 2;
    ay_offset = (-mean_ay) / 2;
    az_offset = (-mean_az) / 2;
    gx_offset = -mean_gx;
    gy_offset = -mean_gy;
    gz_offset = -mean_gz;
  
    // set...Offset function does not work -> using software offset
    mpu.setXAccelOffset(ax_offset);
    mpu.setYAccelOffset(ay_offset);
    mpu.setZAccelOffset(az_offset);
    mpu.setXGyroOffset(gx_offset);
    mpu.setYGyroOffset(gy_offset);
    mpu.setZGyroOffset(gz_offset);
  }
}

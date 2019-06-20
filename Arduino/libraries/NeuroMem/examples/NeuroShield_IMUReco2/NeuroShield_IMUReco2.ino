//-----------------------------------------------------------------------------------------
// NeuroShield_IMUDemo2
// Copyrights 2018, General Vision Inc.
//
// Motion Recognition using the Invensense accel/gyro and NM500 neurons of the NeuroShield
// with the modeling of 2 separate decision spaces for the accelerations and gyroscsopes
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

int* raw_vector; // vector accumulating sensor data
int* vector1; // vector holding the pattern context 1
int* vector2; // vector holding the pattern context 2
int cat1, cat2, dist1, dist2;
int prevncount=0, prevcat=0, vlen=0; // buffers for previous values
int dist, nid, nsr, ncount; // response from the neurons
//
// Optional LED outputs to report the motion category 1 and 2
//
#define LED_RED 2
#define LED_GREEN 3

String inputString = "";         // a string to hold incoming data (from serial port)
String prevString = "";
boolean stringComplete = false;  // whether the string is complete

int VAL=0;
int sampleNbr=10; // number of samples appended into a feature vector
int channelFeat1=3; // ax,ay,az
int channelFeat2=3; // gx,gy,gz
int16_t ax, ay, az, gx, gy, gz;  

bool learningFlag=false;
bool recoFlag=false;
bool transmitSensorDataFlag=false;
int minIterations=5; // number of examples to learn at each learn command
int iterationID=0;

#define DEF_MAXIF 1000 // default Maximum Influence Field of the neurons

// This lapse global is used in the getSensorData function
int lapse=0;
unsigned long millisPrevious=0;
//unsigned long millisPerReading = 1000 / 25;

void showInstructions()
{
    Serial.print("\n\nType a command + Enter");
    Serial.print("\n\t h = help");
    Serial.print("\n\t s = stop");
    Serial.print("\n\t 1 to 9 = start learning motion type 1 to 9");
    Serial.print("\n\t 0 = incorrect recognition");
    Serial.print("\n\t r = recognize");
    Serial.print("\n\t t = transmit sensor data");
    Serial.print("\n\t v1 = show vector1 or accelerations");
    Serial.print("\n\t v2 = show vector2 ir gyroscope");
    Serial.print("\n\t cfg = show configuration");
    Serial.print("\n\t ckn = clear knowledge");
    Serial.print("\n\t vkn = view knowledge");
    Serial.print("\n");
}
 
void setup()
{
  // reserve 3 bytes for the inputString:
  inputString.reserve(3);
  
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open
  //
  // Initialize the neurons and set a conservative Max Influence Field
  //
  if (hNN.begin(HW_NEUROSHIELD) == 0) 
  {
    Serial.print("\nNeuroMem_Smart device is initialized!");
    Serial.print("\nNeuron capacity = "); Serial.print(hNN.navail);
    hNN.forget(DEF_MAXIF);
    Serial.print("\nNN Maxif = "); Serial.print(DEF_MAXIF);    
  }
  else 
  {
    Serial.print("\nNeuroMem_Smart device is NOT properly connected!!");
    Serial.print("\nCheck your device type and connection!\n");
    while (1);
  }
  if (sampleNbr*channelFeat1 > hNN.NEURONSIZE)
  {
    sampleNbr = hNN.NEURONSIZE / channelFeat1;
  }
  vector1= new int[sampleNbr*channelFeat1];
  vector2= new int[sampleNbr*channelFeat2];
  raw_vector= new int[sampleNbr*channelFeat1]; // same buffer for the 2 features since they use the same number of channels
  //
  // Initialization and calibration of IMU device  
  //
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
  //
  // Decode and process inputs from serial monitor
  //
  if (stringComplete) 
  {
    processSerialEvent();
    stringComplete=false;
  }
  //
  // execution of the sustained commands
  //
  if (transmitSensorDataFlag==true)
  {
    getSensorData();
    //Serial.print(lapse);
    Serial.print("\t"); Serial.print(ax);Serial.print("\t"); Serial.print(ay);Serial.print("\t"); Serial.print(az);
    Serial.print("\t"); Serial.print(gx);Serial.print("\t"); Serial.print(gy);Serial.print("\t"); Serial.print(gz);
    Serial.print("\n");   
  }
  else if (learningFlag==true)
  {
    prevncount=ncount;
    extractFeatureVector1(); // acceleration vector
    hNN.GCR(1);      
    extractFeatureVector2(); // gyro vector
    ncount=hNN.learn(vector1, sampleNbr*channelFeat1, VAL); // learn feature vector 1 under context 1  
    hNN.GCR(2);
    ncount=hNN.learn(vector2, sampleNbr*channelFeat2, VAL); // learn feature vector 2 under context 2
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
    Recognize();
  }
  else
  {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);   
  }
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
      learningFlag=false;
      recoFlag=false;
      transmitSensorDataFlag=false;
      if (inputString == "h\n") // ********* Help
      {
        showInstructions();
      }
      else if (inputString == "v1\n") // ********* Vector1 display
      {
        Serial.print("\n\nCurrent vector1 or accelerations");
        extractFeatureVector1();
        int compNbr=channelFeat1*5;
        for (int i=0; i<compNbr; i++) { Serial.print("\t"); Serial.print(vector1[i]);}
        Serial.print("..."); 
      }
      else if (inputString == "v2\n") // ********* Vector2 display
      {
        Serial.print("\n\nCurrent vector2 or gyroscopes");
        extractFeatureVector2();
        int compNbr=channelFeat2*5;
        for (int i=0; i<compNbr; i++) { Serial.print("\t"); Serial.print(vector2[i]);}
        Serial.print("..."); 
      }
      else if (inputString == "cfg\n") // ********** View settings
      {
          Serial.print("\n\nCurrent settings");
          Serial.print("\n\tAccel Full Range = "); Serial.print(mpu.getFullScaleAccelRange());
          Serial.print("\n\tGyro Full Range = "); Serial.print(mpu.getFullScaleGyroRange());
          Serial.print("\n\tNN Maxif = "); Serial.print(DEF_MAXIF);       
      }
      else if (inputString == "ckn\n") // ********* clear knowledge or neurons' content
      {
        hNN.forget(DEF_MAXIF);
        Serial.print("\n\nKnowledge cleared");
        showInstructions();
      }
      else if (inputString == "vkn\n")  // ********* view knowledge or neurons' content
      {
        int compNbr=channelFeat1*3;
        displayNeurons(compNbr);
        showInstructions();
      }
      else if (inputString == "t\n") // ********* Transmit signals
      {
        transmitSensorDataFlag=true;
        millisPrevious = millis(); // to calculate the lapse value between 2 samples
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
//
// Extract feature vector 1 from accelerometer
//
int extractFeatureVector1()
{
  for (int j=0; j<sampleNbr; j++)
  {
    getSensorData();
    vector1[(j*channelFeat1)]= (int)(ax);
    vector1[(j*channelFeat1)+1]= (int)(ay);
    vector1[(j*channelFeat1)+2]= (int)(az);   
  }
  return(sampleNbr*channelFeat1);
}
//
// Extract feature vector 1 from accelerometer
//
int extractFeatureVector2()
{
  for (int j=0; j<sampleNbr; j++)
  {
    getSensorData();
    vector2[(j*channelFeat2)]= (int)(gx);
    vector2[(j*channelFeat2)+1]= (int)(gy);
    vector2[(j*channelFeat2)+2]= (int)(gz); 
  }
  return(sampleNbr*channelFeat2);
}

//------------------------------------------------------
// Recognize the sensor data through 2 feature sets or contexts
// Output a positive response if neurons from both contexts are in agreement with the recognition
//------------------------------------------------------
void Recognize()
{
  char tmpStr[10];
  String resString="";
  //
  // Recognize feature vector 1 under context 1 (accelerations)
  //
  extractFeatureVector1();
  hNN.GCR(1);
  hNN.classify(vector1, sampleNbr*channelFeat1,&dist1, &cat1, &nid);
  //
  // Recognize feature vector 2 under context 2 (gyroscope)
  //
  extractFeatureVector2();
  hNN.GCR(2);
  hNN.classify(vector2, sampleNbr*channelFeat2,&dist2, &cat2, &nid);
  //
  // Return a positive response if neurons from context 1 and 2
  // are in agreement with the recognized category
  //
  if ((cat1 == cat2) & (cat1 != 0xFFFF))
  {
      resString ="Motion identified as ";
      itoa (cat1 & 0x7FFF, tmpStr, 10); // mask the degenerated flag (bit 15 of CAT register)
      resString += tmpStr;
  }
  else
  {
    resString="Motion unknown";
  }
  if (resString!=prevString)
  {
    Serial.println(resString);
    prevString=resString;
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
    Serial.print("\nneuron#"); Serial.print(i);
    Serial.print("\tncr="); Serial.print(ncr);
    Serial.print("\tcat="); Serial.print(cat);
    Serial.print("\taif="); Serial.print(aif);
    Serial.print("\t\tmodel=");
    for (int k = 0; k < displayLength; k++) 
    {
        Serial.print("\t");
        Serial.print(model[k]);
    }
    Serial.print("..."); 
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

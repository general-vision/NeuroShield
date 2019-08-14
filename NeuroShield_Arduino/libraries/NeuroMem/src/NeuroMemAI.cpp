/************************************************************************/
/*																		
 *	NeuroMemAI.cpp	--	Class to use a NeuroMem network	
 
 *	Copyright (c) 2016, General Vision Inc, All rights reserved	
 *
 *  Created September 8, 2017
 *
 *
 * Updated 11/09/2017
 * In order to accommodate future models of NeuroMem chips with neuron memory size
 * different from a 256 byte array, the data type of vector, model and neuron has
 * been changed from unsigned char to int array. The upper bytes are always null in the case of the CM1K and NM500 chips.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/******************************************************************************/


/* ------------------------------------------------------------ */
/* www.general-vision.com/documentation/TM_NeuroMem_API.pdf		*/
/* ------------------------------------------------------------ */

#include <NeuroMemAI.h>
#include <NeuroMemSPI.h> // Modules and Registers are declared in NeuroMemSPI.h
#include <SD.h>

using namespace std;
extern "C" {
  #include <stdint.h>
}

NeuroMemSPI spi;

// ------------------------------------------------------------ //
//    Constructor to the class NeuroMemAI
// ------------------------------------------------------------ 
NeuroMemAI::NeuroMemAI(){	
}
// ------------------------------------------------------------ 
// Initialize the neural network
// ------------------------------------------------------------ 

// SD card chip select
#define SD_CS_BRAINCARD 9
#define SD_CS_NEUROSHIELD 6 // use SD_CS_NEUROSHIELD 9 for SD on ArduCAM instead 
#define SD_CS_NEUROSTAMP 0 // no SDcard
#define SD_CS_NEUROTILE 0

int NeuroMemAI::version()
{
	return(13);
}

int NeuroMemAI::begin(int Platform)
{
	int error=spi.connect(Platform);
	if (error==0) 
	{
		countNeuronsAvailable(); // update the global navail
		clearNeurons();
		SD_select=0;
		switch(Platform)
		{
			case HW_BRAINCARD: SD_select=SD_CS_BRAINCARD; break;
			case HW_NEUROSHIELD: SD_select=SD_CS_NEUROSHIELD; break;
			case HW_NEUROTILE: SD_select=SD_CS_NEUROTILE; break;
		}
		SD_detected=SD.begin(SD_select);
	}
	return(error);
}
// ------------------------------------------------------------ 
// Un-commit all the neurons, so they all become ready to learn
// Reset the Maximum Influence Field to default value=0x4000
// ------------------------------------------------------------ 
void NeuroMemAI::forget()
{
	spi.write(MOD_NM, NM_FORGET, 0);
}
// ------------------------------------------------------------ 
// Un-commit all the neurons, so they all become ready to learn,
// Set the Maximum Influence Field (default value=0x4000)
// ------------------------------------------------------------ 
void NeuroMemAI::forget(int Maxif)
{
	spi.write(MOD_NM, NM_FORGET, 0);
	spi.write(MOD_NM, NM_MAXIF, Maxif);
}
// --------------------------------------------------------------
// Clear the memory of the neurons to the value 0
// Reset default GCR=1, MINIF=2, MANIF=0x4000, CAT=0
// --------------------------------------------------------------
void NeuroMemAI::clearNeurons()
{
	spi.write(MOD_NM, NM_NSR, 16);
	spi.write(MOD_NM, NM_TESTCAT, 0x0001);
	spi.write(MOD_NM, NM_NSR, 0);
	for (int i=0; i< NEURONSIZE; i++)
	{
		spi.write(MOD_NM, NM_INDEXCOMP,i);
		spi.write(MOD_NM, NM_TESTCOMP,0);
	}
	spi.write(MOD_NM, NM_FORGET,0);
}
// ------------------------------------------------------------ 
// Detect the capacity of the NeuroMem network
// plugged on the board
// ------------------------------------------------------------ 
int NeuroMemAI::countNeuronsAvailable()
{
	spi.write(MOD_NM, NM_FORGET, 0);
	spi.write(MOD_NM, NM_NSR, 0x0010);
	spi.write(MOD_NM, NM_TESTCAT, 0x0001);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	int read_cat=0xFFFF;	
	navail = 0;
	while (1) {
		read_cat = spi.read(MOD_NM, NM_CAT);
		if (read_cat == 0xFFFF)
			break;
		navail++;
	}
	spi.write(MOD_NM, NM_NSR, 0x0000);
	spi.write(MOD_NM, NM_FORGET, 0);
	return(navail);
}
// --------------------------------------------------------
// Broadcast a vector to the neurons and return the recognition status
// 0= unknown, 4=uncertain, 8=Identified
//---------------------------------------------------------
int NeuroMemAI::broadcast(int vector[], int length)
{
	spi.writeAddr(0x01000001, length-1, vector);
	//for (int i=0; i<length-1;i++) spi.write(MOD_NM, NM_COMP, vector[i] & 0x00FF);
	spi.write(MOD_NM, NM_LCOMP, vector[length-1] & 0x00FF);
	return(spi.read(MOD_NM, NM_NSR));
}
//-----------------------------------------------
// Learn a vector using the current context value
//----------------------------------------------
int NeuroMemAI::learn(int vector[], int length, int category)
{
	broadcast(vector, length);
	spi.write(MOD_NM, NM_CAT,category);
	return(spi.read(MOD_NM, NM_NCOUNT));
}
// ---------------------------------------------------------
// Classify a vector and return its classification status
// NSR=0, unknown
// NSR=8, identified
// NSR=4, uncertain
// ---------------------------------------------------------
int NeuroMemAI::classify(int vector[], int length)
{
	broadcast(vector, length);
	return(spi.read(MOD_NM, NM_NSR));
}
//----------------------------------------------
// Recognize a vector and return the best match, or the 
// category, distance and identifier of the top firing neuron
//----------------------------------------------
int NeuroMemAI::classify(int vector[], int length, int* distance, int* category, int* nid)
{
	broadcast(vector, length);
	*distance = spi.read(MOD_NM, NM_DIST);
	*category= spi.read(MOD_NM, NM_CAT); //remark : Bit15 = degenerated flag, true value = bit[14:0]
	*nid =spi.read(MOD_NM, NM_NID);
	return(spi.read(MOD_NM, NM_NSR));
}
//----------------------------------------------
// Recognize a vector and return the response  of up to K top firing neurons
// The response includes the distance, category and identifier of the neuron
// The Degenerated flag of the category is masked rmask the degenerated response, use the current context value
// Return the number of firing neurons or K whichever is smaller
//----------------------------------------------
int NeuroMemAI::classify(int vector[], int length, int K, int distance[], int category[], int nid[])
{
	int recoNbr=0;
	broadcast(vector, length);
	for (int i=0; i<K; i++)
	{
		distance[i] = spi.read(MOD_NM, NM_DIST);
		if (distance[i]==0xFFFF)
		{ 
			category[i]=0xFFFF;
			nid[i]=0xFFFF;
		}
		else
		{
			recoNbr++;
			category[i]= spi.read(MOD_NM, NM_CAT); //remark : Bit15 = degenerated flag, true value = bit[14:0]
			nid[i] =spi.read(MOD_NM, NM_NID);
		}
	}
return(recoNbr);
}
// ------------------------------------------------------------ 
// Set a context and associated minimum and maximum influence fields
// ------------------------------------------------------------ 
void NeuroMemAI::setContext(int context, int minif, int maxif)
{
	// context[15-8]= unused
	// context[7]= Norm (0 for L1; 1 for LSup)
	// context[6-0]= Active context value
	spi.write(MOD_NM, NM_GCR, context);
	spi.write(MOD_NM, NM_MINIF, minif);
	spi.write(MOD_NM, NM_MAXIF, maxif);
}
// ------------------------------------------------------------ 
// Get a context and associated minimum and maximum influence fields
// ------------------------------------------------------------ 
void NeuroMemAI::getContext(int* context, int* minif, int* maxif)
{
	// context[15-8]= unused
	// context[7]= Norm (0 for L1; 1 for LSup)
	// context[6-0]= Active context value
	*context = spi.read(MOD_NM, NM_GCR);
	*minif= spi.read(MOD_NM, NM_MINIF); 
	*maxif =spi.read(MOD_NM, NM_MAXIF);
}
// --------------------------------------------------------
// Set the neurons in Radial Basis Function mode (default)
//---------------------------------------------------------
void NeuroMemAI::setRBF()
{
	int tempNSR=spi.read(MOD_NM, NM_NSR);
	spi.write(MOD_NM, NM_NSR, tempNSR & 0xDF);
}
// --------------------------------------------------------
// Set the neurons in K-Nearest Neighbor mode
//---------------------------------------------------------
void NeuroMemAI::setKNN()
{
	int tempNSR=spi.read(MOD_NM, NM_NSR);
	spi.write(MOD_NM, NM_NSR, tempNSR | 0x20);
}
//-------------------------------------------------------------
// Read the contents of the neuron pointed by index in the chain of neurons
// starting at index 0
//-------------------------------------------------------------
void NeuroMemAI::readNeuron(int nid, int model[], int* context, int* minif, int* aif, int* category)
{
	int TempNSR=spi.read(MOD_NM, NM_NSR);
	spi.write(MOD_NM, NM_NSR, 0x10);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	if (nid>0)
	{
		 // move to index in the chain of neurons
		 for (int i=0; i<nid; i++) spi.read(MOD_NM, NM_CAT);
	}
	*context=spi.read(MOD_NM, NM_NCR);
	spi.readAddr(0x01000001, NEURONSIZE, model);
	*minif=spi.read(MOD_NM, NM_MINIF);
	*aif=spi.read(MOD_NM, NM_AIF);
	*category=spi.read(MOD_NM, NM_CAT);
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status
}
//-------------------------------------------------------------
// Read the contents of the neuron pointed by index in the chain of neurons
// starting index is 0
// Returns an array of integers of length NEURONSIZE + 4
// and with the following format
// NCR, NEURONSIZE * COMP, AIF, MINIF, CAT
//-------------------------------------------------------------
void NeuroMemAI::readNeuron(int nid, int neuron[])
{
	int TempNSR=spi.read(MOD_NM, NM_NSR);
	spi.write(MOD_NM, NM_NSR, 0x10);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	if (nid>0)
	{
		 // move to index in the chain of neurons
		 for (int i=0; i<nid; i++) spi.read(MOD_NM, NM_CAT);
	}
	neuron[0]=spi.read(MOD_NM, NM_NCR);
	spi.readAddr(0x01000001, NEURONSIZE, &neuron[1]);
	neuron[NEURONSIZE+1]=spi.read(MOD_NM, NM_AIF);
	neuron[NEURONSIZE+2]=spi.read(MOD_NM, NM_MINIF);
	neuron[NEURONSIZE+3]=spi.read(MOD_NM, NM_CAT);
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status
}
//----------------------------------------------------------------------------
// Read the contents of the committed neurons
// The output array has a dimension ncount * neurondata
// neurondata describes the content of a neuron and has a dimension (NEURONSIZE + 4)
// and with the following format
// NCR, NEURONSIZE * COMP, AIF, MINIF, CAT
//----------------------------------------------------------------------------
int NeuroMemAI::readNeurons(int neurons[])
{
	int ncount= spi.read(MOD_NM, NM_NCOUNT);
	int TempNSR=spi.read(MOD_NM, NM_NSR); // save value to restore upon exit
	spi.write(MOD_NM, NM_NSR, 0x0010);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	int recLen=NEURONSIZE+4; // memory plus 4 int of neuron registers
	for (int i=0; i< ncount; i++)
	{
		neurons[(i*recLen)]=spi.read(MOD_NM, NM_NCR);
		spi.readAddr(0x01000001, NEURONSIZE, &neurons[(i*recLen) + 1]);
		neurons[(i*recLen) + NEURONSIZE + 1]=spi.read(MOD_NM, NM_AIF);
		neurons[(i*recLen) + NEURONSIZE + 2]=spi.read(MOD_NM, NM_MINIF);
		neurons[(i*recLen) + NEURONSIZE + 3]=spi.read(MOD_NM, NM_CAT);
	}
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status
	return(ncount);
}

//---------------------------------------------------------------------
// Clear the neurons and write their content from an input array
// The input array has a dimension ncount * neurondata
// neurondata describes the content of a neuron and has a dimension (NEURONSIZE + 4)
// and with the following format
// NCR, NEURONSIZE * COMP, AIF, MINIF, CAT
//---------------------------------------------------------------------
void NeuroMemAI::writeNeurons(int neurons[], int ncount)
{
	int TempNSR=spi.read(MOD_NM, NM_NSR); // save value to restore NN upon exit
	int TempGCR=spi.read(MOD_NM, NM_GCR);
	clearNeurons();
	spi.write(MOD_NM, NM_NSR, 0x0010);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);		
	int recLen=NEURONSIZE+4;
	for (int i=0; i< ncount; i++)
	{	
		spi.write(MOD_NM, NM_NCR, neurons[(recLen*i)]);
		spi.writeAddr(0x01000001, NEURONSIZE, &neurons[(recLen*i)+1]);	
		spi.write(MOD_NM, NM_AIF,neurons[(recLen*i) + NEURONSIZE + 1]);
		spi.write(MOD_NM, NM_MINIF, neurons[(recLen*i) + NEURONSIZE+ 2]);	
		spi.write(MOD_NM, NM_CAT, neurons[(recLen*i) + NEURONSIZE+ 3]);
	}
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status
	spi.write(MOD_NM, NM_GCR, TempGCR);
}

// --------------------------------------------------------
// Read the number of committed neurons
//---------------------------------------------------------
int NeuroMemAI::NCOUNT()
{
	return(spi.read(MOD_NM, NM_NCOUNT));
}
// --------------------------------------------------------
// Get/Set the Minimum Influence Field register
//---------------------------------------------------------
void NeuroMemAI::MINIF(int value)
{
	spi.write(MOD_NM, NM_MINIF, value);
}
int NeuroMemAI::MINIF()
{
	return(spi.read(MOD_NM, NM_MINIF));
}
// --------------------------------------------------------
// Get/Set the Maximum Influence Field register
//---------------------------------------------------------
void NeuroMemAI::MAXIF(int value)
{
	spi.write(MOD_NM, NM_MAXIF, value);
}
int NeuroMemAI::MAXIF()
{
	return(spi.read(MOD_NM, NM_MAXIF));
}
// --------------------------------------------------------
// Get/Set the Global Context register
//---------------------------------------------------------
void NeuroMemAI::GCR(int value)
{
	// GCR[15-8]= unused
	// GCR[7]= Norm (0 for L1; 1 for LSup)
	// GCR[6-0]= Active context value
	spi.write(MOD_NM, NM_GCR, value);
}
int NeuroMemAI::GCR()
{
	return(spi.read(MOD_NM, NM_GCR));
}
// --------------------------------------------------------
// Get/Set the Category register
//---------------------------------------------------------
void NeuroMemAI::CAT(int value)
{
	spi.write(MOD_NM, NM_CAT, value);
}
int NeuroMemAI::CAT()
{
	return(spi.read(MOD_NM, NM_CAT));
}
// --------------------------------------------------------
// Get the Distance register
//---------------------------------------------------------
int NeuroMemAI::DIST()
{	
	return(spi.read(MOD_NM, NM_DIST));
}
// --------------------------------------------------------
// Set the Component Index register
//---------------------------------------------------------
void NeuroMemAI::NID(int value)
{
	spi.write(MOD_NM, NM_NID, value);
}
int NeuroMemAI::NID()
{
	return(spi.read(MOD_NM, NM_NID));
}
// --------------------------------------------------------
// Get/Set the Network Status register
// bit 2 = UNC (read only)
// bit 3 = ID (read only)
// bit 4 = SR mode
// bit 5= KNN mode
//---------------------------------------------------------
void NeuroMemAI::NSR(int value)
{
	spi.write(MOD_NM, NM_NSR, value);
}
int NeuroMemAI::NSR()
{
	return(spi.read(MOD_NM, NM_NSR));
}
// --------------------------------------------------------
// Get/Set the AIF register
//---------------------------------------------------------
void NeuroMemAI::AIF(int value)
{
	spi.write(MOD_NM, NM_AIF, value);
}
int NeuroMemAI::AIF()
{
	return(spi.read(MOD_NM, NM_AIF));
}
// --------------------------------------------------------
// Reset the chain to first neuron in SR Mode
//---------------------------------------------------------
void NeuroMemAI::RESETCHAIN()
{
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
}
// --------------------------------------------------------
// Get/Set the NCR register
//---------------------------------------------------------
void NeuroMemAI::NCR(int value)
{
	spi.write(MOD_NM, NM_NCR, value);
}
int NeuroMemAI::NCR()
{
	return(spi.read(MOD_NM, NM_NCR));
}
// --------------------------------------------------------
// Get/Set the COMP register (component)
//---------------------------------------------------------
void NeuroMemAI::COMP(int value)
{
	spi.write(MOD_NM, NM_COMP, value);
}
int NeuroMemAI::COMP()
{
	return(spi.read(MOD_NM, NM_COMP));
}
// --------------------------------------------------------
// Get/Set the LCOMP register (last component)
//---------------------------------------------------------
void NeuroMemAI::LCOMP(int value)
{
	spi.write(MOD_NM, NM_LCOMP, value);
}
int NeuroMemAI::LCOMP()
{
	return(spi.read(MOD_NM, NM_LCOMP));
}

// --------------------------------------------------------
// Save the knowledge of the neurons to a knowledge file
// saved in a format compatible with the NeuroMem API
// --------------------------------------------------------
int NeuroMemAI::saveKnowledge_SDcard(char* filename)
{
	if (!SD_detected)
	{
		SD_detected=SD.begin(SD_select);
	}
	if (!SD_detected) return(1);
	if (SD.exists(filename)) SD.remove(filename);
    File SDfile = SD.open(filename, FILE_WRITE);
    if(! SDfile) return(2);

    int header[4]{ KN_FORMAT, 0,0,0 };
    header[1] = NEURONSIZE;
    int ncount = NCOUNT();
    header[2]=ncount;
    int* p_myheader = header;
    byte* b_myheader = (byte*)p_myheader;    
    SDfile.write(b_myheader, sizeof(int)*4);

    int neuron[NEURONSIZE + 4];
    int* p_myneuron = neuron;
    byte* b_myneuron = (byte*)p_myneuron;
	int TempNSR=spi.read(MOD_NM, NM_NSR);
	spi.write(MOD_NM, NM_NSR, 0x10);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	for (int i=0; i< ncount; i++)
	{
		neuron[0]=spi.read(MOD_NM, NM_NCR);
		spi.readAddr(0x01000001, NEURONSIZE, &neuron[1]);
		neuron[NEURONSIZE+1]=spi.read(MOD_NM, NM_AIF);
		neuron[NEURONSIZE+2]=spi.read(MOD_NM, NM_MINIF);
		neuron[NEURONSIZE+3]=spi.read(MOD_NM, NM_CAT);		     
		SDfile.write(b_myneuron, sizeof(int)*(NEURONSIZE + 4));
	}
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status	
    SDfile.close();
	return(0); 
}
// --------------------------------------------------------
// Load the neurons with a knowledge stored in a knowledge file
// saved in a format compatible with the NeuroMem API
// --------------------------------------------------------
int NeuroMemAI::loadKnowledge_SDcard(char* filename)
{
	if (!SD_detected)
	{
		SD_detected=SD.begin(SD_select);
	}
	if (!SD_detected) return(1);
	if (!SD.exists(filename)) return(2); 
    File SDfile = SD.open(filename, FILE_READ);
    if (!SDfile) return(3);
	
    int header[4];
    int* p_myheader = header;
    byte* b_myheader = (byte*)p_myheader;    
    SDfile.read(b_myheader, sizeof(int)*4);  
    if (header[0] < KN_FORMAT) return(4);
    if (header[1] > NEURONSIZE) return(5);
	int ncount=0;    
    if (header[2] > navail)return(6); 
		else ncount=header[2]; // incompatible neuron size
		
    int neuron[NEURONSIZE + 4];
    int* p_myneuron = neuron;
    byte* b_myneuron = (byte*)p_myneuron; 
	int TempGCR=spi.read(MOD_NM, NM_GCR);
	int TempNSR=spi.read(MOD_NM, NM_NSR); // save value to restore NN upon exit	
	clearNeurons();
	spi.write(MOD_NM, NM_NSR, 0x0010);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
    while(SDfile.available())
	{
		int len=SDfile.read(b_myneuron, sizeof(int)*(NEURONSIZE + 4));
		if (len == sizeof(int)*(NEURONSIZE + 4))
		{
			spi.write(MOD_NM, NM_NCR, neuron[0]);
			spi.writeAddr(0x01000001, NEURONSIZE, &neuron[1]);	
			spi.write(MOD_NM, NM_AIF,neuron[NEURONSIZE + 1]);
			spi.write(MOD_NM, NM_MINIF, neuron[NEURONSIZE+ 2]);	
			spi.write(MOD_NM, NM_CAT, neuron[NEURONSIZE+ 3]);			
		}
	}
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status
	spi.write(MOD_NM, NM_GCR, TempGCR);	
	SDfile.close();
	return(0); 
}
//
// CogniSight registers
//
#define MOD_CS				0x10
#define CS_WIDTH			0x81
#define CS_HEIGHT			0x82
#define CS_FEATID			0x83
#define CS_FEATNORMALIZE	0x84
#define CS_FEATMINIF		0x85
#define CS_FEATMAXIF		0x86
#define CS_FEATPARAM1		0x87
#define CS_FEATPARAM2		0x88

struct expert {
  int width;
  int height;
  int featID;
  int normalized;
  int minif;
  int maxif;
  int featparam1;
  int featparam2;
} ;

// About the CogniSight Project format
//
//The .csp format implemented below is proprietary but not confidential.
// It includes the content of the neurons and also the description of the feature extraction
// used to generate the models stored in the neurons.
// This is necessary so the run-time or inference engine knows which feature to extract
// from pixels to compare to the models.
// The .csp format is documented in the General Vision's CogniSight SDK manual.


// --------------------------------------------------------
// Save the project to a file format compatible with the CogniSight API, SDKs and IKBs
// 
// A project file includes knowledge but also sensor and feature extraction settings
// --------------------------------------------------------
int NeuroMemAI::saveProject_SDcard(char* filename, int roiWidth, int roiHeight, int bWidth, int bHeight, int featID, int normalized)
{
	if (!SD_detected)
	{
		SD_detected=SD.begin(SD_select);
	}
	if (!SD_detected) return(1);
	if (SD.exists(filename)) SD.remove(filename);
    File SDfile = SD.open(filename, FILE_WRITE);
    if(! SDfile) return(2);
	
    byte settings[4]; 
	int Data=0;
	for (int i = 0; i < 9; i++)
	{
		switch (i)
		{
			case 0: settings[0] = MOD_CS; settings[1] = CS_WIDTH; Data = roiWidth; break;
			case 1: settings[0] = MOD_CS; settings[1] = CS_HEIGHT; Data = roiHeight; break;
			case 2: settings[0] = MOD_CS; settings[1] = CS_FEATID; Data = featID; break;
			case 3: settings[0] = MOD_CS; settings[1] = CS_FEATNORMALIZE; Data = normalized; break;
			case 4: settings[0] = MOD_CS; settings[1] = CS_FEATPARAM1; Data = bWidth; break;
			case 5: settings[0] = MOD_CS; settings[1] = CS_FEATPARAM2; Data = bHeight; break;
			case 6: settings[0] = MOD_NM; settings[1] = NM_MINIF; Data = MINIF(); break;
			case 7: settings[0] = MOD_NM; settings[1] = NM_MAXIF; Data = MAXIF(); break;
			case 8: settings[0] = 0xFF; settings[1] = 0xFF; break; //end of file marker
			default: break;
		}
		settings[2] = (byte)((Data & 0xFF00) >> 8);
		settings[3] = (byte)(Data & 0x00FF);
		SDfile.write(settings, 4);
	}
	
    int neuron[NEURONSIZE + 4];
    int* p_myneuron = neuron;
    byte* b_myneuron = (byte*)p_myneuron;
	int ncount = NCOUNT();
	int TempNSR=spi.read(MOD_NM, NM_NSR);
	spi.write(MOD_NM, NM_NSR, 0x10);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	for (int i=0; i< ncount; i++)
	{
		neuron[0]=spi.read(MOD_NM, NM_NCR);
		spi.readAddr(0x01000001, NEURONSIZE, &neuron[1]);
		neuron[NEURONSIZE+1]=spi.read(MOD_NM, NM_AIF);
		neuron[NEURONSIZE+2]=spi.read(MOD_NM, NM_MINIF);
		neuron[NEURONSIZE+3]=spi.read(MOD_NM, NM_CAT);		     
		SDfile.write(b_myneuron, sizeof(int)*(NEURONSIZE + 4));
	}
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status	
    SDfile.close();
	return(0); 
}
// --------------------------------------------------------
// Load the project from a file format compatible with the CogniSight API, SDKs and IKBs
//
// A project file includes knowledge but also sensor and feature extraction settings
// --------------------------------------------------------
int NeuroMemAI::loadProject_SDcard(char* filename, int* roiWidth, int* roiHeight, int* bWidth, int* bHeight, int* featID, int* normalized)
{
	if (!SD_detected)
	{
		SD_detected=SD.begin(SD_select);
	}
	if (!SD_detected) return(1);
	if (!SD.exists(filename)) return(2); 
    File SDfile = SD.open(filename, FILE_READ);
    if (!SDfile) return(3);

	byte settings[4];
	int minif=2, maxif=0x4000;
	int Data, Temp;
	int iter=100; // to limit the number of readings from the header file
		
	do
	{
		SDfile.read(settings, 4); 
		Data = (settings[2] << 8) + settings[3];
		Temp = (settings[0] << 8) + settings[1];
		if (Temp != 0xFFFF)
		{
			if (settings[0] == MOD_NM)
			{
				switch (settings[1]) // to apply the settings to the API variables
				{
				case NM_MINIF: minif = Data; break;
				case NM_MAXIF: maxif = Data; break;
				default: break;
				}
			}
			else if (settings[0] == MOD_CS)
			{
				switch (settings[1]) // to apply the settings to the variables of the API
				{
					case CS_WIDTH: *roiWidth = Data; break;
					case CS_HEIGHT: *roiHeight = Data; break;
					case CS_FEATPARAM1: *bWidth = Data; break;
					case CS_FEATPARAM2: *bHeight = Data; break;
					case CS_FEATID: *featID = Data; break;
					case CS_FEATNORMALIZE: *normalized = Data; break;
					case CS_FEATMINIF: minif=Data; break;
					case CS_FEATMAXIF: maxif=Data; break;
					default: break;
				}
			}
		}
		iter++;
	} while ((Temp != 0xFFFF) & (iter>0));
  	if (iter==0) return(4);
	
    int neuron[NEURONSIZE + 4];
    int* p_myneuron = neuron;
    byte* b_myneuron = (byte*)p_myneuron; 

	int TempGCR=spi.read(MOD_NM, NM_GCR);
	int TempNSR=spi.read(MOD_NM, NM_NSR); // save value to restore NN upon exit	
	clearNeurons();
	spi.write(MOD_NM, NM_NSR, 0x0010);
	spi.write(MOD_NM, NM_RESETCHAIN, 0);
	while(SDfile.available()>0)
	{
		int len=SDfile.read(b_myneuron, sizeof(int)*(NEURONSIZE + 4));
		if (len == sizeof(int)*(NEURONSIZE + 4))
		{
			spi.write(MOD_NM, NM_NCR, neuron[0]);
			spi.writeAddr(0x01000001, NEURONSIZE, &neuron[1]);
			spi.write(MOD_NM, NM_AIF,neuron[NEURONSIZE + 1]);
			spi.write(MOD_NM, NM_MINIF, neuron[NEURONSIZE+ 2]);	
			spi.write(MOD_NM, NM_CAT, neuron[NEURONSIZE+ 3]);
		}
	}
	spi.write(MOD_NM, NM_NSR, TempNSR); // set the NN back to its calling status
	spi.write(MOD_NM, NM_GCR, TempGCR);
	spi.write(MOD_NM, NM_MINIF, minif);
	spi.write(MOD_NM, NM_MAXIF, maxif);
		
	SDfile.close();
	return(0); 
}

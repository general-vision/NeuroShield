// NeuroMem.cpp
// Copyright 2019 General Vision Inc.

#include <Windows.h>
#include "stdlib.h"	 //for calloc
#include "string.h"  //for memcpy
#include "GV_comm.h"

extern int platform; // initialized in the comm_xyz.cpp
extern int maxveclength;// initialized in the comm_xyz.cpp


// --------------------------------------------------------------
// Initialize communication with the NeuroMem platform and
// return the number of neurons detected in the network
// --------------------------------------------------------------
int InitializeNetwork()
{
	int navail = 0;
	int error=Connect(0);
	if (error == 0)
	{
		Write(MOD_NM, NM_FORGET, 0);
		Write(MOD_NM, NM_NSR, 0x0010);
		Write(MOD_NM, NM_TESTCAT, 0x0001);
		Write(MOD_NM, NM_RESETCHAIN, 0);
		int read_cat;
		navail = 0;
		while (1) {
			read_cat = Read(MOD_NM, NM_CAT);
			if (read_cat == 0xFFFF)
				break;
			navail++;
		}
		Write(MOD_NM, NM_NSR, 0x0000);
		Write(MOD_NM, NM_FORGET, 0);
	}
	return(navail);
}

// --------------------------------------------------------------
// Clear the memory of the neurons to the value 0
// Reset default NM_GCR=1, NM_MINIF=2, MANIF=0x4000, NM_CAT=0
// --------------------------------------------------------------
void ClearNeurons()
{
	Write(1, NM_NSR, 16);
	Write(1, NM_TESTCAT, 0x0001);
	Write(1, NM_NSR, 0);
	for (int i = 0; i< maxveclength; i++)
	{
		Write(1, NM_INDEXCOMP, i);
		Write(1, NM_TESTCOMP, 0);
	}
	Write(1, NM_FORGET, 0);
}

//-----------------------------------------------
// Return the number of committed neurons
//----------------------------------------------
int GetCommitted()
{
	return(Read(MOD_NM, NM_NCOUNT));
}

//-----------------------------------------------
// Uncommit the neurons
// option to change the default NM_MAXIF
//----------------------------------------------
void Forget()
{
	Write(MOD_NM,NM_FORGET,0);
}
void Forget(int Maxif)
{
	Write(MOD_NM,NM_FORGET,0);
	Write(MOD_NM,NM_MAXIF, Maxif);
}
// --------------------------------------------------------
// Broadcast a vector to the neurons and return the recognition status
// 0= unknown, 4=uncertain, 8=Identified
//---------------------------------------------------------
int Broadcast(int* vector, int length)
{
	if (length>maxveclength) length = maxveclength;
	if (length == 1) Write(MOD_NM, NM_LCOMP, vector[0]);
	else
	{
		if (platform == 0)
		// case of simulation
		{
			for (int i = 0; i < length - 1; i++) Write(MOD_NM, NM_COMP, vector[i]);
		}
		else
		{
			// case of hardware supported Read/Write of packets
			int l = length * 2;
			unsigned char* vectorB = new unsigned char[l];
			for (int i = 0; i < length; i++) 
			{
				vectorB[i * 2] = 0;
				vectorB[(i * 2) + 1] = vector[i];
			}
			Write_Addr(0x01000001, l - 2, vectorB);
		}
		Write(MOD_NM, NM_LCOMP, vector[length - 1]);
	}
	return(Read(MOD_NM, NM_NSR));
}
//-----------------------------------------------
// Learn a vector using the current context value
//----------------------------------------------
int Learn(int* vector, int length, int category)
{
	Broadcast(vector, length);
	Write(MOD_NM, NM_CAT,category);
	return(Read(MOD_NM,NM_NCOUNT));
}
//----------------------------------------------
// Recognize a vector and return the response of the top firing neuron
// category (wo/ DEG flag), distance and identifier
//----------------------------------------------
int BestMatch(int* vector, int length, int* distance, int* category, int* nid)
{
	Broadcast(vector, length);
	*distance = Read(MOD_NM, NM_DIST);
	*category= Read(MOD_NM, NM_CAT) & 0x7FFF; 
	*nid =Read(MOD_NM, NM_NID);
	return(Read(MOD_NM, NM_NSR));
}
//----------------------------------------------
// Recognize a vector and return the response  of up to K top firing neurons
// The response includes the distance, category and identifier of the neuron
// The Degenerated flag of the category is masked
// Return the number of firing neurons or K whichever is smaller
//----------------------------------------------
int Recognize(int* vector, int length, int K, int distance[], int category[], int nid[])
{
	Broadcast(vector, length);
	int recoNbr=0;
	for (int i=0; i<K; i++)
	{
		distance[i] = Read(MOD_NM, NM_DIST);
		if (distance[i]==0xFFFF)
		{ 
			category[i]=0xFFFF;
			nid[i]=0xFFFF;
		}
		else
		{
			recoNbr++;
			category[i]= Read(MOD_NM, NM_CAT) & 0x7FFF;
			nid[i] =Read(MOD_NM, NM_NID);
		}
	}
	return(recoNbr);
}
// ------------------------------------------------------------ 
// Set a context and associated minimum and maximum influence fields
// ------------------------------------------------------------ 
void setContext(int context, int minif, int maxif)
{
	// context[15-8]= unused
	// context[7]= Norm (0 for L1; 1 for LSup)
	// context[6-0]= Active context value
	Write(MOD_NM, NM_GCR, context);
	Write(MOD_NM, NM_MINIF, minif);
	Write(MOD_NM, NM_MAXIF, maxif);
}
// ------------------------------------------------------------ 
// Get a context and associated minimum and maximum influence fields
// ------------------------------------------------------------ 
void getContext(int* context, int* minif, int* maxif)
{
	// context[15-8]= unused
	// context[7]= Norm (0 for L1; 1 for LSup)
	// context[6-0]= Active context value
	*context = Read(MOD_NM, NM_GCR);
	*minif = Read(MOD_NM, NM_MINIF);
	*maxif = Read(MOD_NM, NM_MAXIF);
}
// --------------------------------------------------------
// Set the neurons in Radial Basis Function mode (default)
//---------------------------------------------------------
void setRBF()
{
	int tempNSR = Read(MOD_NM, NM_NSR);
	Write(MOD_NM, NM_NSR, tempNSR & 0xDF);
}
// --------------------------------------------------------
// Set the neurons in K-Nearest Neighbor mode
//---------------------------------------------------------
void setKNN()
{
	int tempNSR = Read(MOD_NM, NM_NSR);
	Write(MOD_NM, NM_NSR, tempNSR | 0x20);
}

//--------------------------------------------------------------------------------------
// Read the content of a specific neuron
// Warning: neuuons are indexed in the chain starting at 1
// if index is greater than the number of committed neurons, read the RTL neuron
//--------------------------------------------------------------------------------------
void ReadNeuron(int neuronID, int* ncr, int model[], int* aif, int* minif, int* category)
{
	int ncount = Read(1, NM_NCOUNT);
	if ((neuronID <= 0) | (neuronID > ncount))
	{
		*ncr = 0; *aif = 0; *minif = 0; *category = 0;
		for (int i = 0; i<maxveclength; i++) model[i] = 0;
		return;
	}
	int Temp = 0;
	unsigned char* modelB = new unsigned char[maxveclength *2];
	int TempNSR = Read(1, NM_NSR);
	Write(1, NM_NSR, 0x0010);
	Write(1, NM_RESETCHAIN, 0);
	if (neuronID > 1) { for (int i = 0; i < neuronID - 1; i++) Temp = Read(1, NM_CAT); }
	*ncr = Read(1, NM_NCR);
	if (platform == 0)
	{
		for (int i = 0; i<maxveclength; i++) model[i] = Read(1, NM_COMP);
	}
	else
	{
		Read_Addr(0x01000001, maxveclength*2, modelB);
		for (int i = 0; i < maxveclength; i++) model[i] = modelB[i*2 + 1];
	}	
	*aif = Read(1, NM_AIF);
	*minif = Read(1, NM_MINIF);
	*category = Read(1, NM_CAT);
	Write(1, NM_NSR, TempNSR);
	return;
}

void ReadNeuron(int neuronID, int neuron[])
{
	int ncount = Read(1, NM_NCOUNT);
	if ((neuronID <= 0) | (neuronID > ncount))
	{
		for (int i = 0; i<maxveclength+4; i++) neuron[i] = 0;
		return;
	}
	int Temp = 0;
	unsigned char* model = new unsigned char[maxveclength*2];
	int TempNSR = Read(1, NM_NSR);
	Write(1, NM_NSR, 0x0010);
	Write(1, NM_RESETCHAIN, 0);
	if (neuronID > 1) { for (int i = 0; i < neuronID - 1; i++) Temp = Read(1, NM_CAT); }
	neuron[0] = Read(1, NM_NCR);
	if (platform == 0)
	{
		for (int i = 0; i<maxveclength; i++) neuron[i+1] = Read(1, NM_COMP);
	}
	else
	{
		Read_Addr(0x01000001, maxveclength * 2, model);
		for (int i = 0; i < maxveclength; i++) neuron[i+1] = model[(i * 2) + 1];
	}
	neuron[maxveclength+1] = Read(1, NM_AIF);
	neuron[maxveclength+2] = Read(1, NM_MINIF);
	neuron[maxveclength+3] = Read(1, NM_CAT);
	Write(1, NM_NSR, TempNSR);
	return;
}
//-------------------------------------------------------------
// Read the contents of the neurons
//-------------------------------------------------------------
int ReadNeurons(int *neurons)
{
	int ncount = Read(1, NM_NCOUNT);
	memset(neurons,0, ncount*(maxveclength + 4)*sizeof(int));
	int* neuron=new int[maxveclength+4];
	unsigned char* neuronB = new unsigned char[maxveclength*2 + 4];
	if (ncount == 0) return(0);
	else
	{
		int TempNSR = Read(1, NM_NSR);
		Write(1, NM_NSR, 16);
		Write(1, NM_RESETCHAIN, 0);
		for (int i = 0; i < ncount; i++)
		{
			neuron[0] = Read(1, NM_NCR);
			if (platform == 0)
			{
				for (int i = 0; i<maxveclength; i++) neuron[i+1] = Read(1, NM_COMP);
			}
			else
			{
				Read_Addr(0x01000001, maxveclength * 2, neuronB);
				for (int i = 0; i < maxveclength; i++) neuron[i+1] = neuronB[i * 2 + 1];
			}
			neuron[maxveclength + 1] = Read(1, NM_AIF);
			neuron[maxveclength + 2] = Read(1, NM_MINIF);
			neuron[maxveclength + 3] = Read(1, NM_CAT);
			memcpy(neurons + (i*(maxveclength + 4)), neuron, (maxveclength + 4) * sizeof(int));
		}
		Write(1, NM_NSR, TempNSR);
		return(ncount);
	}
}
//-------------------------------------------------------------
// load the neurons' content from file
//-------------------------------------------------------------
int WriteNeurons(int *neurons, int ncount)
{
	int TempNSR = Read(1, NM_NSR);
	ClearNeurons();
	Write(1, NM_NSR, 0x0010);
	Write(1, NM_RESETCHAIN, 0);
	int* neuron = new int[maxveclength + 4];
	unsigned char* neuronB = new unsigned char[(maxveclength * 2) + 4];
	for (int i = 0; i<ncount; i++)
	{
		memcpy(neuron, neurons + (i*(maxveclength + 4)), (maxveclength + 4) * sizeof(int));
		Write(1, NM_NCR, neuron[0]);
		if (platform == 0)
		{
			for (int i = 0; i<maxveclength; i++) Write(1, NM_COMP, neuron[i + 1]);
		}
		else
		{
			for (int i = 0; i < maxveclength; i++) {
				neuronB[i * 2] = 0;  neuronB[i * 2 + 1] = neuron[i + 1];
			}
			Write_Addr(0x01000001, maxveclength * 2, neuronB);
		}
		Write(1, NM_AIF, neuron[maxveclength + 1]);
		Write(1, NM_MINIF, neuron[maxveclength + 2]);
		Write(1, NM_CAT, neuron[maxveclength + 3]);
	}
	Write(1, NM_NSR, TempNSR);
	return(Read(1, NM_NCOUNT));
}


// --------------------------------------------------------------
// Functions for interfacing with integer pointers in python
// --------------------------------------------------------------
/*
int *new_int(int ivalue)
{
	int *i = (int *)malloc(sizeof(ivalue));
	*i = ivalue;
	return i;
}

int get_int(int *i)
{
	return *i;
}

void delete_int(int *i)
{
	free(i);
}
*/

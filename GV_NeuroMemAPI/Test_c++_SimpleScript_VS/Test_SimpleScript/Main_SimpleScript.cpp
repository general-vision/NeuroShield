//------------------------------------------------------------
//
// Main_SimpleScript
// Copyright 2019 General Vision Inc.
//
// This academic script is described in the following document page 15
// https://www.general-vision.com/documentation/TM_NeuroMem_API.pdf
//
// The NeuroMem platform is defined by the file comm_xyz.cpp included in the project.
//
//------------------------------------------------------------
//
#include "stdio.h" //for printf
#include "../../lib/neuromem/NeuroMem.h"

// this variable is defined by the selected NeuroMem platform
// and corresponds to the maximum vector length fitting in the neurons' memory
extern int maxveclength;

void DisplayNeurons(int dispLen)
{
	int nc = GetCommitted();
	int* neurondata = new int[nc * (maxveclength + 4)];
	ReadNeurons(neurondata);
	int k1 = 0;
	for (int j = 0; j < nc; j++)
	{
		printf("\nNCR= %u COMP= ", neurondata[k1]);
		for (int i = 0; i < dispLen; i++) printf("%3u, ", neurondata[k1 + 1 + i]);
		k1 += maxveclength;
		printf(" MIF= %6u ", neurondata[k1 + 2]);
		printf(" AIF= %6u ", neurondata[k1 + 1]);
		printf(" CAT= %6u", neurondata[k1 + 3]);
		k1 += 4;
	}
	delete[] neurondata;
}

int main()
{
	int gcr = 0, minif = 0, maxif = 0, ncr = 0, comp = 0, dist = 0xFFFF, aif = 0xFFFF, cat = 0xFFFF, nid = 0;
	int responseNbr = 0;

	int navail = InitializeNetwork();
	if (navail == 0) 
	{	
		printf("Did not detect the NeuroMem platform!");
		getchar();
		return -1;
	}
	printf("\nAvailable neurons: \t%u", navail);
	getContext(&gcr, &minif, &maxif);
	printf("\nContext = \t%u \nMinif = \t%u \nMaxif = \t%u", gcr, minif, maxif);

	int veclen = 4; // length of the vector patterns in this example
	int displen = veclen + 2; // length of the models retrieved from the neurons for display (added 2 spare components after veclen on purpose)

	int* vector= new int[veclen];
	int ncount=0;

	//---------------------------------
	// Learning Phase
	//---------------------------------
	printf("\n\nLearning 3 vectors...\n");

	//Learn first vector as category 55
	for (int i=0; i<veclen; i++) vector[i]=11;
	Learn(vector, veclen, 55);

	//Learn second vector as category 33
	for (int i = 0; i < veclen; i++) vector[i]=15;
	Learn(vector,veclen, 33);

	//Learn third vector as category 100
	for (int i = 0; i < veclen; i++) vector[i]=20;
	ncount=Learn(vector,veclen, 100);

	DisplayNeurons(displen);

	// Save the knowledge or content of the neurons to an array kn1
	int * kn1 = new int[ncount * (maxveclength + 4)];
	int nc1 = ReadNeurons(kn1);

	//---------------------------------
	// Recognition phase
	//---------------------------------
	printf("\n\nRecognizing vectors...");
	printf("\nType any key to continue"); getchar();
	//
	// Default network setting is RBF. If you want to set the KNN mode, uncomment the following line.
	// Write(MOD_NM,NSR,32);
	//
	int k=5;
	int* distList=new int[k];
	int* catList=new int[k];
	int* nidList=new int[k];

	//Recognize first vector
	//example using Recognize function
	printf("\n\nCase of uncertainty, closer from Neuron#1");
	for (int i = 0; i < veclen; i++) vector[i]=12;
	printf("\nVector="); for (int i=0;i<veclen;i++) printf("%u, ",vector[i]);
	responseNbr=Recognize(vector,4, k, distList, catList, nidList);		
	for (int i=0; i<responseNbr; i++)
		printf("\nNeuronID %u \tDistance= %u \tCategory= %u", nidList[i], distList[i], catList[i]);

	//Recognize second vector
	//example using Recognize function
	printf("\n\nCase of uncertainty, closer from Neuron#2");
	for (int i = 0; i < veclen; i++) vector[i]=14;
	printf("\nVector="); for (int i=0;i<veclen;i++) printf("%u, ",vector[i]);
	responseNbr = Recognize(vector, 4, k, distList, catList, nidList);
	for (int i = 0; i < responseNbr; i++)
		printf("\nNeuronID %u \tDistance= %u \tCategory= %u", nidList[i], distList[i], catList[i]);

	//Recognize third vector 
	//example using BestMatch function
	printf("\n\nCase of unknown");
	for (int i = 0; i < veclen; i++) vector[i]=30;
	printf("\nVector="); for (int i=0;i<veclen;i++) printf("%u, ",vector[i]);
	BestMatch(vector, 4, &dist, &cat, &nid);		
	printf("\nNeuronID %u \tDistance= %u \tCategory= %u", nid, dist, cat);

	//Recognize fourth vector
	//example using the Recognize function
	printf("\n\nCase of uncertainty, equi-distant to Neuron#1 and Neuron#2");
	for (int i = 0; i < veclen; i++) vector[i]=13;
	printf("\nVector="); for (int i=0;i<veclen;i++) printf("%u, ",vector[i]);
	responseNbr =Recognize(vector, veclen, k, distList, catList, nidList);
	for (int i=0; i<responseNbr; i++)
		printf("\nNeuronID %u \tDistance= %u \tCategory= %u", nidList[i], distList[i], catList[i]);

	//-----------------------------------------
	//Learn a fourth vector shrinking 2 neurons
	//-----------------------------------------
	printf("\n\nLearning a new vector to illustrate the reduction of neurons'AIFs");
	printf("\nType any key to continue\n"); getchar();

	for (int i=0;i<veclen;i++) vector[i]=13;
	ncount=Learn(vector, veclen, 100);
	DisplayNeurons(displen);

	// Restore the knowledge kn1 prior to commitment of 4th neuron
	// just to test the WriteNeurons
	WriteNeurons(kn1, nc1);
	printf("\n\nRestore the previous knowledge\n");
	DisplayNeurons(displen);

	printf("\n\nEnd of test...\n\n");
	printf("\nType any key to exit\n"); getchar();
	
	return(0);
}

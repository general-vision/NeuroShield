// NeuroMem.h
// copyright 2019 General Vision Inc.

int InitializeNetwork();
void Forget();
void Forget(int Maxif);
void ClearNeurons();
int GetCommitted();

//Operations on single vector
int Broadcast(int* vector, int length);
int Learn(int* vector, int length, int category);
int BestMatch(int* vector, int length, int* distance, int* category, int* nid);
int Recognize(int* vector, int length, int K, int distance[], int category[], int nid[]);

void setContext(int context, int minif, int maxif);
void getContext(int* context, int* minif, int* maxif);
void setRBF();
void setKNN();
		
//Loading and retrieval of neurons' content
int ReadNeurons(int *neurons);
int WriteNeurons(int *neurons, int ncount);
void ReadNeuron(int nid, int neuron[]);
void ReadNeuron(int nid, int* ncr, int model[], int* aif, int* minif, int* category);

// for compatibility with pointers in python
//int *new_int(int ivalue);
//int get_int(int *i);
//void delete_int(int *i);
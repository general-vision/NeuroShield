## Simple neuron test
## copyright(c) 2018 General Vision

import sys
import ctypes
import NeuroMem as nm
import GVcomm_SPI as comm
from GVconstants import *

if (comm.Connect()!=0):
  print ("Cannot connect to NeuroShield\n")
  sys.exit()

# for test purposes
print("\nClear Knowledge")
nm.ClearNeurons()
nm.DisplayNeurons(5)

#print("\nLearning three vectors of LENGTH components with identical values")
LENGTH=4
bytearray = ctypes.c_int * LENGTH
vector=bytearray()

for i in range(0,LENGTH):
  vector[i]=11
nm.Learn(vector, LENGTH, 33)

for i in range(0,LENGTH):
  vector[i]=15
nm.Learn(vector, LENGTH, 55)

for i in range(0,LENGTH):
  vector[i]=20
nm.Learn(vector, LENGTH, 100)
nm.DisplayNeurons(5)

print("\nRecognition of new vectors")

# initializing the variables to hold the result of a recognition
# with up to k responses of the top firing neurons, if applicable
k=3
bytearray = ctypes.c_int * k
dists=bytearray()
cats=bytearray()
nids=bytearray()

print("\nVector of components equal to 14. Uncertainty, but closer to neuron 2")
for i in range(0,LENGTH):
  vector[i]=14
respNbr, dists, cats, nids = nm.Classify(vector, LENGTH, k)
for i in range(0,respNbr):
  print("\tdist= %u, \tcat= %u, \tnid= %u" % (dists[i],cats[i],nids[i]))

print("\nVector of components equal to 12. Uncertainty, but closer to neuron 1")
for i in range(0,LENGTH):
  vector[i]=12
respNbr, dists, cats, nids = nm.Classify(vector, LENGTH, k)
for i in range(0,respNbr):
  print("\tdist= %u, \tcat= %u, \tnid= %u" % (dists[i],cats[i],nids[i]))

print("\nVector of components equal to 6. Unknown")
for i in range(0,LENGTH):
  vector[i]=6
respNbr, dists, cats, nids = nm.Classify(vector, LENGTH, k)
for i in range(0,respNbr):
  print("\tdist= %u, \tcat= %u, \tnid= %u" % (dists[i],cats[i],nids[i]))

print("\nVector of components equal to 13. Uncertainty, equi-distant to neuron 1 and 2")
for i in range(0,LENGTH):
  vector[i]=13
respNbr, dists, cats, nids = nm.Classify(vector, LENGTH, k)
for i in range(0,respNbr):
  print("\tdist= %u, \tcat= %u, \tnid= %u" % (dists[i],cats[i],nids[i]))

print("\nLearn this same vector with a category 100")
nm.Learn(vector, LENGTH, 100)

nm.DisplayNeurons(5)
print("\nNotice the new neuron #4 and the reduction of AIFs of neurons #1 and #2\n")

print("\nThe End\n")
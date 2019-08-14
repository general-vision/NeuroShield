## Simple Imaging Example
## copyright(c) 2018 General Vision
##
## Academic example!
## Learn an example extracted from the center region of a first frame
## Monitor this region in subsequent frames
## and report its category and distance to the learned example

import sys
import ctypes
import NeuroMem as nm
import GVcomm_SPI as comm

from picamera.array import PiRGBArray
from picamera import PiCamera
import time
import cv2 as cv

LENGTH=256
bytearray = ctypes.c_int * LENGTH
vector=bytearray()

#-----------------------------------------------------
# Extract a subsample of the ROI at the location X,Y
# using a block size bW*bH and amplitude normalization on or off
# return the length of the output vector
#-----------------------------------------------------
def GetGreySubsample(image, roiL, roiT, roiW, roiH, bW, bH, normalize):
  # subsample blocks of BWxBH pixels from the ROI [Left, Top, Width, Height]
  # vector is the output to broadcast to the neurons for learning or recognition
  Left=int(roiL)
  Top=int(roiT)
  Width=int(roiW)
  Height=int(roiH)
  p = 0
  for y in range(Top, Top + Height, bH):
    for x in range(Left, Left + Width, bW):
      Sum=0	
      for yy in range(0, bH):
        for xx in range (0, bW):
          # to adjust if monochrome versus rgb image array
          Sum += image[y+yy, x+xx]
      vector[p] = (int)(Sum / (bW*bH))

      #log the min and max component
      min = 255
      max = 0
      if (max < vector[p]):
        max = vector[p]
      if (min > vector[p]):
        min = vector[p]
      p=p+1
    
  if ((normalize == 1) & (max > min)):
    for i in range (0, p):
      Sum= (vector[i] - min) * 255
      vector[i] = (int)(Sum / (max - min))

  # return the length of the vector which must be less or equal to 256
  return p, vector




#-------------------------------------------------
# Select a NeuroMem platform
# 0=simu, 1=NeuroStack, 2=NeuroShield, 4=Brilliant
#-------------------------------------------------
if (comm.Connect()!=0):
  print ("Cannot connect to NeuroShield\n")
  sys.exit()
nm.ClearNeurons()

# initialize the camera and grab a reference image
camera = PiCamera()
camera.resolution=(600,400)
time.sleep(2) # camera warm up
rawCapture = PiRGBArray(camera)

# grab an image from the camera
camera.capture(rawCapture, format="bgr")
imsrc = rawCapture.array

# define center of image
imW=imsrc.shape[1]
imH=imsrc.shape[0]
print("Image = " + repr(imW) + " x " + repr(imH))

# define the Region of Interest (roi) to learn
roiW = 128
roiH = 128
roiL= int(imW/2 - roiW/2)
roiT= int(imH/2 - roiH/2)

print("Learning center Region Of Interest")
cv.rectangle(imsrc,(roiL, roiT),(roiL+roiW, roiT+roiH),(255,0,0),1)
cv.imshow('Video', imsrc)
cv.waitKey(3)

# convert to grey-level for the analytics
imgl = cv.cvtColor(imsrc, cv.COLOR_BGR2GRAY)
bW = int(8)
bH = int(8)
normalize = int(1)

print("Type a target # when ready between[1-9]")

font                   = cv.FONT_HERSHEY_SIMPLEX
fontScale              = 0.5
fontColor              = (255,0,0)
lineType               = 1
while (1==1):
  rawCapture = PiRGBArray(camera)
  camera.capture(rawCapture, format="bgr")
  imsrc = rawCapture.array
  imgl = cv.cvtColor(imsrc, cv.COLOR_BGR2GRAY)
  vlen, vector =GetGreySubsample(imgl, roiL, roiT, roiW, roiH, bW, bH, normalize)
  dist, cat, nid = nm.BestMatch(vector, vlen)
  if cat==65535:
    roiLabel=""
  else:
    roiLabel= "Cat " + str(cat) + " @ Distance " + str(dist)
  cv.rectangle(imsrc,(roiL, roiT),(roiL+roiW, roiT+roiH),(255,0,0),1)
  cv.putText(imsrc,roiLabel, (roiL, roiT - 10), font, fontScale, fontColor,lineType)
  cv.imshow('Video', imsrc)
  c=cv.waitKey(1)
  if (c > 48) & (c < 57):
    catL= c-48
    print("Learning target " + str(catL))
    # Learn ROI at center of the image
    vlen, vector=GetGreySubsample(imgl, roiL, roiT, roiW, roiH, bW, bH, normalize)
    nm.Learn(vector,vlen,catL)
    # Learn a counter example ROI at an offset of roi/2 right and down
    vlen, vector=GetGreySubsample(imgl, roiL + roiW/2, roiT + roiT/2, roiW, roiH, bW, bH, normalize)
    ncount = nm.Learn(vector,vlen,0)
    print("ncount =" + repr(ncount))

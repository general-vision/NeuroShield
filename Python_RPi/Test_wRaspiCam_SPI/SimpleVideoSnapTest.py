## Simple Imaging test
## copyright(c) 2018 General Vision
##
## Academic example!
## Take a snapshot, learn its center and
## recognize similar pattern within a larger region

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

#----------------------------------------------
# Scan a region of scan (ROS) with a step XY
# recognize a region of interest at each position
# return the list of XY positions where a known pattern
# was recognized by the neurons and which category was recognized
#----------------------------------------------
posX=[]
posY=[]
cats=[]
def surveyROS(image, rosL, rosT, rosW, rosH, stepX, stepY):
  objNbr=0
  for y in range(rosT, rosT + rosH, stepY):
    for x in range(rosL, rosL + rosW, stepX):
      vlen, vector=GetGreySubsample(image, x, y, roiW, roiH, bW, bH, normalize)
      dist, cat, nid = nm.BestMatch(vector, vlen)
      if (dist!=0xFFFF):
        # store the position and category if the neurons recognize something
        posX.append(x)
        posY.append(y)
        cats.append(cat)
        objNbr=objNbr+1
  return(objNbr)
#-----------------------------------------------------
# Extract a subsample of the ROI at the location X,Y
# using a block size bW*bH and amplitude normalization on or off
# return the length of the output vector
#-----------------------------------------------------
def GetGreySubsample(image, roiL, roiT, roiW, roiH, bW, bH, normalize):
  # subsample blocks of BWxBH pixels from the ROI [Left, Top, Width, Height]
  # vector is the output to broadcast to the neurons for learning or recognition
  p = 0
  for y in range(roiT, roiT + roiH, bH):
    for x in range(roiL, roiL + roiW, bW):
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
#camera.start_preview(alpha=128)
time.sleep(2) # camera warm up
rawCapture = PiRGBArray(camera)

# grab an image from the camera
camera.capture(rawCapture, format="bgr")
imsrc = rawCapture.array

# define center of image
imW=imsrc.shape[1]
imH=imsrc.shape[0]
print("Image = " + repr(imW) + " x " + repr(imH))
imCtrX= imW/2
imCtrY= imH/2

# define the Region of Interest (roi) to learn
roiW = int(64)
roiH = int(64)
roiL= int(imCtrX - roiW/2)
roiT= int(imCtrY - roiH/2)

print("Learning center Region Of Interest")
cv.rectangle(imsrc,(roiL, roiT),(roiL+roiW, roiT+roiH),(255,0,0),1)
cv.imshow('Video', imsrc)
cv.waitKey(3)

# convert to grey-level for the analytics
imgl = cv.cvtColor(imsrc, cv.COLOR_BGR2GRAY)
bW = int(4)
bH = int(4)
normalize = int(1)

# Learn ROI at center of the image
vlen, vector=GetGreySubsample(imgl, roiL, roiT, roiW, roiH, bW, bH, normalize)
nm.Learn(vector,vlen,1)

# Learn a counter example ROI at an offset of 10 pixels right and down
vlen, vector=GetGreySubsample(imgl, roiL + 10, roiT + 10, roiW, roiH, bW, bH, normalize)
ncount = nm.Learn(vector,vlen,0)
print("ncount =" + repr(ncount))

print("Recognizing around center")

# Recognize ROI at same position, expect a distance of 0
vlen, vector =GetGreySubsample(imgl, roiL, roiT, roiW, roiH, bW, bH, normalize)
dist, cat, nid = nm.BestMatch(vector, vlen)
print("Offset = 00: Reco cat = " + repr(cat) + " at dist =" + repr(dist))

# Recognize ROI at an offset of 5 pixels left and down, expect a distance > 0
vlen, vector =GetGreySubsample(imgl, roiL + 5, roiT + 5, roiW, roiH, bW, bH, normalize)
dist, cat, nid = nm.BestMatch(vector, vlen)
print("Offset = 05: Reco cat = " + repr(cat) + " at dist =" + repr(dist))

# Recognize ROI at an offset of 12 pixels left and down, expect a distance 0xFFFF
# due to counter example taught at offset =10
vlen, vector =GetGreySubsample(imgl, roiL + 12, roiT + 12, roiW, roiH, bW, bH, normalize)
dist, cat, nid = nm.BestMatch(vector, vlen)
print("Offset = 12: Reco cat = " + repr(cat) + " at dist =" + repr(dist))

# Look for similar ROI over a larger ROS (Region Of Search) representing
# one third of the whole image and centered in the image
print("Searching for similar objects..." )

# define the Region of Search (ros)
rosW= int(imW/4)
rosH= int(imH/4)
rosL= int(imCtrX - rosW/2)
rosT= int(imCtrY - rosH/2)
stepX = int(8)
stepY = int(8)

objNbr=surveyROS(imgl, rosL, rosT, rosW, rosH, stepX, stepY)
print("Recognized objects: " + repr(objNbr))
for i in range (0,objNbr):
  cv.circle(imsrc,(posX[i],posY[i]), 1, (0,0,255), -1)
#   print("PosX " + repr(posX[i]) + " PosY=" + repr(posY[i]))
cv.imshow('Video', imsrc)
print("The end")
cv.waitKey(0)

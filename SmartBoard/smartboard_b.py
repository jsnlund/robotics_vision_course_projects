##################################
# ----- SmartBoard Project ----- #
##################################
# version b

####################################################################
####################################################################
print("importing packages", flush = True)
import argparse
import os
import numpy as np
import cv2
import glob
from matplotlib import pyplot as plt

####################################################################
####################################################################
print("defining functions",flush = True)
def argument_parser():
	parser = argparse.ArgumentParser(description="Change color space of the \
			input video stream using keyboard controls. The control keys are: \
			Binary - 'b', Adaptive Binary - 'g', Canny - 'y', Corners - 'c'  \
			Lines - 'l', Differencing - 'd'")
	return parser

def draw(img, corners, imgpts):
	imgpts = np.int32(imgpts).reshape(-1,2)

	# draw ground floor in green
	img = cv2.drawContours(img, [imgpts[:4]],-1,(0,255,0),-3)

	# draw pillars in blue color
	for i,j in zip(range(4),range(4,8)):
		img = cv2.line(img, tuple(imgpts[i]), tuple(imgpts[j]),(255),3)

	# draw top layer in red color
	img = cv2.drawContours(img, [imgpts[4:]],-1,(0,0,255),3)

	return img

####################################################################
####################################################################
print("defining parameters",flush = True)
title = "Webcam"
option = "Webcam"
font = cv2.FONT_HERSHEY_SIMPLEX
text_col = (255,0,0)
width_chess = 9
height_chess = 7

scale_img = 1 # used in the resize function for displaying the video
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
objp = np.zeros((width_chess*height_chess,3), np.float32)
objp[:,:2] = np.mgrid[0:width_chess,0:height_chess].T.reshape(-1,2)
# axis = np.float32([[3,0,0], [0,3,0], [0,0,-3]]).reshape(-1,3)
axis = np.float32([[0,0,0], [0,1,0], [1,1,0], [1,0,0], [0,0,-5],[0,1,-5],[1,1,-5],[1,0,-5] ])

####################################################################
####################################################################
print("importing camera parameters", flush = True)
with np.load('WebcamParams.npz') as X:
	mtx, dist = [X[i] for i in ('mtx','dist')]#,'rvecs','tvecs')]

####################################################################
####################################################################
print("capturing webcam",flush = True)
cap = cv2.VideoCapture(0)
ret, frame = cap.read()
frame = cv2.resize(frame, None, fx=scale_img, fy=scale_img, interpolation=cv2.INTER_AREA)
rows, cols, rgb = frame.shape


####################################################################
####################################################################
print("looping...",flush = True)
if __name__=='__main__':
	args = argument_parser().parse_args()
	cap = cv2.VideoCapture(0)

	# ----- Check if the webcam is opened correctly ----- #
	if not cap.isOpened():
		raise IOError("Cannot open webcam")
	else: 
		print("Webcam Initialized")

	# ----- initialize current and previous charcter variables ----- #
	cur_char = -1
	prev_char = -1

	while True:
		# ----- Read the current frame from webcam ----- #
		ret, frame = cap.read()

		# ----- Resize the captured image ----- #
		frame = cv2.resize(frame, None, fx=scale_img, fy=scale_img, interpolation=cv2.INTER_AREA)
		rows, cols, rgb = frame.shape
		# wait 1 ms for keypress
		c = cv2.waitKey(1)

		# ----- Press ESC to exit program ----- #
		if c == 27:
			break

		# ----- reads in keypress characters ----- #
		if c > -1 and c != prev_char:
			cur_char = c
			first = True
		else:
			first = False
		prev_char = c

		if cur_char == ord('a'):
			option = "Augmented"
			text_col = (0,255,0)
			gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

			ret, corners = cv2.findChessboardCorners(gray, (width_chess,height_chess),None)
			# print("Corners: ", corners.shape,flush = True)
			if ret:
				# print("found Chessboard", flush = True)
				corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
				# print("Corners2: ", corners2.shape,flush = True)
				# Find the rotation and translation vectors.
				retval, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)
				# rvecs, tvecs, inliers = cv2.solvePnPRansac(objp, corners2, mtx, dist)

				# project 3D points to image plane
				imgpts, jac = cv2.projectPoints(axis, rvecs, tvecs, mtx, dist)
				output = draw(frame,corners2,imgpts)
			else:
				output = frame
		else:
			option = "Webcam"
			text_col = (255,0,0)
			output = frame

		cv2.putText(output,option,(10,rows-25), font, 1,text_col,2,cv2.LINE_AA)
		cv2.imshow(title, output)

	cap.release()
	cv2.destroyAllWindows()
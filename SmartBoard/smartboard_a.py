##################################
# ----- SmartBoard Project ----- #
##################################

print("importing packages", flush = True)
import os
import numpy as np
import cv2
import glob
from matplotlib import pyplot as plt

##################################
##################################
print("importing camera parameters", flush = True)
with np.load('WebcamParams.npz') as X:
	mtx, dist = [X[i] for i in ('mtx','dist')]#,'rvecs','tvecs')]

# def draw(img, corners, imgpts):
# 	corner = tuple(corners[0].ravel())
# 	img = cv2.line(img, corner, tuple(imgpts[0].ravel()), (255,0,0), 5)
# 	img = cv2.line(img, corner, tuple(imgpts[1].ravel()), (0,255,0), 5)
# 	img = cv2.line(img, corner, tuple(imgpts[2].ravel()), (0,0,255), 5)
# 	return img

##################################
##################################
print("defining functions", flush = True)
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


##################################
##################################
print("defining parameters", flush = True)
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
objp = np.zeros((10*7,3), np.float32)
objp[:,:2] = np.mgrid[0:10,0:7].T.reshape(-1,2)
# axis = np.float32([[3,0,0], [0,3,0], [0,0,-3]]).reshape(-1,3)
axis = np.float32([[0,0,0], [0,1,0], [1,1,0], [1,0,0], [0,0,-5],[0,1,-5],[1,1,-5],[1,0,-5] ])

# print(objp.shape,flush = True)

##################################
##################################
print("drawing... ", flush = True)
for fname in glob.glob('AR*.jpg'):
	# print(fname,flush = True)
	img = cv2.imread(fname)
	gray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
	ret, corners = cv2.findChessboardCorners(gray, (10,7),None)
	# print("Corners: ", corners.shape,flush = True)
	if ret == True:
		# print("found Chessboard", flush = True)
		corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
		# print("Corners2: ", corners2.shape,flush = True)
		# Find the rotation and translation vectors.
		retval, rvecs, tvecs = cv2.solvePnP(objp, corners2, mtx, dist)
		# rvecs, tvecs, inliers = cv2.solvePnPRansac(objp, corners2, mtx, dist)
		

		# project 3D points to image plane
		imgpts, jac = cv2.projectPoints(axis, rvecs, tvecs, mtx, dist)

		img = draw(img,corners2,imgpts)
		cv2.imshow('img',img)
		k = cv2.waitKey(0) & 0xff
		if k == 's':
			cv2.imwrite(fname[:6]+'.png', img)

cv2.destroyAllWindows()
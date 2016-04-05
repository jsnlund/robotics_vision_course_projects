###########################################
# ----- TASK 6 Assignment 2 - EE631 ----- #
###########################################
def jprint(phrase):
	print(phrase , flush = True)
def jplot(image):
	plt.subplot(111),plt.imshow(image, cmap = 'gray')
	plt.title('Chessboard Found =)'), plt.xticks([]), plt.yticks([])
	plt.show()
jprint("importing packages...")
import cv2
import numpy as np
from matplotlib import pyplot as plt

###########################################
jprint("defining parameters")

camParams = np.load('WebcamParams.npz')
mtx = camParams['mtx']
dist = camParams['dist']
# jprint(mtx)
# jprint(dist)

filePath = '/Users/jason/Documents/EE631/Assignment Work/A2/Webcam Images JPG/'
fileName = 'WC'
fType = '%01d.jpg'
# np.set_printoptions(precision=5)
numFiles = 32
files = []
for i in range(1,numFiles+1):
	files.append(filePath + fileName + fType %i)

images = []
for image in files:
	saveName = image[:-4] + "_distDiff" + image[-4:]
	img = cv2.imread(image) # load image
	img_undist = cv2.undistort(img,mtx,dist,None)
	dist_diff = cv2.absdiff(img,img_undist)
	images.append(dist_diff)
	cv2.imwrite(saveName, dist_diff) 

plt.subplot(221),plt.imshow(images[0], cmap = 'gray')
plt.title('Chessboard Found =)'), plt.xticks([]), plt.yticks([])
plt.subplot(222),plt.imshow(images[1], cmap = 'gray')
plt.title('Chessboard Found =)'), plt.xticks([]), plt.yticks([])
plt.subplot(212),plt.imshow(images[3], cmap = 'gray')
plt.title('Chessboard Found =)'), plt.xticks([]), plt.yticks([])
plt.show()

###########################################
# ----- TASK 5 Assignment 2 - EE631 ----- #
###########################################
def jprint(phrase):
	print(phrase , flush = True)
def jplot(image):
	plt.subplot(111),plt.imshow(image, cmap = 'gray')
	plt.title('Chessboard Found =)'), plt.xticks([]), plt.yticks([])
	plt.show()
# def argument_parser():
#     parser = argparse.ArgumentParser(description="Change color space of the \
#             input video stream using keyboard controls. The control keys are: \
#             Binary - 'b', Adaptive Binary - 'g', Canny - 'y', Corners - 'c'  \
#             Lines - 'l', Differencing - 'd'")
#     return parser

jprint("importing packages...")
import cv2
import numpy as np
from matplotlib import pyplot as plt

###########################################
jprint("defining parameters")
filePath = '/Users/jason/Documents/EE631/Assignment Work/A2/Webcam Images JPG/'
fileName = 'WC'
fType = '%01d.jpg'
# np.set_printoptions(precision=5)
numFiles = 32
files = []
for i in range(1,numFiles+1):
	files.append(filePath + fileName + fType %i)

scale_img = 1 # used in the resize function for displaying the video
title = "Webcam"
option = "Webcam"
font = cv2.FONT_HERSHEY_SIMPLEX
text_col = (255,0,0)
num = 0
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 0.1)
patternSize = (9,7)   # CRITICAL terms
objp = np.zeros((9*7,3), np.float32)  # CRITICAL terms
objp[:,:2] = np.mgrid[0:9,0:7].T.reshape(-1,2)  # CRITICAL terms
objp = objp*1.0

objPoints = [] # 3d point in real world space
imgPoints = [] # 2d points in image plane.
percent = 0.0

###########################################
if __name__=='__main__':
	# args = argument_parser().parse_args()
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

		if cur_char == ord(' '):
			# frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
			# output = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
			
			# if first:
			cv2.waitKey(2000)
			output = frame
			cv2.imwrite(files[num], output)
			print("image {} of {} taken".format(num, numFiles),flush = True)
			num += 1
			c = 27
			# else:
			# 	output = frame
		else:
			option = ""
			text_col = (255,0,0)
			output = frame

		if num >= numFiles:
			break
        # cv2.destroyAllWindows()
		cv2.putText(output,option,(10,rows-25), font, 1,text_col,2,cv2.LINE_AA)
		cv2.imshow(title, output)
cap.release()
cv2.destroyAllWindows()

###########################################
print("Loading Images", flush = True)
for image in files:
	img = cv2.imread(image) # load image
	gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY) # convert to grayscale
	ret,corners = cv2.findChessboardCorners(gray, patternSize, None) # find chessboard corners
	# if chessboard corners found - refine corners and store
	if ret:
		print("Corners Found", flush = True)
		objPoints.append(objp)
		corners = cv2.cornerSubPix(gray, corners, (5,5), (-1,-1), criteria)
		imgPoints.append(corners)
	else:
		jprint("No Corners Found") 
	percent += 100.0/numFiles
	print("{0:.0f}% images analyzed".format(percent),flush = True) # display percent complete

jprint(gray.shape[::-1]) 
if imgPoints != []:
	jprint("calibrating camera...")
	retval, cameraMatrix, distCoeffs, rvecs, tvecs = cv2.calibrateCamera(objPoints, imgPoints, gray.shape[::-1],None,cv2.CALIB_USE_INTRINSIC_GUESS)
	np.savez('WebcamParams.npz', mtx = cameraMatrix, dist = distCoeffs)
	print(cameraMatrix)
	print(distCoeffs)

else:
	jprint("Oops, you didn't give me a chessboard, Try again.")





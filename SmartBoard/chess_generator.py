import numpy as np
import argparse
import cv2

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-v", "--video",
	help="path to the (optional) video file")
ap.add_argument("-b", "--buffer", type=int, default=64,
	help="max buffer size")
args = vars(ap.parse_args())

def argument_parser():
	parser = argparse.ArgumentParser(description="Change color space of the \
			input video stream using keyboard controls. The control keys are: \
			Binary - 'b', Adaptive Binary - 'g', Canny - 'y', Corners - 'c'  \
			Lines - 'l', Differencing - 'd'")
	return parser

# if a video path was not supplied, grab the reference
# to the webcam
if not args.get("video", False):
	camera = cv2.VideoCapture(0)

# otherwise, grab a reference to the video file
else:
	camera = cv2.VideoCapture(args["video"])

(grabbed, frame) = camera.read()

chess = np.copy(frame[:,:,0])
chess[:,:] = 0
height, width = chess.shape

sq = 40
sq2 = 39
sx = int(width / sq)
sy = int(height / sq)

# print(sx, sy,flush = True)

for i in range(sy):
	if i % 2 == 0:
		s = 0
	else:
		s = 1
	for j in range(s,sx):
		if (j-s) % 2 == 0:
			x = int(j*sq)
			y = int(i*sq)
			cv2.rectangle(chess,(x,y),(x+sq2,y+sq2),(255,255,255),-1)
			cv2.imshow("Frame", chess)
			cv2.waitKey(2)

saveName = 'chessboard_generated.jpg'
cv2.imwrite(saveName, chess)



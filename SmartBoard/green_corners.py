##################################
# ----- Ball Tracking Demo ----- #
##################################
from collections import deque
import numpy as np
import argparse
import imutils
import cv2

def inBounds(center, pcenter, delta, width, height):
	x = center[0]
	y = center[1]
	if x < delta:
		center = (delta,y)
	elif x > width - delta:
		center = (width - delta,y)

	x = center[0]
	y = center[1]

	if y < delta:
		center = (x,delta)
	elif y > height - delta:
		center = (x,height - delta)
	 # and x < width - delta and y > delta and y < height - delta:
	return center
	# else:
	# 	return pcenter

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

# define the lower and upper boundaries of the "green"
# ball in the HSV color space, then initialize the
# list of tracked points
greenLower = (35, 60, 50) #(40,int(0.3*255),int(0.1*255))# (80,60,140)#
greenUpper = (64, 255, 255) #(70,int(1*255),int(1*255))# (130,100,200)#
pts = deque(maxlen=args["buffer"])
blurme = True
center = (320,240)
lineThick = 2
lineMin = 2
eraseThick = 30
eraseMin = 5
red = (0,0,255)
green = (0,255,0)
blue = (255,0,0)
orange = (0,80,200)
markerColors = [red, green, blue,orange]
lineColor = 0
dwnarrow = 63233
uparrow = 63232
larrow = 63234
rarrow = 63235
liveVideo = True
cornerWidth = 75
width = 640
height = 480

# if a video path was not supplied, grab the reference
# to the webcam
if not args.get("video", False):
	camera = cv2.VideoCapture(0)

# otherwise, grab a reference to the video file
else:
	camera = cv2.VideoCapture(args["video"])

(grabbed, frame) = camera.read()
ink = np.copy(frame)
ink[:,:,:] = 0
cali = np.copy(ink)
cv2.rectangle(cali,(0,0),(cornerWidth,cornerWidth),green,-1)
cv2.rectangle(cali,(width,0),(width - cornerWidth,cornerWidth),green,-1)
cv2.rectangle(cali,(0,height),(cornerWidth,height - cornerWidth),green,-1)
cv2.rectangle(cali,(width,height),(width - cornerWidth,height - cornerWidth),green,-1)
# width = camera.get(cv2.CAP_PROP_FRAME_WIDTH)
# height = camera.get(cv2.CAP_PROP_FRAME_HEIGHT)
# myType = camera.get(cv2.CAP_PROP_FORMAT)
# ink = np.zeros((height,width,3))

# print(width, height,flush = True)
# keep looping

# cv2.namedWindow("Frame", cv2.WND_PROP_FULLSCREEN)
# cv2.setWindowProperty("Frame", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_NORMAL)


# ----- initialize current and previous charcter variables ----- #
cur_char = -1
prev_char = -1
# center = 

while True:
	# grab the current frame
	(grabbed, frame) = camera.read()
	# if first:
	# 	ink = np.copy(frame)
	# 	ink[:,:,:] = 0
	# 	first = False
	# else:
	# 	continue
	# if we are viewing a video and we did not grab a frame,
	# then we have reached the end of the video
	c = cv2.waitKey(1)
	# print(c,flush = True)
	# ----- Press ESC to exit program ----- #
	if c == 27:
		break
	elif c == uparrow:
		lineThick += 2
		c = prev_char
	elif c == dwnarrow:
		c = prev_char
		if lineThick > lineMin:
			lineThick -= 2
	elif c == rarrow:
		eraseThick += 5
		c = prev_char
	elif c == larrow:
		c = prev_char
		if eraseThick > eraseMin:
			eraseThick -= 5
	elif c == ord('c'):
		c = prev_char
		if lineColor < len(markerColors) - 1:
			lineColor += 1
		else:
			lineColor = 0
	elif c == ord('v'):
		c = prev_char
		liveVideo = not liveVideo

	# ----- reads in keypress characters ----- #
	if c > -1 and c != prev_char:
		cur_char = c
		first = True
	else:
		first = False
	prev_char = c

	if args.get("video") and not grabbed:
		break
 
	# resize the frame, blur it, and convert it to the HSV
	# color space
	# frame = imutils.resize(frame, width=600)
	if blurme:
		blurred = cv2.GaussianBlur(frame, (11, 11), 0)
	else:
		blurred = frame
	hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)
 
	# construct a mask for the color "green", then perform
	# a series of dilations and erosions to remove any small
	# blobs left in the mask
	mask = cv2.inRange(hsv, greenLower, greenUpper)
	mask = cv2.erode(mask, None, iterations=2)
	mask = cv2.dilate(mask, None, iterations=2)
	# find contours in the mask and initialize the current
	# (x, y) center of the ball
	cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)[-2]

	tracker = np.copy(frame)
	tracker[:,:,:] = 0
	# only proceed if at least one contour was found
	if len(cnts) > 0:
		# find the largest contour in the mask, then use
		# it to compute the minimum enclosing circle and
		# centroid
		c = max(cnts, key=cv2.contourArea)
		((x, y), radius) = cv2.minEnclosingCircle(c)
		M = cv2.moments(c)
		center = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))
 
		# only proceed if the radius meets a minimum size
		# if radius > 10 and radius < 50:
			# draw the circle and centroid on the frame,
			# then update the list of tracked points

		
		if cur_char == ord('d'):
			center = inBounds(center, pcenter, cornerWidth, width, height)
			cv2.circle(tracker, center, 5, blue, -1)
			cv2.circle(tracker, center, int(radius), (200, 200, 0), 2)
			if first:
				pcenter = center
			cv2.line(ink, pcenter, center,markerColors[lineColor], lineThick)

		elif cur_char == ord('e'):
			cv2.circle(tracker, center, eraseThick,red, 2)
			cv2.circle(ink, center, eraseThick, (0, 0, 0), -1)
			cv2.line(ink, pcenter, center,(0, 0, 0), eraseThick)

		pcenter = center
		# elif cur_char == ord('w'):
	# else:
	# 	cv2.circle(tracker, center, 5, blue, -1)
			
			# cv2.rectangle(ink,(width,0),(width - cornerWidth,cornerWidth))

	
	# center = None
			# update the points queue
			# pts.appendleft(center)
			# loop over the set of tracked points
	# for i in range(1, len(pts)):
	# 	# if either of the tracked points are None, ignore
	# 	# them
	# 	if pts[i - 1] is None or pts[i] is None:
	# 		continue
 
	# 	# otherwise, compute the thickness of the line and
	# 	# draw the connecting lines
	# 	thickness = 2 #int(np.sqrt(args["buffer"] / float(i + 1)) * 2.5)
	# 	cv2.line(ink, pts[i - 1], pts[i], (0, 0, 255), thickness)
 
	# show the frame to our screen
	# print(np.mean(frame[:,:,0]), np.mean(frame[:,:,1]), np.mean(frame[:,:,2]), flush = True)
	mask = cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR)
	if cur_char == ord('w'):
		feed = cali
	else:
		feed = cv2.add(tracker,ink)
	if liveVideo:
		# gray = cv2.cvtColor(feed, cv2.COLOR_BGR2GRAY)
		# mask = np.copy(frame)
		# mask[:,:,:] = 0
		# mask[gray == 0] = frame[gray == 0]
		# # feed = mask
		feed = cv2.add(frame,feed)
	cv2.imshow("Frame", cv2.flip(feed,1))
	# key = cv2.waitKey(1) & 0xFF
 
	# # if the 'q' key is pressed, stop the loop
	# if key == ord("q"):
	# 	break
 
# cleanup the camera and close any open windows
camera.release()
cv2.destroyAllWindows()
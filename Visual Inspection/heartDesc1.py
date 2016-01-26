# ----- Heart Shape Description ----- #

# ----- Define Functions ----- #

def jplot(image):
	plt.subplot(111),plt.imshow(image, cmap = 'gray')
	plt.title('Hearted'), plt.xticks([]), plt.yticks([])
	plt.show()


def jprint(phrase):
	print(phrase , flush = True)

def mask(image,xp):
	rows, cols = image.shape
	image[:,0:int(xp*float(cols))] = 0
	image[:,int((1-xp)*float(cols)):cols] = 0
	return image


def getBlobs(image,color):
	params = cv2.SimpleBlobDetector_Params()
	# Change thresholds
	params.thresholdStep = 5
	params.minThreshold = 10;
	params.maxThreshold = 255;
	# Filter by Area.
	params.filterByArea = True
	params.minArea = 50
	# Filter by Circularity
	params.filterByCircularity = True
	params.minCircularity = 0.01
	# params.maxCircularity = 1
	# Filter by Convexity
	params.filterByConvexity = True
	params.minConvexity = 0.01
	# params.maxConvexity = 1
	# Filter by Inertia
	params.filterByInertia = True
	params.minInertiaRatio = 0.01
	detector = cv2.SimpleBlobDetector_create(params)

	blob = detector.detect(image)
	jprint(blob[0].size)
	# jprint(blob[0].angle)
	# jprint(blob[0].response)
	jprint(blob[0].inertia)
	jprint(blob[0].pt)
	image = cv2.drawKeypoints(image, blob, np.array([]), color, cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS)
	jplot(image)

def getLines(image,rho_acc,theta_acc,line_thresh,color):
	edges = cv2.Canny(image, 200,400)
	lines = cv2.HoughLines(edges,rho_acc,theta_acc,line_thresh)
	angles = lines[:,0,1]
	angle = 90 - (abs(angles[1]-angles[0]) % (pi/2))*180/pi
	print('Angle: ', angle, flush = True)
	image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)
	if lines is not None:
	    # print("Detected",lines.shape, lines[:,0,:], flush = True)
	    for rho, theta in lines[:,0,:]:
	        # print theta.shape
	        # theta = pi - (theta % pi)
	        # print theta.shape
	        a = cos(theta)
	        b = sin(theta)
	        x0 = a*rho
	        y0 = b*rho
	        x1 = int(x0 + 1000*(-b))
	        y1 = int(y0 + 1000*(a))

	        x2 = int(x0 - 1000*(-b))
	        y2 = int(y0 - 1000*(a))

	        cv2.line(image,(x1,y1),(x2,y2),color,2)
	jplot(image)


def getCorners(image,maxCorners,qualityLevel,minDist,blurFactor,color):
	img = cv2.GaussianBlur(image,(blurFactor,blurFactor),0)

	corners = cv2.goodFeaturesToTrack(img, maxCorners, qualityLevel, minDist)
	criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 0.1)
	corners = cv2.cornerSubPix(img, corners, (11,11), (-1,-1), criteria)
	corners = np.float32(corners)

	image = cv2.cvtColor(image, cv2.COLOR_GRAY2BGR)

	distance = np.sqrt(np.sum((corners[0,:] - corners[1,:])**2.0))
	print('Corners: \n', corners, flush = True)
	print('Corner Distance: ', distance, flush = True)

	for item in corners:
	    x, y = item[0]
	    cv2.circle(image, (x,y), 5, color, -1)
	jplot(image)

	
jprint('importing packages...')
from numpy import sin, cos, pi
import numpy as np
import cv2
from matplotlib import pyplot as plt

# ----- Parameters ----- #
jprint('defining parameters...')
# - General - #
color = (0,255,0) # plot color
xp = 0.21

# - Lines - #
line_thresh = 20
rho_acc = 1
theta_acc = 10.0*pi/180

# - Corners - #
maxCorners = 2
qualityLevel = 0.01
minDist = 10
blurFactor = 11

jprint('importing image...')
fileName = 'frame3.bmp'
filePath = '/Users/jason/Documents/EE631/VisualInspection-10/'
fName = filePath + fileName

img = cv2.imread(fName,0)

mask(img,xp)

# getCorners(img,maxCorners,qualityLevel,minDist,blurFactor,color)

# getLines(img,rho_acc,theta_acc,line_thresh,color)

# getBlobs(img,color)
img = cv2.GaussianBlur(img,(blurFactor,blurFactor),0)

jprint('canny image...')
edges = cv2.Canny(img, 10, 250)
kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (1, 1))
closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)
jprint('finding contours...')
_,cnts, _ = cv2.findContours(closed.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
total = 0
# loop over the contours
img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)

# jplot(img)

cnt = cnts[0]

hull = cv2.convexHull(cnt,returnPoints = False)
defects = cv2.convexityDefects(cnt,hull)

for c in cnts:
	# approximate the contour
	k = cv2.isContourConvex(cnt)
	# jprint(k)
	if not k:
		jprint('contour is concave...')
		peri = cv2.arcLength(c, True)
		approx = cv2.approxPolyDP(c, 0.001 * peri, True)
		# jprint(approx)

		# if the approximated contour has four points, then assume that the
		# contour is a book -- a book is a rectangle and thus has four vertices
		# if len(approx) == 4:
		cv2.drawContours(img, [approx], -1, (0, 255, 0), 2)

		# rect = cv2.minAreaRect(cnt)
		# jprint(rect[1])
		# box = cv2.boxPoints(rect)
		# box = np.int0(box)
		# # jprint(box)
		# boxArea = rect[1][0]*rect[1][1]
		# boxCenter = rect[0]
		# jprint(boxArea)
		# cv2.drawContours(img,[box],0,(0,0,255),2)
		# rows,cols = img.shape[:2]

		(x,y),radius = cv2.minEnclosingCircle(c)
		center = (int(x),int(y))
		radius = int(radius)
		cv2.circle(img,center,radius,(0,255,0),2)
		circleArea = pi*radius**2
		cntArea = cv2.contourArea(c)
		print('Contour Perimeter: ', peri, flush = True)
		print('Contour Area: ', cntArea, flush = True)
		print('Circle Area: ', circleArea, flush = True)
		print('Circle Radius: ', radius, flush = True)
		print('Circle - Contour area Difference: ', abs(circleArea - cntArea), flush = True)
		print(center,flush=True)
		# encShpDist = np.sqrt(np.sum((boxCenter - np.rint(center))**2))
		# jprint(encShpDist)
		total += 1

# for i in range(defects.shape[0]):
#     s,e,f,d = defects[i,0]
#     start = tuple(cnt[s][0])
#     end = tuple(cnt[e][0])
#     far = tuple(cnt[f][0])
#     cv2.line(img,start,end,[0,255,0],2)
#     cv2.circle(img,far,5,[0,0,255],-1)

jplot(img)


if yc < 400:
	# perform contour analysis
	if 




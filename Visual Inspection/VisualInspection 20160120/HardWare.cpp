#include "stdafx.h"
#include "time.h"
#include "math.h"
#include "Hardware.h"

ImagingResources	CTCSys::IR;

// // TODO: Convert to Python
// // Constants/Parameters
// COLOR = (0,255,0)
// double XP = 0.21;

// // - LINES -
// LINE_THRESH = 20
// RHO_ACC = 1
// THETA_ACC = 10.0*PI/180

// // - CORNERS -
// MAXCORNERS = 2
// QUALITYLEVEL = 0.01
// MINDIST = 10
int const BLURFACTOR = 11;
int const IMAGE_WIDTH = 640;
int const IMAGE_HEIGHT = 480;
int const ROI_X = 134;
int const ROI_Y = 0;
int const ROI_WIDTH = 371;
int const ROI_HEIGHT = IMAGE_HEIGHT;

bool displayMsg = true;

Mat frameDisplay; // the frame to work with

CTCSys::CTCSys()
{
	EventEndProcess = TRUE;
	IR.Acquisition = TRUE;
	IR.UpdateImage = TRUE;
	IR.Inspection = FALSE;
	OPENF("c:\\Projects\\RunTest.txt");
}

CTCSys::~CTCSys()
{
	CLOSEF;
}

void CTCSys::QSStartThread()
{
	EventEndProcess = FALSE;
	//QSProcessEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	// Image Processing Thread
	QSProcessThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSProcessThreadFunc,
		this, NULL, (LPDWORD) &QSProcessThreadHandleID);
	ASSERT(QSProcessThreadHandle != NULL);
	SetThreadPriority(QSProcessThreadHandle, THREAD_PRIORITY_HIGHEST);
}

void CTCSys::QSStopThread()
{
	// need to make sure camera acquisition has stopped
	EventEndProcess = TRUE;
	do {
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSProcessThreadHandle);
}

long QSProcessThreadFunc(CTCSys *QS)
{
	int		i;
	bool	pass = false;
	Mat CurrentImage;

	while (QS->EventEndProcess == FALSE) {

#ifdef PTGREY
		if (QS->IR.Acquisition == TRUE) {
			for(i=0; i < QS->IR.NumCameras; i++) {
				if (QS->IR.pgrCamera[i]->RetrieveBuffer(&QS->IR.PtGBuf[i]) == PGRERROR_OK) {
					QS->QSSysConvertToOpenCV(&QS->IR.AcqBuf[i], QS->IR.PtGBuf[i]);
				}
			}
			for(i=0; i < QS->IR.NumCameras; i++) {
#ifdef PTG_COLOR
				mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.ProcBuf[i], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
				QS->IR.AcqBuf[i].copyTo(QS->IR.ProcBuf[i][BufID]);	// Has to copy out of acquisition buffer before processing
#endif
			}
		}
#else
		Sleep (200);
#endif
		// TODO: Edit code here!!!
		// Process Image ProcBuf
		if (QS->IR.Inspection) {
			// Images are acquired into ProcBuf{0]
			// QS->IR.ProcBuf[0].copyTo(frameDisplay);
		    // Mat mask = Mat::zeros(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);
		    // mask(Rect(ROI_X,ROI_Y,ROI_WIDTH,ROI_HEIGHT)) = 1;

			//    GaussianBlur(QS->IR.OutBuf1[0], QS->IR.OutBuf1[0], Size(BLURFACTOR,BLURFACTOR), 1.5, 1.5);
			// Canny(QS->IR.OutBuf1[0], QS->IR.OutBuf1[0], 70, 100);
			cvtColor(QS->IR.ProcBuf[0], QS->IR.OutBuf1[0], CV_RGB2GRAY);
			CurrentImage = QS->IR.OutBuf1[0](Rect(ROI_X,ROI_Y,ROI_WIDTH,ROI_HEIGHT));
			CurrentImage.setTo(255);

			// QS->IR.ProcBuf[0] = CurrentImage;
		    QS->IR.OutBuf1[0] = CurrentImage;

			pass = true;

			if(displayMsg){
				displayMsg = false;
				AfxMessageBox(L"Yo!!", MB_ICONSTOP);
			}


			// TODO: Convert over from Python to C++
			// edges = cv2.Canny(img, 10, 250)
			// kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (1, 1))
			// closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)
			// jprint('finding contours...')
			// TODO: Get this to work!
			// _,cnts, _ = cv2.findContours(closed.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)






			// total = 0
			// # loop over the contours
			// img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
			// cnt = cnts[0]

			// hull = cv2.convexHull(cnt,returnPoints = False)
			// defects = cv2.convexityDefects(cnt,hull)

			// for c in cnts:
			// 	# approximate the contour
			// 	k = cv2.isContourConvex(cnt)
			// 	# jprint(k)
			// 	if not k:
			// 		jprint('contour is concave...')
			// 		peri = cv2.arcLength(c, True)
			// 		approx = cv2.approxPolyDP(c, 0.001 * peri, True)
			// 		# jprint(approx)

			// 		# if the approximated contour has four points, then assume that the
			// 		# contour is a book -- a book is a rectangle and thus has four vertices
			// 		# if len(approx) == 4:
			// 		cv2.drawContours(img, [approx], -1, (0, 255, 0), 2)

			// 		# rect = cv2.minAreaRect(cnt)
			// 		# jprint(rect[1])
			// 		# box = cv2.boxPoints(rect)
			// 		# box = np.int0(box)
			// 		# # jprint(box)
			// 		# boxArea = rect[1][0]*rect[1][1]
			// 		# boxCenter = rect[0]
			// 		# jprint(boxArea)
			// 		# cv2.drawContours(img,[box],0,(0,0,255),2)
			// 		# rows,cols = img.shape[:2]

			// 		(x,y),radius = cv2.minEnclosingCircle(c)
			// 		center = (int(x),int(y))
			// 		radius = int(radius)
			// 		cv2.circle(img,center,radius,(0,255,0),2)
			// 		circleArea = pi*radius**2
			// 		cntArea = cv2.contourArea(c)
			// 		print('Contour Perimeter: ', peri, flush = True)
			// 		print('Contour Area: ', cntArea, flush = True)
			// 		print('Circle Area: ', circleArea, flush = True)
			// 		print('Circle Radius: ', radius, flush = True)
			// 		print('Circle - Contour area Difference: ', abs(circleArea - cntArea), flush = True)
			// 		print(center,flush=True)
			// 		# encShpDist = np.sqrt(np.sum((boxCenter - np.rint(center))**2))
			// 		# jprint(encShpDist)
			// 		total += 1



			// // Example processing
			// for(i=0; i < QS->IR.NumCameras; i++) {
			// 	// Example using Canny.  Input is ProcBuf.  Output is OutBuf1
			// 	cvtColor(QS->IR.ProcBuf[i], QS->IR.OutBuf1[i], CV_RGB2GRAY);
			// 	Canny(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], 70, 100);
			// 	// QS->IR.OutBuf1[i] = processHeartImage(QS->IR.ProcBuf[i]);
			// }

		}
		// Display Image
		if (QS->IR.UpdateImage) {
			for (i=0; i<QS->IR.NumCameras; i++) {
				if (!QS->IR.Inspection) {
					// Example of displaying color buffer ProcBuf
					QS->IR.ProcBuf[i].copyTo(QS->IR.DispBuf[i]);
				} else {
					// Example of displaying B/W buffer OutBuf1
					QS->IR.OutBuf[0] = QS->IR.OutBuf[1] = QS->IR.OutBuf[2] = QS->IR.OutROI1[i];
					merge(QS->IR.OutBuf, 3, QS->IR.DispROI[i]);
					// Example to show inspection result, print result after the image is copied
					QS->QSSysPrintResult(pass);
				}
			}
			QS->QSSysDisplayImage();
		}
	}
	QS->EventEndProcess = FALSE;
	return 0;
}

void CTCSys::QSSysInit()
{
	long i;
	IR.DigSizeX = 640;
	IR.DigSizeY = 480;
	initBitmapStruct(IR.DigSizeX, IR.DigSizeY);
	// Camera Initialization
#ifdef PTGREY
	IR.cameraConfig.asyncBusSpeed = BUSSPEED_S800;
	IR.cameraConfig.isochBusSpeed = BUSSPEED_S800;
	IR.cameraConfig.grabMode = DROP_FRAMES;			// take the last one, block grabbing, same as flycaptureLockLatest
	IR.cameraConfig.grabTimeout = TIMEOUT_INFINITE;	// wait indefinitely
	IR.cameraConfig.numBuffers = 4;					// really does not matter since DROP_FRAMES is set not to accumulate buffers

	// How many cameras are on the bus?
	if(IR.busMgr.GetNumOfCameras((unsigned int *)&IR.NumCameras) != PGRERROR_OK){	// something didn't work correctly - print error message
		AfxMessageBox(L"Connect Failure", MB_ICONSTOP);
	} else {
		IR.NumCameras = (IR.NumCameras > MAX_CAMERA) ? MAX_CAMERA : IR.NumCameras;
		for(i = 0; i < IR.NumCameras; i++) {
			// Get PGRGuid
			if (IR.busMgr.GetCameraFromIndex(0, &IR.prgGuid[i]) != PGRERROR_OK) {
				AfxMessageBox(L"PGRGuID Failure", MB_ICONSTOP);
			}
			IR.pgrCamera[i] = new Camera;
			if (IR.pgrCamera[i]->Connect(&IR.prgGuid[i]) != PGRERROR_OK) {
				AfxMessageBox(L"PConnect Failure", MB_ICONSTOP);
			}
			// Set all camera configuration parameters
			if (IR.pgrCamera[i]->SetConfiguration(&IR.cameraConfig) != PGRERROR_OK) {
				AfxMessageBox(L"Set Configuration Failure", MB_ICONSTOP);
			}
			// Set video mode and frame rate
			if (IR.pgrCamera[i]->SetVideoModeAndFrameRate(VIDEO_FORMAT, CAMERA_FPS) != PGRERROR_OK) {
				AfxMessageBox(L"Video Format Failure", MB_ICONSTOP);
			}
			// Sets the onePush option off, Turns the control on/off on, disables auto control.  These are applied to all properties.
			IR.cameraProperty.onePush = false;
			IR.cameraProperty.autoManualMode = false;
			IR.cameraProperty.absControl = true;
			IR.cameraProperty.onOff = true;
			// Set shutter sppeed
			IR.cameraProperty.type = SHUTTER;
			IR.cameraProperty.absValue = SHUTTER_SPEED;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
				AfxMessageBox(L"Shutter Failure", MB_ICONSTOP);
			}
#ifdef  PTG_COLOR
			// Set white balance (R and B values)
			IR.cameraProperty = WHITE_BALANCE;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = WHITE_BALANCE_R;
			IR.cameraProperty.valueB = WHITE_BALANCE_B;
//			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
//				AfxMessageBox(L"White Balance Failure", MB_ICONSTOP);
//			}
#endif
			// Set gain values (350 here gives 12.32dB, varies linearly)
			IR.cameraProperty = GAIN;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = GAIN_VALUE_A;
			IR.cameraProperty.valueB = GAIN_VALUE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
				AfxMessageBox(L"Gain Failure", MB_ICONSTOP);
			}
			// Set trigger state
			IR.cameraTrigger.mode = 0;
			IR.cameraTrigger.onOff = TRIGGER_ON;
			IR.cameraTrigger.polarity = 0;
			IR.cameraTrigger.source = 0;
			IR.cameraTrigger.parameter = 0;
			if(IR.pgrCamera[i]->SetTriggerMode(&IR.cameraTrigger, false) != PGRERROR_OK){
				AfxMessageBox(L"Trigger Failure", MB_ICONSTOP);
			}
			// Start Capture Individually
			if (IR.pgrCamera[i]->StartCapture() != PGRERROR_OK) {
				char Msg[128];
				sprintf_s(Msg, "Start Capture Camera %d Failure", i);
				AfxMessageBox(CA2W(Msg), MB_ICONSTOP);
			}
		}
		// Start Sync Capture (only need to do it with one camera)
//		if (IR.pgrCamera[0]->StartSyncCapture(IR.NumCameras, (const Camera**)IR.pgrCamera, NULL, NULL) != PGRERROR_OK) {
//			AfxMessageBox(L"Start Sync Capture Failure", MB_ICONSTOP);
//		}
	}
#else
	IR.NumCameras = MAX_CAMERA;
#endif
	Rect R = Rect(0, 0, IR.DigSizeX, IR.DigSizeY);
	// create openCV image
	for(i=0; i<IR.NumCameras; i++) {
#ifdef PTG_COLOR
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
#else
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i][0].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i][1].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
#endif
		IR.AcqPtr[i] = IR.AcqBuf[i].data;
		IR.DispROI[i] = IR.DispBuf[i](R);

		IR.OutBuf1[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI1[i] = IR.OutBuf1[i](R);
		IR.OutBuf2[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI2[i] = IR.OutBuf2[i](R);
		IR.DispBuf[i] = Scalar(0);
		IR.ProcBuf[i] = Scalar(0);
	}
	IR.from_to[0] = 0;
	IR.from_to[1] = 2;
	IR.from_to[2] = 1;
	IR.from_to[3] = 1;
	IR.from_to[4] = 2;
	IR.from_to[5] = 0;
	QSStartThread();
}

void CTCSys::QSSysFree()
{
	QSStopThread(); // Move to below PTGREY if on Windows Vista
#ifdef PTGREY
	for(int i=0; i<IR.NumCameras; i++) {
		if (IR.pgrCamera[i]) {
			IR.pgrCamera[i]->StopCapture();
			IR.pgrCamera[i]->Disconnect();
			delete IR.pgrCamera[i];
		}
	}
#endif
}

void CTCSys::initBitmapStruct(long iCols, long iRows)
{
	m_bitmapInfo.bmiHeader.biSize			= sizeof( BITMAPINFOHEADER );
	m_bitmapInfo.bmiHeader.biPlanes			= 1;
	m_bitmapInfo.bmiHeader.biCompression	= BI_RGB;
	m_bitmapInfo.bmiHeader.biXPelsPerMeter	= 120;
	m_bitmapInfo.bmiHeader.biYPelsPerMeter	= 120;
    m_bitmapInfo.bmiHeader.biClrUsed		= 0;
    m_bitmapInfo.bmiHeader.biClrImportant	= 0;
    m_bitmapInfo.bmiHeader.biWidth			= iCols;
    m_bitmapInfo.bmiHeader.biHeight			= -iRows;
    m_bitmapInfo.bmiHeader.biBitCount		= 24;
	m_bitmapInfo.bmiHeader.biSizeImage =
      m_bitmapInfo.bmiHeader.biWidth * m_bitmapInfo.bmiHeader.biHeight * (m_bitmapInfo.bmiHeader.biBitCount / 8 );
}

void CTCSys::QSSysDisplayImage()
{
	SetDIBitsToDevice(ImageDC[0]->GetSafeHdc(), 1, 1,
		m_bitmapInfo.bmiHeader.biWidth,
		::abs(m_bitmapInfo.bmiHeader.biHeight),
		0, 0, 0,
		::abs(m_bitmapInfo.bmiHeader.biHeight),
		IR.DispBuf[0].data,
		&m_bitmapInfo, DIB_RGB_COLORS);
}


#ifdef PTGREY
void CTCSys::QSSysConvertToOpenCV(Mat* openCV_image, Image PGR_image)
{
	openCV_image->data = PGR_image.GetData();	// Pointer to image data
	openCV_image->cols = PGR_image.GetCols();	// Image width in pixels
	openCV_image->rows = PGR_image.GetRows();	// Image height in pixels
	openCV_image->step = PGR_image.GetStride(); // Size of aligned image row in bytes
}
#endif

void CTCSys::QSSysPrintResult(bool pass)
{
	putText(IR.DispBuf[0], (pass) ? "Pass" : "Fail", Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
}

#include "stdafx.h"
#include "time.h"
#include "math.h"
#include "Hardware.h"

ImagingResources	CTCSys::IR;

CTCSys::CTCSys()
{
	EventEndProcess = TRUE;
	EventEndMove = TRUE;
	IR.Acquisition = TRUE;
	IR.UpdateImage = TRUE;
	IR.CaptureSequence = FALSE;
	IR.DisplaySequence = FALSE;
	IR.PlayDelay = 30;
	IR.CaptureDelay = 30;
	IR.FrameID = 0;
	IR.CatchBall = FALSE;
	OPENF("c:\\Projects\\RunTest.txt");
}

CTCSys::~CTCSys()
{
	CLOSEF;
}

void CTCSys::QSStartThread()
{
	EventEndProcess = FALSE;
	EventEndMove = FALSE;
	QSMoveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);		// Create a manual-reset and initially nosignaled event handler to control move event
	ASSERT(QSMoveEvent != NULL);

	// Image Processing Thread
	QSProcessThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSProcessThreadFunc,
		this, NULL, (LPDWORD) &QSProcessThreadHandleID);
	ASSERT(QSProcessThreadHandle != NULL);
	SetThreadPriority(QSProcessThreadHandle, THREAD_PRIORITY_HIGHEST);

	QSMoveThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSMoveThreadFunc,
		this, NULL, (LPDWORD) &QSMoveThreadHandleID);
	ASSERT(QSMoveThreadHandle != NULL);
	SetThreadPriority(QSMoveThreadHandle, THREAD_PRIORITY_HIGHEST);
}

void CTCSys::QSStopThread()
{
	// Must close the move event first
	EventEndMove = TRUE;				// Set the flag to true first
	SetEvent(QSMoveEvent);				// must set event to complete the while loop so the flag can be checked
	do {
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSMoveThreadHandle);

	// need to make sure camera acquisition has stopped
	EventEndProcess = TRUE;
	do {
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSProcessThreadHandle);
}

long QSMoveThreadFunc(CTCSys *QS)
{
	while (QS->EventEndMove == FALSE) {
		WaitForSingleObject(QS->QSMoveEvent, INFINITE);
		if (QS->EventEndMove == FALSE) QS->Move(QS->Move_X, QS->Move_Y);
		ResetEvent(QS->QSMoveEvent);
	}
	QS->EventEndMove = FALSE;
	return 0;
}


long QSProcessThreadFunc(CTCSys *QS)
{
	int     i;
	int     BufID = 0;
	char    str[32];
	long	FrameStamp;
	FrameStamp = 0;

	//
	//// Team 10 Code
	//

	int const IMAGE_WIDTH = 640;
	int const IMAGE_HEIGHT = 480;

	// allocate an image buffer objects
	//Mat frame_left; // The current frame
	//Mat frame_left_display; // The current frame with color and drawings
	Mat frame_left_prev;  // The previous frame
	Mat frame_left; // Diff between current and prev
	//Mat frame_left_thresh; // Threshold between diff
	//Mat frame_left_first; // The image before the first detected ball movement
	//Mat frame_left_first_diff; // Diff between current and first
	//Mat frame_left_first_diff_thresh;
	//Mat frame_left_and;

	//Mat frame_right; // The current frame
	//Mat frame_right_display; // The current frame with color and drawings
	Mat frame_right_prev; // The previous frame
	Mat frame_right; // Diff between current and prev
	//Mat frame_right_thresh; // Threshold between diff
	//Mat frame_right_first; // The image before the first detected ball movement
	//Mat frame_right_first_diff; // Diff between current and first
	//Mat frame_right_first_diff_thresh;
	//Mat frame_right_and;

	// ROI Initial Offset Constants
	int const ROI_DEFAULT_X_LEFT = 320;
	int const ROI_DEFAULT_Y_LEFT = 60;
	int const ROI_DEFAULT_WIDTH_LEFT = 100;
	int const ROI_DEFAULT_HEIGHT_LEFT = 100;

	int const ROI_DEFAULT_X_RIGHT = 235;
	int const ROI_DEFAULT_Y_RIGHT = 55;
	int const ROI_DEFAULT_WIDTH_RIGHT = ROI_DEFAULT_WIDTH_LEFT;
	int const ROI_DEFAULT_HEIGHT_RIGHT = ROI_DEFAULT_HEIGHT_LEFT;

	// Create Default ROI
	Rect ROI_DEFAULT_LEFT = Rect(ROI_DEFAULT_X_LEFT, ROI_DEFAULT_Y_LEFT, ROI_DEFAULT_WIDTH_LEFT, ROI_DEFAULT_HEIGHT_LEFT);
	Rect ROI_DEFAULT_RIGHT = Rect(ROI_DEFAULT_X_RIGHT, ROI_DEFAULT_Y_RIGHT, ROI_DEFAULT_WIDTH_RIGHT, ROI_DEFAULT_HEIGHT_RIGHT);

	// Dynamic ROIs
	// Fields: x, y, width, height
	Rect roi_left = ROI_DEFAULT_LEFT;
	Rect roi_right = ROI_DEFAULT_RIGHT;

	// When activity is generated in the initial ROI, trigger ball_in_flight bool
	// Or maybe when width of ball is too great - i.e. when too close
	bool ball_in_flight = false;

	// A vector to hold an array of corner coordinates
	vector<Point2f> corners_left, corners_right;

	// TODO: Erosion kernel - create one, or use default via Mat()?
	// Mat erosionKernel = getStructuringElement(int shape, Size ksize, Point anchor=Point(-1,-1))

	// For find contours
	vector<vector<Point>> contours_left, contours_right;
	vector<Point> ball_contour_left, ball_contour_right;
	vector<Vec4i> hierarchy_left, hierarchy_right;
	Moments ball_m_left, ball_m_right;
	Point2i ball_centroid_left, ball_centroid_right;

	// TODO: Load camera params from files


	while (QS->EventEndProcess == FALSE) {


#ifdef PTGREY		// Image Acquisition
		if (QS->IR.Acquisition == TRUE) {
			for(i=0; i < QS->IR.NumCameras; i++) {
				QS->IR.PGRError = QS->IR.pgrCamera[i]->RetrieveBuffer(&QS->IR.PtGBuf[i]);
				// Get frame timestamp if exact frame time is needed.  Divide FrameStamp by 32768 to get frame time stamp in mSec
				QS->IR.metaData[i] = QS->IR.PtGBuf[i].GetMetadata();
				FrameStamp = QS->IR.metaData[i].embeddedTimeStamp;
				if(QS->IR.PGRError == PGRERROR_OK){
					QS->QSSysConvertToOpenCV(&QS->IR.AcqBuf[i], QS->IR.PtGBuf[i]);		// copy image data pointer to OpenCV Mat structure
				}
			}
			for(i=0; i < QS->IR.NumCameras; i++) {
				if (QS->IR.CaptureSequence) {
#ifdef PTG_COLOR
					mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.SaveBuf[i][QS->IR.FrameID], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
					QS->IR.AcqBuf[i].copyTo(QS->IR.SaveBuf[i][QS->IR.FrameID]);
#endif
				} else {
#ifdef PTG_COLOR
					mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.ProcBuf[i][BufID], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
					QS->IR.AcqBuf[i].copyTo(QS->IR.ProcBuf[i]);	// Has to be copied out of acquisition buffer before processing
#endif
				}
			}
		}
#else
		Sleep (100);
#endif
		// Process Image ProcBuf
		if (QS->IR.CatchBall) {  	// Click on "Catch" button to toggle the CatchBall flag when done catching
			// Images are acquired into ProcBuf[0] for left and ProcBuf[1] for right camera
			// Need to create child image or small region of interest for processing to exclude background and speed up processing
			// Mat child = QS->IR.ProcBuf[i](Rect(x, y, width, height));

			QS->IR.ProcBuf[0].copyTo(frame_left);
			QS->IR.ProcBuf[1].copyTo(frame_right);



			// Let the corners trigger the ball in flight bool once
			if(ball_in_flight == false){
				// Abs diff the whole image. It should be pretty fast
				// TODO: Use a slightly bigger roi?
				absdiff(QS->IR.ProcBuf[0], frame_left_prev, frame_left);
				absdiff(QS->IR.ProcBuf[1], frame_right_prev, frame_right);

				// Blur an roi
				GaussianBlur(frame_left(roi_left), frame_left(roi_left), Size(11, 11), 15.0, 15.0);
				GaussianBlur(frame_right(roi_right), frame_right(roi_right), Size(11, 11), 15.0, 15.0);

				// Threshold roi
				threshold(frame_left(roi_left), frame_left(roi_left), 10, 256, THRESH_BINARY);
				threshold(frame_right(roi_right), frame_right(roi_right), 10, 256, THRESH_BINARY);

				// TODO: Replace corner detection method with a faster triggering method
				// I.e. average all the pixels and trigger if average goes above a certain number
				// use Scalar my_mean = mean(frame_left(roi_left))

				// Detect when ball emerges
				// TODO: Not Needed?
				// corners_left.clear();
				// corners_right.clear();
				goodFeaturesToTrack(frame_left(roi_left), corners_left, 10, 0.01, 10);
				goodFeaturesToTrack(frame_right(roi_right), corners_right, 10, 0.01, 10);


				if(corners_left.size() > 0 && corners_left.size() > 0){
					// Set the previous image as the first image (background image)
					frame_left_prev.copyTo(frame_left_first);
					frame_right_prev.copyTo(frame_right_first);
					ball_in_flight = true;
				}
			}
			else {
				// Do processing on images now that the ball is in flight
				absdiff(frame_left, frame_left_first, frame_left);
				absdiff(frame_right, frame_right_first, frame_right);

				// Blur an roi
				GaussianBlur(frame_left(roi_left), frame_left(roi_left), Size(11, 11), 15.0, 15.0);
				GaussianBlur(frame_right(roi_right), frame_right(roi_right), Size(11, 11), 15.0, 15.0);

				// Threshold roi
				threshold(frame_left(roi_left), frame_left(roi_left), 10, 256, THRESH_BINARY);
				threshold(frame_right(roi_right), frame_right(roi_right), 10, 256, THRESH_BINARY);

				// TODO: Ball tracking algorithm

				// Find Contours
				contours_left.clear();
				contours_right.clear();
				hierarchy_left.clear();
				hierarchy_right.clear();
				// Find contours modifies the input image!! So pass in a copy
				// findContours(frame_left_first_diff_thresh.clone()(roi_left), contours_left, hierarchy_left, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_left.x, roi_left.y));
				// CV_RETR_TREE CV_RETR_EXTERNAL
				// CV_CHAIN_APPROX_NONE CV_CHAIN_APPROX_SIMPLE
				// findContours(frame_left_first_diff_thresh.clone(), contours_left, hierarchy_left, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point());
				// findContours(frame_right_first_diff_thresh.clone(), contours_right, hierarchy_right, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point());
				findContours(frame_left.clone()(roi_left), contours_left, hierarchy_left, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_left.x, roi_left.y));
				findContours(frame_right.clone()(roi_right), contours_right, hierarchy_right, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_right.x, roi_right.y));
				// cout << "Left Contours: " << contours_left.size() << endl;

				// cout << "contours_left count: " << contours_left.size() << endl;
				// cout << "contours_right count: " << contours_right.size() << endl;

				// cvtColor(frame_left_first_diff_thresh, frame_left_first_diff_thresh, COLOR_GRAY2BGR);
				//         cvtColor(frame_right_first_diff_thresh, frame_right_first_diff_thresh, COLOR_GRAY2BGR);
				//
				// GET X,Y,Z location and move ROI
				if(contours_left.size() > 0 && contours_right.size() > 0){
					// cout << "contours_left[0]: " << contours_left[0] << endl;
					ball_contour_left = contours_left[0];
					ball_m_left = moments(ball_contour_left);
					ball_centroid_left = Point2i(int(ball_m_left.m10 / ball_m_left.m00), int(ball_m_left.m01 / ball_m_left.m00));
					// cout << "left centroid: " << ball_centroid_left << endl;
					// cout << "contours_right[0]: " << contours_right[0] << endl;
					ball_contour_right = contours_right[0];
					ball_m_right = moments(ball_contour_right);
					ball_centroid_right = Point2i(int(ball_m_right.m10 / ball_m_right.m00), int(ball_m_right.m01 / ball_m_right.m00));
					// cout << "right centroid: " << ball_centroid_right << endl;
					// cout << "drawContours!" << endl;
					drawContours(frame_left, contours_left, -1, Scalar(0,0,255), 3, 8, hierarchy_left, 2, Point() );
					// Draw the centroid of the ball
					circle(frame_left, ball_centroid_left, 5, Scalar(200,80,0), 1);
					// TODO: recalculate the roi
					int x_margin_left = roi_left.width/2;
					int y_margin_left = roi_left.height/2;
					// TODO:
					if(
						ball_centroid_left.x <=  IMAGE_WIDTH - ROI_DEFAULT_WIDTH_LEFT/2 &&
						ball_centroid_left.x >  ROI_DEFAULT_X_LEFT/2 &&
						ball_centroid_left.y <=  IMAGE_WIDTH - ROI_DEFAULT_HEIGHT_LEFT/2 &&
						ball_centroid_left.y >  ROI_DEFAULT_Y_LEFT/2 &&
						ball_centroid_right.x <=  IMAGE_WIDTH - ROI_DEFAULT_WIDTH_RIGHT/2 &&
						ball_centroid_right.x >  ROI_DEFAULT_X_RIGHT/2 &&
						ball_centroid_right.y <=  IMAGE_WIDTH - ROI_DEFAULT_HEIGHT_RIGHT/2 &&
						ball_centroid_right.y >  ROI_DEFAULT_Y_RIGHT/2
					){
						roi_left.x = ball_centroid_left.x;
						roi_left.y = ball_centroid_left.y;
						roi_right.x = ball_centroid_right.x;
						roi_right.y = ball_centroid_right.y;
					}
				}


				// TODO: Ball Trajectory algorithm
			}



			//findContours(frame_left_first_diff_thresh.clone()(roi_left), contours_left, hierarchy_left, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_left.x, roi_left.y));
			//findContours(frame_right_first_diff_thresh.clone()(roi_right), contours_right, hierarchy_right, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_right.x, roi_right.y));











			frame_left.copyTo(QS->IR.OutBuf1[0]);
			frame_right.copyTo(QS->IR.OutBuf1[1]);

			// This is how you move the catcher.  QS->moveX and QS->moveY (both in inches) must be calculated and set first.
			QS->Move_X = 0;					// replace 0 with your x coordinate
			QS->Move_Y = 0;					// replace 0 with your y coordinate
			SetEvent(QS->QSMoveEvent);		// Signal the move event to move catcher. The event will be reset in the move thread.
		}
		// Display Image
		if (QS->IR.UpdateImage) {
			for (i=0; i<QS->IR.NumCameras; i++) {
				if (QS->IR.CaptureSequence || QS->IR.DisplaySequence) {
#ifdef PTG_COLOR
					QS->IR.SaveBuf[i][QS->IR.FrameID].copyTo(QS->IR.DispBuf[i]);
#else
					QS->IR.OutBuf[0] = QS->IR.OutBuf[1] = QS->IR.OutBuf[2] = QS->IR.SaveBuf[i][QS->IR.FrameID];
					merge(QS->IR.OutBuf, 3, QS->IR.DispBuf[i]);
#endif
					sprintf_s(str,"%d",QS->IR.FrameID);
					putText(QS->IR.DispBuf[0], str, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
					if (QS->IR.PlayDelay) Sleep(QS->IR.PlayDelay);
				} else {
#ifdef PTG_COLOR
					QS->IR.ProcBuf[i][BufID].copyTo(QS->IR.DispBuf[i]);
#else
					// Display OutBuf1 when Catch Ball, otherwise display the input image
					QS->IR.OutBuf[0] = QS->IR.OutBuf[1] = QS->IR.OutBuf[2] = (QS->IR.CatchBall) ? QS->IR.OutBuf1[i] : QS->IR.ProcBuf[i];
					merge(QS->IR.OutBuf, 3, QS->IR.DispBuf[i]);
					// line(QS->IR.DispBuf[i], Point(0, 400), Point(640, 400), Scalar(0, 255, 0), 1, 8, 0);
#endif
				}
				QS->QSSysDisplayImage();
			}
		}
		if (QS->IR.CaptureSequence || QS->IR.DisplaySequence) {
			QS->IR.FrameID++;
			if (QS->IR.FrameID == MAX_BUFFER) {				// Sequence if filled
				QS->IR.CaptureSequence = FALSE;
				QS->IR.DisplaySequence = FALSE;
			} else {
				QS->IR.FrameID %= MAX_BUFFER;
			}
		}
		BufID = 1 - BufID;

		// Set previous frame for next loop iteration
		QS->IR.ProcBuf[0].copyTo(frame_left_prev);
		QS->IR.ProcBuf[1].copyTo(frame_right_prev);
	}
	QS->EventEndProcess = FALSE;
	return 0;
}

void CTCSys::QSSysInit()
{
	long i, j;
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
	char ErrorMsg[64];

	// How many cameras are on the bus?
	if(IR.busMgr.GetNumOfCameras((unsigned int *)&IR.NumCameras) != PGRERROR_OK){	// something didn't work correctly - print error message
		sprintf_s(ErrorMsg, "Connect Failure: %s", IR.PGRError.GetDescription());
		AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP );
	} else {
		IR.NumCameras = (IR.NumCameras > MAX_CAMERA) ? MAX_CAMERA : IR.NumCameras;
		for(i = 0; i < IR.NumCameras; i++) {
			// Get PGRGuid
			if (IR.busMgr.GetCameraFromIndex(i, &IR.prgGuid[i]) != PGRERROR_OK) {    // change to 1-i is cameras are swapped after powered up
				sprintf_s(ErrorMsg, "PGRGuID Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			IR.pgrCamera[i] = new Camera;
			if (IR.pgrCamera[i]->Connect(&IR.prgGuid[i]) != PGRERROR_OK) {
				sprintf_s(ErrorMsg, "PConnect Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set video mode and frame rate
			if (IR.pgrCamera[i]->SetVideoModeAndFrameRate(VIDEO_FORMAT, CAMERA_FPS) != PGRERROR_OK) {
				sprintf_s(ErrorMsg, "Video Format Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set all camera configuration parameters
			if (IR.pgrCamera[i]->SetConfiguration(&IR.cameraConfig) != PGRERROR_OK) {
				sprintf_s(ErrorMsg, "Set Configuration Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
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
				sprintf_s(ErrorMsg, "Shutter Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set gamma value
			IR.cameraProperty.type = GAMMA;
			IR.cameraProperty.absValue = 1.0;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
				sprintf_s(ErrorMsg, "Gamma Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set sharpness value
			IR.cameraProperty.type = SHARPNESS;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.valueA = 2000;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
				sprintf_s(ErrorMsg, "Sharpness Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
#ifdef  PTG_COLOR
			// Set white balance (R and B values)
			IR.cameraProperty = WHITE_BALANCE;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = WHITE_BALANCE_R;
			IR.cameraProperty.valueB = WHITE_BALANCE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
				ErrorMsg.Format("White Balance Failure: %s",IR.PGRError.GetDescription());
				AfxMessageBox( ErrorMsg, MB_ICONSTOP );
			}
#endif
			// Set gain values (350 here gives 12.32dB, varies linearly)
			IR.cameraProperty = GAIN;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = GAIN_VALUE_A;
			IR.cameraProperty.valueB = GAIN_VALUE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){
				sprintf_s(ErrorMsg, "Gain Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set trigger state
			IR.cameraTrigger.mode = 0;
			IR.cameraTrigger.onOff = TRIGGER_ON;
			IR.cameraTrigger.polarity = 0;
			IR.cameraTrigger.source = 0;
			IR.cameraTrigger.parameter = 0;
			if(IR.pgrCamera[i]->SetTriggerMode(&IR.cameraTrigger, false) != PGRERROR_OK){
				sprintf_s(ErrorMsg, "Trigger Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			IR.embeddedInfo[i].frameCounter.onOff = true;
			IR.embeddedInfo[i].timestamp.onOff = true;
			IR.pgrCamera[i]->SetEmbeddedImageInfo(&IR.embeddedInfo[i]);
			// Start Capture Individually
			if (IR.pgrCamera[i]->StartCapture() != PGRERROR_OK) {
				sprintf_s(ErrorMsg, "Start Capture Camera %d Failure: %s", i, IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
		}
		// Start Sync Capture (only need to do it with one camera)
//		if (IR.pgrCamera[0]->StartSyncCapture(IR.NumCameras, (const Camera**)IR.pgrCamera, NULL, NULL) != PGRERROR_OK) {
//				sprintf_s(ErrorMsg, "Start Sync Capture Failure: %s", IR.PGRError.GetDescription());
//				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP );
//		}
	}

#else
	IR.NumCameras = MAX_CAMERA;
#endif
	Rect R = Rect(0, 0, 640, 480);
	// create openCV image
	for(i=0; i<IR.NumCameras; i++) {
#ifdef PTG_COLOR
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		for (j=0; j<MAX_BUFFER; j++)
			IR.SaveBuf[i][j].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
#else
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		for (j=0; j<MAX_BUFFER; j++)
			IR.SaveBuf[i][j].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
#endif
		IR.AcqPtr[i] = IR.AcqBuf[i].data;
		IR.DispROI[i] = IR.DispBuf[i](R);
		IR.ProcROI[i] = IR.ProcBuf[i](R);

		IR.OutBuf1[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutBuf2[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI1[i] = IR.OutBuf1[i](R);
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
	for (int i = 0; i < 2; i++) {
		::SetDIBitsToDevice(
			ImageDC[i]->GetSafeHdc(), 1, 1,
			m_bitmapInfo.bmiHeader.biWidth,
			::abs(m_bitmapInfo.bmiHeader.biHeight),
			0, 0, 0,
			::abs(m_bitmapInfo.bmiHeader.biHeight),
			IR.DispBuf[i].data,
			&m_bitmapInfo, DIB_RGB_COLORS);
	}
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
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


	// The number of points before allowing the regression algorithm to run and predict points
	int const MINIMUM_REGRESSION_POINTS = 3;
	double const BALL_EMERGE_MEAN_THRESH = 0.01;
	float const MAX_DIFF_BALL_Y = 10.0f;
	float const MAX_DIFF_BALL_AREA = 200.0f;

	// inches below the camera, so add this to
	double const OFFSET_Y_CAMERA = 22.5;
	// inches
	double const OFFSET_X_CAMERA = 10.5;

	// The camera is at z=0, but the catcher is probably at z = -3
	double const CATCHER_Z = -3.0;

	// X and Y Scale value - +/- 9 inches to catcher is more like +/- 11 inches in real life
	// double const CATCHER_RANGE = 9.0;
	// double const CATCHER_REAL_RANGE = 11.0;
	//double const SCALE_X_CATCHER = 9.0/11.0;
	//double const SCALE_Y_CATCHER = 9.0/11.0;
	double const SCALE_X_CATCHER = 1.0;
	double const SCALE_Y_CATCHER = 1.0;

	// Reset catcher if nothing is happening
	int const EMPTY_FRAME_LIMIT = 10;
	int empty_frame_count = 0;

	// Create a hard reset after
	int const HARD_RESET_LIMIT = 100;
	int hard_reset_counter = 0;


	int const IMAGE_WIDTH = 640;
	int const IMAGE_HEIGHT = 480;


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

	// allocate an image buffer objects
	Mat frame_left;
	//Mat frame_left_original; // a copy of the original image, no processing
	Mat frame_left_first; // The image before the first detected ball movement
	Mat frame_left_prev;  // The previous frame
	Mat frame_left_prev_2;
	Mat frame_left_prev_3;

	Mat frame_right;
	//Mat frame_right_original;
	Mat frame_right_first; // The image before the first detected ball movement
	Mat frame_right_prev; // The previous frame
	Mat frame_right_prev_2;
	Mat frame_right_prev_3;

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

	// Load camera params from a file
	FileStorage baseball_params("baseball_params.yaml", FileStorage::READ);
	// NOTE: baseball_params.yaml needs to be in the same directory as HardWare.cpp!
	if (!baseball_params.isOpened()){
		char ErrorMsg[64];
		sprintf_s(ErrorMsg, "Failed to open baseball_params.yaml. Make sure it's in the base of BaseballCatcher.");
		AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
	}
	CV_Assert(baseball_params.isOpened());

	// Left and right camera params
	Mat camera_matrix_left, camera_matrix_right;
	Mat dist_coeffs_left, dist_coeffs_right;
	baseball_params["distortion_left"] >> dist_coeffs_left;
	baseball_params["intrinsic_left"] >> camera_matrix_left;
	baseball_params["distortion_right"] >> dist_coeffs_right;
	baseball_params["intrinsic_right"] >> camera_matrix_right;

	// Stereo params
	//Mat fundamental_matrix, essential_matrix;
	//Mat rmat, tvec;
	//baseball_params["fundamental"] >> fundamental_matrix;
	//baseball_params["rotation"] >> rmat;
	//baseball_params["translation"] >> tvec;
	//baseball_params["essential"] >> essential_matrix;

	// Stereo rectify parameters
	Mat rmat_left, rmat_right, pmat_left, pmat_right, Q;
	baseball_params["rmat_left"] >> rmat_left;
	baseball_params["rmat_right"] >> rmat_right;
	baseball_params["pmat_left"] >> pmat_left;
	baseball_params["pmat_right"] >> pmat_right;
	baseball_params["q"] >> Q;

	// Convert ball centroids to real-world coordinates using undistort points

	vector<Point2f> ball_centroids_left, ball_centroids_left_undistorted, ball_centroids_right, ball_centroids_right_undistorted;

	// Real world coordinates of the centroid of the ball
	// World points are based off of the left camera
	vector<Point3f> real_ball_path;

	// Change these to move the catcher
	double move_catcher_x = OFFSET_X_CAMERA;
	double move_catcher_y = -OFFSET_Y_CAMERA;


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


			// TODO: Keep a copy of the original around
			//QS->IR.ProcBuf[0].copyTo(frame_left_original);
			//QS->IR.ProcBuf[1].copyTo(frame_right_original);


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
				threshold(frame_left, frame_left, 10, 256, THRESH_BINARY);
				threshold(frame_right, frame_right, 10, 256, THRESH_BINARY);

				//// Trigger detection
				double mean_left = mean(frame_left(roi_left)).val[0];
				double mean_right = mean(frame_right(roi_right)).val[0];

				//// Average all the pixels and trigger if average goes above a certain number
				if (mean_left >= BALL_EMERGE_MEAN_THRESH && mean_right >= BALL_EMERGE_MEAN_THRESH){
					// putText(QS->IR.ProcBuf[0], to_string(mean_left), Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);
					// putText(QS->IR.ProcBuf[1], to_string(mean_right), Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);

					// Copy the the third image previous to be the background 'first' image
					//frame_left_prev.copyTo(frame_left_first);
					//frame_right_prev.copyTo(frame_right_first);

					frame_left_prev_3.copyTo(frame_left_first);
					frame_right_prev_3.copyTo(frame_right_first);

					ball_in_flight = true;
					// Reset ball path
					// TODO: Is this the right place to do this?
					real_ball_path.clear();
				}

				//// Detect when ball emerges
				//goodFeaturesToTrack(frame_left(roi_left), corners_left, 10, 0.01, 10);
				//goodFeaturesToTrack(frame_right(roi_right), corners_right, 10, 0.01, 10);

				//if(corners_left.size() > 0 && corners_left.size() > 0){
				//	// Set the previous image as the first image (background image)
				//	frame_left_prev.copyTo(frame_left_first);
				//	frame_right_prev.copyTo(frame_right_first);
				//	ball_in_flight = true;
				//}
			}
			else {
				putText(QS->IR.ProcBuf[0], "BALL IN FLIGHT", Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);

				// Ball should only be in flight for around 40 frames
				hard_reset_counter++;

				// Do processing on images now that the ball is in flight
				absdiff(frame_left, frame_left_first, frame_left);
				absdiff(frame_right, frame_right_first, frame_right);

				// Blur an roi
				GaussianBlur(frame_left(roi_left), frame_left(roi_left), Size(11, 11), 15.0, 15.0);
				GaussianBlur(frame_right(roi_right), frame_right(roi_right), Size(11, 11), 15.0, 15.0);

				// Threshold roi
				threshold(frame_left(roi_left), frame_left(roi_left), 10, 256, THRESH_BINARY);
				threshold(frame_right(roi_right), frame_right(roi_right), 10, 256, THRESH_BINARY);

				//// TODO: Ball tracking algorithm

				//// Find Contours
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

				//// cvtColor(frame_left_first_diff_thresh, frame_left_first_diff_thresh, COLOR_GRAY2BGR);
				////         cvtColor(frame_right_first_diff_thresh, frame_right_first_diff_thresh, COLOR_GRAY2BGR);
				////
				//// GET X,Y,Z location and move ROI
				if(contours_left.size() > 0 && contours_right.size() > 0){
					// We found a ball! Reset idle frame count
					empty_frame_count = 0;
					putText(QS->IR.ProcBuf[1], "BALL FOUND", Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);

					// cout << "contours_left[0]: " << contours_left[0] << endl;
					ball_contour_left = contours_left[0];
					ball_m_left = moments(ball_contour_left);
					ball_centroid_left = Point2f(ball_m_left.m10 / ball_m_left.m00, ball_m_left.m01 / ball_m_left.m00);
					// cout << "left centroid: " << ball_centroid_left << endl;
					// cout << "contours_right[0]: " << contours_right[0] << endl;
					ball_contour_right = contours_right[0];
					ball_m_right = moments(ball_contour_right);
					ball_centroid_right = Point2f(ball_m_right.m10 / ball_m_right.m00, ball_m_right.m01 / ball_m_right.m00);
					// cout << "right centroid: " << ball_centroid_right << endl;
					// cout << "drawContours!" << endl;

					float ball_y_diff = fabs((float)(ball_centroid_left.y - ball_centroid_right.y));
					float ball_area_diff = (float) fabs(ball_m_left.m00 - ball_m_right.m00);

					if( ball_y_diff < MAX_DIFF_BALL_Y && ball_area_diff < MAX_DIFF_BALL_AREA){
						//drawContours(frame_left, contours_left, -1, Scalar(0,0,255), 3, 8, hierarchy_left, 2, Point() );
						// Draw the centroid of the ball
						circle(frame_left, ball_centroid_left, 5, Scalar(0,0,0), 1);
						circle(frame_right, ball_centroid_right, 5, Scalar(0, 0, 0), 1);
						// recalculate the roi (roi widths and heights should be the same)
						int x_margin = roi_left.width/2;
						int y_margin = roi_left.height/2;
						// Make it so that ROI moves left or right still, even if it stops moving up or down
						if(
							ball_centroid_left.x <  IMAGE_WIDTH - x_margin &&
							ball_centroid_left.x >  x_margin &&
							ball_centroid_right.x <  IMAGE_WIDTH - x_margin &&
							ball_centroid_right.x >  x_margin
						){
							roi_left.x = ball_centroid_left.x - x_margin;
							roi_right.x = ball_centroid_right.x - x_margin;
						}

						// TODO: ROI.y should be tied together for both left and right
						if(
							ball_centroid_left.y <  IMAGE_HEIGHT - y_margin &&
							ball_centroid_left.y >  y_margin &&
							ball_centroid_right.y <  IMAGE_HEIGHT - y_margin &&
							ball_centroid_right.y >  y_margin
						){
							roi_left.y = ball_centroid_left.y - y_margin;
							roi_right.y = ball_centroid_right.y - y_margin;
						}

						// Convert centroid points to real worlds 3d points
						// Save the real 3d coordinates of the left centroid in real_ball_path
						vector<Point2f> ball_centroids_left_vec;
						ball_centroids_left_vec.push_back(ball_centroid_left);

						vector<Point2f> ball_centroids_right_vec;
						ball_centroids_right_vec.push_back(ball_centroid_right);

						vector<Point2f> ball_centroids_left_undistorted, ball_centroids_right_undistorted;

						undistortPoints(ball_centroids_left_vec, ball_centroids_left_undistorted, camera_matrix_left, dist_coeffs_left, rmat_left, pmat_left);
						undistortPoints(ball_centroids_right_vec, ball_centroids_right_undistorted, camera_matrix_right, dist_coeffs_right, rmat_right, pmat_right);

						float x_left = ball_centroids_left_undistorted[0].x;
						float y_left = ball_centroids_left_undistorted[0].y;
						float x_right = ball_centroids_right_undistorted[0].x;
						float y_right = ball_centroids_right_undistorted[0].y;

						// Create a 3 channel vector of (x,y,d), where x,y come from the 4 chosen points, and d is the x disparity between the two points
						// These vectors should only have one element each iteration of the loop
						vector<Point3f> ball_centroid_3d_left, ball_centroid_3d_right;
						// So disparity = x - x', or left - right
						ball_centroid_3d_left.push_back(Point3f(x_left, y_left, x_left - x_right));
						ball_centroid_3d_right.push_back(Point3f(x_right, y_right, x_left - x_right));
						CV_Assert(ball_centroid_3d_left.size() == 1);
						// Choose left points to transform
						vector<Point3f> ball_centroid_3d_real_left;
						perspectiveTransform(ball_centroid_3d_left, ball_centroid_3d_real_left, Q);
						// Save the real-world centroid location
						real_ball_path.push_back(ball_centroid_3d_real_left[0]);
						CV_Assert(ball_centroid_3d_real_left.size() == 1);

						// Print out the real-world coordinates of the ball
						stringstream real_coordinates;
						real_coordinates.str("");
						real_coordinates.clear();
						real_coordinates << "(" << to_string((int)ball_centroid_3d_real_left[0].x) << ", " << to_string((int)ball_centroid_3d_real_left[0].y) << ", " << to_string((int)ball_centroid_3d_real_left[0].z) << ")";
						putText(QS->IR.ProcBuf[0], real_coordinates.str(), Point2f(10, 410), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,255,255), 2);

						//
						//// Ball Trajectory algorithm
						//

						// Regression analysis Matrices
						// Extract the points from a vector in a for loop and create temp matrices
						// This is way easier to handle than having global matrices
						Mat A, B, Z, Y, X;

						for (int i = 0; i < real_ball_path.size(); ++i) {
							X.push_back(real_ball_path[i].x);
							Y.push_back(real_ball_path[i].y);
							// Create an temp 1x3 matrix
							Mat Z_row(1, 3, CV_32F);
							Z_row.at<float>(0, 0) = 1;
							Z_row.at<float>(0, 1) = real_ball_path[i].z;
							Z_row.at<float>(0, 2) = real_ball_path[i].z*real_ball_path[i].z;
							// cout << "Z_row: " << Z_row << endl;
							Z.push_back(Z_row);
						}

						// Only perform calculation if we have at least 3 points
						if(real_ball_path.size() >= MINIMUM_REGRESSION_POINTS){
							A = (Z.t()*Z).inv() * Z.t() * Y;
							B = (Z.t()*Z).inv() * Z.t() * X;

							// cout << "A: " << A << endl;
							// cout << "B: " << B << endl;

							// y = A[0] + A[1]*z + A[2]*z^2, where z is the catcher's z plane
							move_catcher_y = A.at<float>(0,0) + A.at<float>(1,0) * CATCHER_Z + A.at<float>(2,0) * CATCHER_Z * CATCHER_Z;
							// x = B[0] + B[1]*z + B[2]*z^2, where z is the catcher's z plane
							move_catcher_x = B.at<float>(0,0) + B.at<float>(1,0) * CATCHER_Z + B.at<float>(2,0) * CATCHER_Z * CATCHER_Z;

							stringstream predicted_coordinates;
							// Cast coordinates to int so they show up small in text
							predicted_coordinates << "(" << to_string((int)move_catcher_x) << ", " << to_string((int)move_catcher_y) << ")";
							cout << "Predicted Coordinates: " << predicted_coordinates.str() << endl;
							putText(QS->IR.ProcBuf[0], predicted_coordinates.str(), Point2f(10, 380), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
						}
						else {
							// cout << "Not enough real-world coordinates to do regression on" << endl;
						}

					}
					else {
						// Contours differed too much, so omit current data point
					}

					// TODO: Figure out the center location of the net relative to the left camera
				}
				else {
					// Nothing is happening
					empty_frame_count++;
				}

				// Reset the ball_in_flight bool after a certain number of frames with nothing detected
				// Also check for hard reset condition
				if (empty_frame_count > EMPTY_FRAME_LIMIT || hard_reset_counter > HARD_RESET_LIMIT){
					hard_reset_counter = 0;
					empty_frame_count = 0;

					// No ball has been found, so reset
					ball_in_flight = false;
					// Reset the roi
					roi_left = ROI_DEFAULT_LEFT;
					roi_right = ROI_DEFAULT_RIGHT;
					// TODO: Do we want the catcher to stay in the same place, or reset to origin?
					// TODO: Do we want the catcher to start to the right or left?
					// TODO: Create starting position constants
					move_catcher_x = OFFSET_X_CAMERA;
					move_catcher_y = -OFFSET_Y_CAMERA;
				}

			}

			// Paint resulting frames to the output
			QS->IR.ProcBuf[0].copyTo(QS->IR.OutBuf1[0]);
			QS->IR.ProcBuf[1].copyTo(QS->IR.OutBuf1[1]);
			frame_left(roi_left).copyTo(QS->IR.OutBuf1[0](roi_left));
			frame_right(roi_right).copyTo(QS->IR.OutBuf1[1](roi_right));

			// This is how you move the catcher.  QS->moveX and QS->moveY (both in inches) must be calculated and set first.
			// TODO: Are we offsetting in the right direction? Scaling correctly?
			QS->Move_X = (move_catcher_x - OFFSET_X_CAMERA) * SCALE_X_CATCHER;
			QS->Move_Y = (move_catcher_y + OFFSET_Y_CAMERA) * SCALE_Y_CATCHER;
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

		// Buffer the last 3 or 4 images
		// Buffer the third to prev image
		if (!frame_left_prev_2.empty()){
			frame_left_prev_2.copyTo(frame_left_prev_3);
			frame_right_prev_2.copyTo(frame_right_prev_3);
		}
		// Buffer the second to prev image
		if (!frame_left_prev.empty()){
			frame_left_prev.copyTo(frame_left_prev_2);
			frame_right_prev.copyTo(frame_right_prev_2);
		}

		// Set previous frame for next loop iteration
		//frame_left_original.copyTo(frame_left_prev);
		//frame_right_original.copyTo(frame_right_prev);

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
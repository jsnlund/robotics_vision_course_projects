// Testing regression algorithm for Project 3
//

// #include <opencv2/core.hpp>
// #include <opencv2/videoio.hpp>
// #include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip> // For leading zeros


using namespace cv;
using namespace std;

int main(int argc, char const *argv[]) {
	String const FOLDER = "12ms6\\";
	String const IMAGE_PREFIX_LEFT = "12ms6L";
	String const IMAGE_PREFIX_RIGHT = "12ms6R";
	String const IMAGE_FILE_SUFFIX = ".bmp";

	// Keyboard codes
	int const ESC_KEY = 27;
	int const C_KEY = 99; // Catchball key
	int keypress = 0;

	int const MIN_IMAGE_NUMBER = 1;
	int const MAX_IMAGE_NUMBER = 59;

	stringstream current_image_file_left;
	stringstream current_image_file_right;

	// Simulate the catch call button
	bool catch_ball = false;

	// Simulate ProcBuf
	Mat ProcBuf[2];
	// Simulate OutBuf1
	Mat OutBuf1[2];

	// Simulate Move vars
	double Move_X;
	double Move_Y;

	//
	//
	//  HardWare.cpp init code
	//
	//

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

	// ROI Initial Offset Constants
	int const ROI_DEFAULT_X_LEFT = 320;
	int const ROI_DEFAULT_Y_LEFT = 60;
	int const ROI_DEFAULT_WIDTH_LEFT = 100;
	int const ROI_DEFAULT_HEIGHT_LEFT = 100;

	int const ROI_DEFAULT_X_RIGHT = 235;
	int const ROI_DEFAULT_Y_RIGHT = 55;
	int const ROI_DEFAULT_WIDTH_RIGHT = ROI_DEFAULT_WIDTH_LEFT;
	int const ROI_DEFAULT_HEIGHT_RIGHT = ROI_DEFAULT_HEIGHT_LEFT;

	double const BALL_EMERGE_MEAN_THRESH = 0.01;

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

	// Load camera params from a file
	FileStorage baseball_params("baseball_params.yaml", FileStorage::READ);
	// NOTE: baseball_params.yaml needs to be in the same directory as HardWare.cpp!
	if (!baseball_params.isOpened()){
		cout << "Failed to open baseball_params.yaml..." << endl;
		return 0;
	}

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

	// Regression analysis Matricies
	Mat A, B;
	// Create a 1x3 initial Z array
	Mat Z(1, 3, DataType<float>::type);
	Mat Y(1, 1, DataType<float>::type);
	Mat X(1, 1, DataType<float>::type);

	// Change these to move the catcher
	double move_catcher_x = OFFSET_X_CAMERA;
	double move_catcher_y = -OFFSET_Y_CAMERA;

	//
	//
	//  End HardWare.cpp init code
	//
	//

	// Load the first image
	int i = MIN_IMAGE_NUMBER;
	current_image_file_left << FOLDER << IMAGE_PREFIX_LEFT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;
	current_image_file_right << FOLDER << IMAGE_PREFIX_RIGHT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;
	frame_left_prev = imread(current_image_file_left.str(), CV_LOAD_IMAGE_GRAYSCALE);
	frame_right_prev = imread(current_image_file_right.str(), CV_LOAD_IMAGE_GRAYSCALE);

	i++;

	while (keypress != ESC_KEY) {
		// Reset string stream
		// See http://stackoverflow.com/a/7623670/1416379
		current_image_file_left.str("");
		current_image_file_left.clear();
		current_image_file_right.str("");
		current_image_file_right.clear();

		// Calculate the full path to the image file
		current_image_file_left << FOLDER << IMAGE_PREFIX_LEFT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;
		current_image_file_right << FOLDER << IMAGE_PREFIX_RIGHT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;

		// Load the image
		// Simulate ProcBuf
		ProcBuf[0] = imread(current_image_file_left.str(), CV_LOAD_IMAGE_GRAYSCALE);
		ProcBuf[1] = imread(current_image_file_right.str(), CV_LOAD_IMAGE_GRAYSCALE);

		// Exit if no images were grabbed
		if( ProcBuf[0].empty() ) {
			cout <<  "Could not open or find the left image" << std::endl;
			// Show the image file path
			cout << "Left Image File: " << current_image_file_left.str() << endl;
			return -1;
		}

		// Exit if no images were grabbed
		if( ProcBuf[1].empty() ) {
			cout <<  "Could not open or find the right image" << std::endl;
			// Show the image file path
			cout << "Right Image File: " << current_image_file_left.str() << endl;
			return -1;
		}

		//
		//
		//  HardWare.cpp loop code
		//
		//
		// if (QS->IR.CatchBall) {  	// Click on "Catch" button to toggle the CatchBall flag when done catching
		if (catch_ball) {
			// Images are acquired into ProcBuf[0] for left and ProcBuf[1] for right camera
			// Need to create child image or small region of interest for processing to exclude background and speed up processing
			// Mat child = QS->IR.ProcBuf[i](Rect(x, y, width, height));

			ProcBuf[0].copyTo(frame_left);
			ProcBuf[1].copyTo(frame_right);

			// TODO: Keep a copy of the original around
			//ProcBuf[0].copyTo(frame_left_original);
			//ProcBuf[1].copyTo(frame_right_original);

			// Let the corners trigger the ball in flight bool once
			if(ball_in_flight == false){
				// Abs diff the whole image. It should be pretty fast
				// TODO: Use a slightly bigger roi?
				absdiff(ProcBuf[0], frame_left_prev, frame_left);
				absdiff(ProcBuf[1], frame_right_prev, frame_right);

				// Blur an roi
				GaussianBlur(frame_left(roi_left), frame_left(roi_left), Size(11, 11), 15.0, 15.0);
				GaussianBlur(frame_right(roi_right), frame_right(roi_right), Size(11, 11), 15.0, 15.0);

				// Threshold roi
				threshold(frame_left, frame_left, 10, 256, THRESH_BINARY);
				threshold(frame_right, frame_right, 10, 256, THRESH_BINARY);

				//// TODO: Replace corner detection method with a faster triggering method
				//// I.e. average all the pixels and trigger if average goes above a certain number
				double mean_left = mean(frame_left(roi_left)).val[0];
				double mean_right = mean(frame_right(roi_right)).val[0];

				if (mean_left >= BALL_EMERGE_MEAN_THRESH && mean_right >= BALL_EMERGE_MEAN_THRESH){
					putText(ProcBuf[0], to_string(mean_left), Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);
					putText(ProcBuf[1], to_string(mean_right), Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);

					// Copy the the third image previous to be the background 'first' image
					//frame_left_prev.copyTo(frame_left_first);
					//frame_right_prev.copyTo(frame_right_first);

					frame_left_prev_3.copyTo(frame_left_first);
					frame_right_prev_3.copyTo(frame_right_first);

					ball_in_flight = true;
					X.reshape(0, 1);
					Y.reshape(0, 1);
					Z.reshape(0, 1);
					// Reset regression matricies
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
				cout << "ball in flight" << endl;

				putText(ProcBuf[0], "BALL IN FLIGHT", Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);

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
					cout << "ball found" << endl;

					// We found a ball! Reset idle frame count
					empty_frame_count = 0;
					putText(ProcBuf[1], "BALL FOUND", Point(10, 470), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 255, 255), 2);

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
					//drawContours(frame_left, contours_left, -1, Scalar(0,0,255), 3, 8, hierarchy_left, 2, Point() );
					// Draw the centroid of the ball
					circle(frame_left, ball_centroid_left, 5, Scalar(0,0,0), 1);
					circle(frame_right, ball_centroid_right, 5, Scalar(0, 0, 0), 1);
					// recalculate the roi (roi widths and heights should be the same)
					int x_margin = roi_left.width/2;
					int y_margin = roi_left.height/2;
					if(
						ball_centroid_left.x <  IMAGE_WIDTH - x_margin &&
						ball_centroid_left.x >  x_margin &&
						ball_centroid_left.y <  IMAGE_HEIGHT - y_margin &&
						ball_centroid_left.y >  y_margin &&

						ball_centroid_right.x <  IMAGE_WIDTH - x_margin &&
						ball_centroid_right.x >  x_margin &&
						ball_centroid_right.y <  IMAGE_HEIGHT - y_margin &&
						ball_centroid_right.y >  y_margin
					){
						roi_left.x = ball_centroid_left.x - x_margin;
						roi_left.y = ball_centroid_left.y - y_margin;
						roi_right.x = ball_centroid_right.x - x_margin;
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
					// So disparity = x - x', or left - right

					// Create a 3 channel vector of (x,y,d), where x,y come from the 4 chosen points, and d is the x disparity between the two points
					// These vectors should only have one element each iteration of the loop
					vector<Point3f> ball_centroid_3d_left, ball_centroid_3d_right;
					ball_centroid_3d_left.push_back(Point3f(x_left, y_left, x_left - x_right));
					ball_centroid_3d_right.push_back(Point3f(x_right, y_right, x_left - x_right));
					// Make sure that there is only one element in the array!
					cout << "ball_centroid_3d_left: " << ball_centroid_3d_left << endl;
					CV_Assert(ball_centroid_3d_left.size() == 1);

					// Choose left points to transform
					vector<Point3f> ball_centroid_3d_real_left;
					perspectiveTransform(ball_centroid_3d_left, ball_centroid_3d_real_left, Q);

					// Save the real-world centroid location
					real_ball_path.push_back(ball_centroid_3d_real_left[0]);
					// Make sure that there is only one element in the array!
					cout << "ball_centroid_3d_real_left: " << ball_centroid_3d_real_left << endl;
					CV_Assert(ball_centroid_3d_real_left.size() == 1);


					// Print out the real-world coordinates of the ball
					stringstream real_coordinates;
					real_coordinates.str("");
					real_coordinates.clear();
					real_coordinates << "(" << to_string((int)ball_centroid_3d_real_left[0].x) << ", " << to_string((int)ball_centroid_3d_real_left[0].y) << ", " << to_string((int)ball_centroid_3d_real_left[0].z) << ")";
					putText(ProcBuf[0], real_coordinates.str(), Point2f(10, 410), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,255,255), 2);

					cout << "Before Matrix Math" << endl;

					//
					//// Ball Trajectory algorithm
					//
					Point3f real_point = real_ball_path.back();

					X.push_back(real_point.x);
					Y.push_back(real_point.y);
					// Create an temp Nx3 matrix
					Mat Z_temp(1, 3, DataType<float>::type);
					Z_temp.at<float>(0, 0) = 1;
					Z_temp.at<float>(0, 1) = real_point.z;
					Z_temp.at<float>(0, 2) = real_point.z*real_point.z;
					Z.push_back(Z_temp);
					//// Calculate A and B
					A = (Z.t()*Z).inv() * Z.t() * Y;
					B = (Z.t()*Z).inv() * Z.t() * X;

					cout << "A: " << A << endl;
					cout << "B: " << B << endl;

					cout << "A(0,0): " << A.at<float>(0,0) << endl;
					cout << "A(1,0): " << A.at<float>(1,0) << endl;
					cout << "A(2,0): " << A.at<float>(2,0) << endl;
					cout << "A(0,1): " << A.at<float>(0,1) << endl;
					cout << "A(0,2): " << A.at<float>(0,2) << endl;


					// y = A[0] + A[1]*z + A[2]*z^2, where z is the catcher's z plane
					move_catcher_y = A.at<float>(0,0) + A.at<float>(1,0) * CATCHER_Z + A.at<float>(2,0) * CATCHER_Z * CATCHER_Z;
					// x = B[0] + B[1]*z + B[2]*z^2, where z is the catcher's z plane
					move_catcher_x = B.at<float>(0,0) + B.at<float>(0,0) * CATCHER_Z + B.at<float>(0,0) * CATCHER_Z * CATCHER_Z;

					cout << "After Matrix Math" << endl;
					stringstream predicted_coordinates;
					predicted_coordinates << "(" << to_string((int)move_catcher_x) << ", " << to_string((int)move_catcher_y) << ")";
					cout << "Predicted Coordinates: " << predicted_coordinates.str() << endl;
					putText(ProcBuf[0], predicted_coordinates.str(), Point2f(10, 380), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);

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
			ProcBuf[0].copyTo(OutBuf1[0]);
			ProcBuf[1].copyTo(OutBuf1[1]);
			frame_left(roi_left).copyTo(OutBuf1[0](roi_left));
			frame_right(roi_right).copyTo(OutBuf1[1](roi_right));

			// This is how you move the catcher.  QS->moveX and QS->moveY (both in inches) must be calculated and set first.
			// TODO: Are we offsetting in the right direction? Scaling correctly?
			Move_X = (move_catcher_x - OFFSET_X_CAMERA) * SCALE_X_CATCHER;
			Move_Y = (move_catcher_y + OFFSET_Y_CAMERA) * SCALE_Y_CATCHER;
			cout << "**Moving X to " << Move_X << " and Y to " << Move_Y << "**" << endl;
			// SetEvent(QS->QSMoveEvent);		// Signal the move event to move catcher. The event will be reset in the move thread.
			imshow("OutBuf1 Left", OutBuf1[0]);
			imshow("OutBuf1 Right", OutBuf1[1]);
		}


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

		ProcBuf[0].copyTo(frame_left_prev);
		ProcBuf[1].copyTo(frame_right_prev);


		//
		//
		//  End HardWare.cpp loop code
		//
		//

		// Show the image output for a sanity check
		imshow("ProcBuf Left", ProcBuf[0]);
		imshow("ProcBuf Right", ProcBuf[1]);

		// Need this for images to display, or else output windows just show up gray
		keypress = waitKey(30);

		// Simulate the catch ball button - toggle it
		if(keypress == C_KEY) {
			catch_ball = !catch_ball;
		}

		i++;
		if(i > MAX_IMAGE_NUMBER) {
			i = MIN_IMAGE_NUMBER;
		}
	}

	return 0;
}
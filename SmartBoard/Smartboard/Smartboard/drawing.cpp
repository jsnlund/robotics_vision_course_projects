#include <opencv2/opencv.hpp>

#include "drawing.h"
#include "globals.h"

using namespace std;
using namespace cv;


void draw_(Mat *frame_camera, Mat *frame_projector, Mat perspective_transform) {


	Mat frame_ink = Mat(Size(IMAGE_WIDTH,IMAGE_HEIGHT), CV_8UC3);
	frame_calibration.setTo(BLACK);

	Mat frame_camera_hsv, frame_camera_hsv_binary;
    cvtColor(*frame_camera, frame_camera_hsv, COLOR_BGR2HSV);

    inRange(frame_camer_hsv, Scalar(HUE_MARKER - HUE_MARKER_BUFFER, MIN_SAT, MIN_VAL), Scalar(HUE_MARKER + HUE_MARKER_BUFFER, MAX_SAT, MAX_VAL), frame_camera_hsv_binary);

    // imshow("Camera 2 binary", frame_camera_hsv_binary);
    // Do erode and dilate
    int const ITERATIONS = 5;
    erode(frame_camera_hsv_binary, frame_camera_hsv_binary, Mat(), Point(-1,-1), ITERATIONS);
    dilate(frame_camera_hsv_binary, frame_camera_hsv_binary, Mat(), Point(-1,-1), ITERATIONS);

    // Find contours
    Mat frame_contours;
    cvtColor(frame_camera_hsv_binary, frame_contours, COLOR_GRAY2BGR);


    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    // Use CV_RETR_EXTERNAL instead of TREE, so we only grab outer-most contours
    // Use CV_CHAIN_APPROX_SIMPLE to store less points
    findContours(frame_camera_hsv_binary.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    // find max area contour
    // get centroid of max area contour
    // if point inside projector area - secondary priority
    // transform centroid to projector frame - prev_pt will already be in projector frame
    // draw line from prev_pt to centroid

    // circle(frame_contours, centroid, 5, GREEN, 3);

    prev_pt = centroid;


}
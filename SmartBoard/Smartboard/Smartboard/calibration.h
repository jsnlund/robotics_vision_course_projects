#pragma once

#include <opencv2/opencv.hpp>
using namespace cv;

// enum CalibrationCommand { ACQUIRE, CALCULATE };

// RGB to HSV converter
// HSV = (Hue, Saturation, Value)
// http://www.rapidtables.com/convert/color/rgb-to-hsv.htm
// Create a buffer of hue values, so hue +/- HUE_BUFFER
int const HUE_MARKER_BUFFER = 15;
int const MIN_SAT = (int) 0.3 * 255;
int const MAX_SAT = (int) 1.0 * 255;
int const MIN_VAL = (int) 0.1 * 255;
int const MAX_VAL = (int) 1.0 * 255;
int const HUE_MARKER = 55;

// if the command is to acquire, it produces a green rectangle on the projector frame and tries to find it in the camera frame (returns an empty Mat)
// if the command is to calculate, it takes the currently detected calibration rectangle and calculates the perspective transform Mat from that
Mat calibrate_smartboard(Mat *frame_camera, Mat *frame_projector, bool calculate_transform);

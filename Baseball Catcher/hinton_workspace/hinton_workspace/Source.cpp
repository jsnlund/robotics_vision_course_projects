// Project 3 Workspace for Michael Hinton
//
// TODO: Test out abs diff to locate baseball
// Calculate ROI
// TODO: Calibrate camera?
// TODO: Look at HW 3 task 5 code for real-world points?

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char const *argv[]) {
    String const FOLDER = "shot_10\\";
    String const IMAGE_PREFIX_LEFT = "L";
    String const IMAGE_PREFIX_RIGHT = "R";
    String const IMAGE_FILE_SUFFIX = ".bmp";
    String helper = "0";

    // Keyboard codes
    int const ESC_KEY = 27;
    int keypress = 0;

    int const MIN_IMAGE_NUMBER = 0;
    int const MAX_IMAGE_NUMBER = 127;

    Mat frame_left; // allocate an image buffer object
    Mat frame_left_prev;
    Mat frame_left_diff;
    Mat frame_left_thresh;

    Mat frame_right; // allocate an image buffer object
    Mat frame_right_prev;
    Mat frame_right_diff;
    Mat frame_right_thresh;

    // ROI Offset Constants
    int const ROI_LEFT_X = 200;
    int const ROI_LEFT_Y = 0;
    int const ROI_LEFT_WIDTH = 400;
    int const ROI_LEFT_HEIGHT = 200;

    int const ROI_RIGHT_X = 200;
    int const ROI_RIGHT_Y = 0;
    int const ROI_RIGHT_WIDTH = 200;
    int const ROI_RIGHT_HEIGHT = 200;




    String current_image_file_left;
    String current_image_file_right;


    // A vector to hold an array of corner coordinates
    vector<Point2f> corners_left, corners_right;


    // Load the first image
    int i = MIN_IMAGE_NUMBER;
    current_image_file_left = FOLDER + IMAGE_PREFIX_LEFT + helper + to_string(MIN_IMAGE_NUMBER) + IMAGE_FILE_SUFFIX;
    current_image_file_right = FOLDER + IMAGE_PREFIX_RIGHT + helper + to_string(MIN_IMAGE_NUMBER) + IMAGE_FILE_SUFFIX;
    frame_left_prev = imread(current_image_file_left, CV_LOAD_IMAGE_GRAYSCALE);
    frame_right_prev = imread(current_image_file_right, CV_LOAD_IMAGE_GRAYSCALE);
    i++;

    // for (int i = MIN_IMAGE_NUMBER; i <= MAX_IMAGE_NUMBER; ++i) {
    while (keypress != ESC_KEY) {
        // Accommodate switch from 0x to xx
        if(i < 10 && i >= 0){
            helper = "0";
        }
        else {
            helper = "";
        }

        // Calculate the full path to the image file
        current_image_file_left = FOLDER + IMAGE_PREFIX_LEFT + helper + to_string(i) + IMAGE_FILE_SUFFIX;
        current_image_file_right = FOLDER + IMAGE_PREFIX_RIGHT + helper + to_string(i) + IMAGE_FILE_SUFFIX;

        // Load the image
        frame_left = imread(current_image_file_left, CV_LOAD_IMAGE_GRAYSCALE);
        frame_right = imread(current_image_file_right, CV_LOAD_IMAGE_GRAYSCALE);

        // Exit if no images were grabbed
        if( frame_left.empty() ) {
            cout <<  "Could not open or find the left image" << std::endl ;
            // Show the image file path
            cout << "Left Image File: " << current_image_file_left << endl;
            return -1;
        }

        // Exit if no images were grabbed
        if( frame_right.empty() ) {
            cout <<  "Could not open or find the right image" << std::endl ;
            // Show the image file path
            cout << "Right Image File: " << current_image_file_left << endl;
            return -1;
        }

        //
        //// Process the image
        //

        // Absolute difference
        absdiff(frame_left, frame_left_prev, frame_left_diff);
        absdiff(frame_right, frame_right_prev, frame_right_diff);

        // Try other blurring factors and sizes
        // Sizes must be an odd number
        // Try Size(31,31), 15, 15 to get rid of curtain movement
        GaussianBlur(frame_left_diff, frame_left_thresh, Size(7,7), 3.0, 3.0);
        GaussianBlur(frame_right_diff, frame_right_thresh, Size(7,7), 3.0, 3.0);

        threshold(frame_left_thresh, frame_left_thresh, 5, 256, THRESH_BINARY);
        threshold(frame_right_thresh, frame_right_thresh, 5, 256, THRESH_BINARY);
        // adaptiveThreshold(frame_right_diff, frame_right_thresh, 5, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, 0.0);

        // TODO: Try out corner detection
        corners_left.clear();
        corners_right.clear();
        // TODO: Apply ROI
        goodFeaturesToTrack(frame_left_thresh(Rect(ROI_LEFT_X,ROI_LEFT_Y,ROI_LEFT_WIDTH,ROI_LEFT_HEIGHT)), corners_left, 10, 0.01, 10);
        goodFeaturesToTrack(frame_right_thresh(Rect(ROI_RIGHT_X,ROI_RIGHT_Y,ROI_RIGHT_WIDTH,ROI_RIGHT_HEIGHT)), corners_right, 10, 0.01, 10);
        // goodFeaturesToTrack(frame_left_thresh, corners_left, 10, 0.01, 10);
        // goodFeaturesToTrack(frame_right_thresh, corners_right, 10, 0.01, 10);
        // cornerSubPix(frame_left_thresh, corners_left, Size(5,5), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 ) );
        // cornerSubPix(frame_right_thresh, corners_right, Size(5,5), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 ) );

        cvtColor(frame_left_thresh, frame_left_thresh, COLOR_GRAY2BGR);
        cvtColor(frame_right_thresh, frame_right_thresh, COLOR_GRAY2BGR);
        // Draw detected corners
        for (int i = 0; i < corners_left.size(); ++i){
            circle(frame_left_thresh, Point(ROI_LEFT_X+corners_left[i].x,ROI_LEFT_Y+corners_left[i].y), 20, Scalar(200,80,0), 3);
        }
        for (int i = 0; i < corners_right.size(); ++i){
            circle(frame_right_thresh, Point(ROI_RIGHT_X+corners_right[i].x,ROI_RIGHT_Y+corners_right[i].y), 20, Scalar(100,180,80), 3);
        }

        // Show the image output for a sanity check
        imshow("Left", frame_left);
        imshow("Right", frame_right);

        imshow("Left Diff", frame_left_diff);
        imshow("Right Diff", frame_right_diff);

        imshow("Left Threshold", frame_left_thresh);
        imshow("Right Threshold", frame_right_thresh);

        // Need this for images to display, or else output windows just show up gray
        keypress = waitKey(30);

        // Update prev frames with current frames
        frame_left.copyTo(frame_left_prev);
        frame_right.copyTo(frame_right_prev);

        i++;
        if(i > MAX_IMAGE_NUMBER) {
            i = MIN_IMAGE_NUMBER;
        }
    }

    return 0;
}
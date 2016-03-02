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

    Mat frame_right; // allocate an image buffer object
    Mat frame_right_prev;
    Mat frame_right_diff;

    String current_image_file_left;
    String current_image_file_right;


    // Load the first image
    int i = MIN_IMAGE_NUMBER;
    current_image_file_left = FOLDER + IMAGE_PREFIX_LEFT + helper + to_string(MIN_IMAGE_NUMBER) + IMAGE_FILE_SUFFIX;
    current_image_file_right = FOLDER + IMAGE_PREFIX_RIGHT + helper + to_string(MIN_IMAGE_NUMBER) + IMAGE_FILE_SUFFIX;
    frame_left_prev = imread(current_image_file_left, CV_LOAD_IMAGE_COLOR);
    frame_right_prev = imread(current_image_file_right, CV_LOAD_IMAGE_COLOR);
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
        frame_left = imread(current_image_file_left, CV_LOAD_IMAGE_COLOR);
        frame_right = imread(current_image_file_right, CV_LOAD_IMAGE_COLOR);

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

        // cvtColor(frame_display_left, frame_display_left, COLOR_BGR2GRAY);
        // cvtColor(frame_display_right, frame_display_right, COLOR_BGR2GRAY);

        //// Process the image


        // Absolute difference
        absdiff(frame_left, frame_left_prev, frame_left_diff);
        absdiff(frame_right, frame_right_prev, frame_right_diff);


        // Show the image output for a sanity check
        imshow("Left", frame_left);
        imshow("Right", frame_right);

        imshow("Left Diff", frame_left_diff);
        imshow("Right Diff", frame_right_diff);

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
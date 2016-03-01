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
    String const IMAGE_FILE_PREFIX = "12ms1";
    String const IMAGE_FILE_SUFFIX = ".bmp";

    int MAX_CALIBRATION_IMAGE_COUNT = 40;

    Mat frame; // The originals
    Mat frame_prev; // The previous frame
    // Mat frame_undistored; // After calling undistort
    Mat frame_diff; // After using absdiff

    // Initialize a 3x3 matrix
    Mat cameraMatrix = Mat::zeros(3, 3, CV_64F);
    // Initialize 5x1 coefficient matrix
    Mat distCoeffs = Mat::zeros(5, 1, CV_64F);

    //// Read in intrinsic and distortion parameters from a file
    // cout << cameraMatrix << endl;
    // cout << distCoeffs << endl;

    // Read yaml files using FileStorage
    FileStorage fs("task5\\task5_parameters.yaml", FileStorage::READ);

    fs["distortion"] >> distCoeffs;
    fs["intrinsic"] >> cameraMatrix;

    cout << "Printing info from YAML files!" << endl;
    cout << distCoeffs << endl;
    cout << cameraMatrix << endl;

    // load all 40 images and calculate the diff between the original and undistorted versions
    for (int i = 0; i < MAX_CALIBRATION_IMAGE_COUNT; ++i) {
        frame = imread(IMAGE_FILE_PREFIX + to_string(i) + IMAGE_FILE_SUFFIX, CV_LOAD_IMAGE_COLOR);

        // Undistort the images
        undistort(frame, frame_undistored, cameraMatrix, distCoeffs);

        // Absolute difference
        absdiff(frame, frame_undistored, frame_diff);

        // Save the diff images
        if(!imwrite("task6\\diff" + to_string(i) + IMAGE_FILE_SUFFIX, frame_diff)){
            cout << "ERROR: Image \"task6\\diff" + to_string(i) + IMAGE_FILE_SUFFIX + "\" failed to write! Does the destination folder exist?" << endl;
            return 0;
        }
    }

    cout << "Absolute diffs of the calibration images successfully produced!" << endl;

    return 0;
}
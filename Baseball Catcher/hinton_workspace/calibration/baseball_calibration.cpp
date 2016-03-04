// Use "Right Camera" and "Left Camera2" sets of images from "Our Images" zip file
// Put both folders in the folder "task1", and rename them "left" and "right"
// There are 32 images each, going from 0 to 31

// Folders and files that are needed:
//      left_calib/
//      right_calib/
//      stereo_calib/

// Creates an output yaml file called baseball_params.yaml

// Example usage:
//      ./baseball_calibration.exe
//      ./baseball_calibration.exe > baseball_params.txt


// #include <opencv2/core.hpp>
// #include <opencv2/videoio.hpp>
// #include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp> // For cvtColor, Canny, GaussianBlur, etc.
// For file input/output
#include <iostream>
#include <iomanip> // To increase output precision
#include <fstream>

using namespace cv;
using namespace std;

// Cool trick: compilers automatically concatenate adjacent string literals! "+" won't work
// string const USAGE_DESCRIPTION =
// "Usage:  baseball_calibration.exe\n"
// "    *Use the -r flag to calibrate with the right camera images\n"
// "    *Run without the -r flag to calibrate with the left camera images\n";


int main(int argc, char const *argv[]){
    //
    //// Set image calibration constants
    //

    String const FOLDER_CALIB_LEFT = "left_calib\\";
    String const FILE_PREFIX_CALIB_LEFT = "LeftCamera2L";
    int const CALIB_LEFT_MIN_NUMBER = 0;
    int const CALIB_LEFT_MAX_NUMBER = 31;
    int const IMAGE_COUNT_LEFT = CALIB_LEFT_MAX_NUMBER - CALIB_LEFT_MIN_NUMBER + 1;

    String const FOLDER_CALIB_RIGHT = "right_calib\\";
    String const FILE_PREFIX_CALIB_RIGHT = "RightCameraR";
    int const CALIB_RIGHT_MIN_NUMBER = 0;
    int const CALIB_RIGHT_MAX_NUMBER = 31;
    int const IMAGE_COUNT_RIGHT = CALIB_RIGHT_MAX_NUMBER - CALIB_RIGHT_MIN_NUMBER + 1;

    String const FOLDER_CALIB_STEREO = "stereo_calib\\";
    String const FILE_PREFIX_STEREO = "Stereo";
    int const CALIB_STEREO_MIN_NUMBER = 0;
    int const CALIB_STEREO_MAX_NUMBER = 31;
    int const IMAGE_COUNT_STEREO = CALIB_STEREO_MAX_NUMBER - CALIB_STEREO_MIN_NUMBER + 1;


    // Should all be .bmp
    String const IMAGE_FILE_SUFFIX = ".bmp";

    // int const IMAGE_WIDTH = 640;
    // int const IMAGE_HEIGHT = 480;


    //
    //// Chessboard vars for left, right, and stereo:
    //

    // The calibration chessboard has 11 columns x 8  rows of squares
    // We want the number of corners formed when black square corners touch, so one less row and column
    int const CHESSBOARD_COLUMNS = 11;
    int const CHESSBOARD_ROWS = 8;
    Size const PATTERN_SIZE =  Size(CHESSBOARD_COLUMNS-1,CHESSBOARD_ROWS-1);
    // The chessboard has 3.88 inch squares. Constants are assumed double unless specified
    float const CHESSBOARD_SQUARE_LENGTH = 3.88f;
    // Figure out how many corners are in the chessboard
    int const CHESSBOARD_CORNERS_COUNT = (CHESSBOARD_ROWS-1)*(CHESSBOARD_COLUMNS-1);
    // If chessboard points were found, then
    bool pattern_was_found = false;
    int const FIND_CHESSBOARD_FLAGS = CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE;

    // A vector to hold the current array of chessboard corner coordinates
    // vector<Point2f> corners_left, corners_right, corners_stereo_left, corners_stereo_right;
    vector<Point2f> chessboard_corners;
    // Object points should be the same for left, right, and stereo
    vector<Point3f> object_points;
    for (int i = 0; i < CHESSBOARD_CORNERS_COUNT; ++i) {
        // Create object_points once
        object_points.push_back(
            Point3f(
                // Take advantage of integer division, then cast to float
                // Multiply each point by 3.88 inches!!!
                float(i%(CHESSBOARD_COLUMNS-1))*CHESSBOARD_SQUARE_LENGTH,
                float(i/(CHESSBOARD_COLUMNS-1))*CHESSBOARD_SQUARE_LENGTH,
                0
            )
        );
        // bottom-right corner is x,y,z = (0,0,0). Moving to the left, it goes (0,0,0), (1,0,0), (2,0,0) ...
        // Then, when it reaches the end of the row, restarts to the right at (0,1,0), (1,1,0), (2,1,0) ...
    }


    //
    //// Left Camera Calibrate
    //

    vector<vector<Point3f>> object_points_vector_left, object_points_vector_right;
    // These are the pixel points of the located corners in the image
    vector<Point2f> image_points_left, image_points_right;
    vector<vector<Point2f>> image_points_vector_left, image_points_vector_right;

    stringstream calib_image_file_left;
    stringstream calib_image_file_right;

    // Current image buffers
    Mat frame;
    Mat frame_display;

    // Mat frameLeft; // allocate an image buffer object
    // Mat frameDisplayLeft; // the frame to work with

    // Mat frameRight; // allocate an image buffer object
    // Mat frameDisplayRight; // the frame to work with

    // Loop through each calibration image and extract a list of image points
    for (int i = CALIB_LEFT_MIN_NUMBER; i <= CALIB_LEFT_MAX_NUMBER; ++i) {
        // Load the image
        calib_image_file_left.str("");
        calib_image_file_left.clear();
        calib_image_file_left << FOLDER_CALIB_LEFT << FILE_PREFIX_CALIB_LEFT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;
        frame = imread(calib_image_file_left.str(), CV_LOAD_IMAGE_COLOR);

        // Exit if no images were grabbed
        if( frame.empty() ) {
            cout <<  "Could not open or find the image. Did you unzip the calibration images?" << std::endl ;
            // Show the image file path
            cout << "Image File: " << calib_image_file_left.str() << endl;
            return -1;
        }

        //  Get a grayscale copy to work on
        frame.copyTo(frame_display);
        cvtColor(frame_display, frame_display, COLOR_BGR2GRAY);

        // Clear corner values, so that it doesn't grow
        image_points_left.clear();

        //// Process the image
        pattern_was_found = findChessboardCorners(frame_display, PATTERN_SIZE, chessboard_corners, FIND_CHESSBOARD_FLAGS);
        if(!pattern_was_found){
            cout << "Chessboard Corners not found!!" << endl;
            cout << "Image File: " << calib_image_file_left.str() << endl;
            // if chessboard is not found, omit the image
            continue;
        }

        cornerSubPix(frame_display, chessboard_corners, Size(11,11), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1 ));

        // Make sure that we have all the corners we expect...
        CV_Assert(chessboard_corners.size() == CHESSBOARD_CORNERS_COUNT);

        // Extract all the 2-d corner points into image_point_lefts
        for (int i = 0; i < chessboard_corners.size(); ++i) {
            image_points_left.push_back(chessboard_corners[i]);
        }
        // cout << image_points_left << endl_left;

        // Save the 2-d and 3-d vectors to a vector of vectors
        object_points_vector_left.push_back(object_points);
        image_points_vector_left.push_back(image_points_left);
    }



    // Initialize a 3x3 matrix that will be overwritten (64F is a double, not a float)
    Mat camera_matrix_left = Mat::zeros(3, 3, CV_64F);
    // Initialize 8x1 coefficient matrix (64F is a double, not a float)
    Mat dist_coeffs_left = Mat::zeros(8, 1, CV_64F);
    // Throwaway values - we don't care about rvec and tvec until stereo
    vector<Mat> rvecs, tvecs;

    // There should be equal amounts of elements
    CV_Assert(object_points_vector_left.size() == image_points_vector_left.size());
    cout << "Calibrating camera with " << object_points_vector_left.size() << " valid images out of a total of " << IMAGE_COUNT_LEFT << endl;

    // Calibrate the LEFT camera with the extracted points
    calibrateCamera(object_points_vector_left, image_points_vector_left, frame_display.size(), camera_matrix_left, dist_coeffs_left, rvecs, tvecs);

    // // Clear for the right side
    // object_points_vector.clear();


    //
    //// Right Camera Calibrate
    //



    // // CV_LOAD_IMAGE_COLOR CV_LOAD_IMAGE_COLOR
    // for (int i = CALIB_RIGHT_MIN_NUMBER; i <= CALIB_RIGHT_MAX_NUMBER; ++i) {
    //     // Load the image
    //     calib_image_file_right << FOLDER_CALIB_RIGHT << FILE_PREFIX_CALIB_RIGHT << setw(2) << setfill('0') << to_string(MIN_IMAGE_NUMBER) << IMAGE_FILE_SUFFIX;
    //     frame = imread(calib_image_file_right.str(), CV_LOAD_IMAGE_COLOR);
    //     // Clear corner values, so that it doesn't grow
    //     image_points.clear();

    //     // Exit if no images were grabbed
    //     if( frame.empty() ) {
    //         cout <<  "Could not open or find the image. Did you unzip the calibration images?" << std::endl ;
    //         // Show the image file path
    //         cout << "Image File: " << calib_image_file_right.str() << endl;
    //         return -1;
    //     }

    //     //  Get a grayscale copy to work on
    //     frame.copyTo(frame_display);
    //     cvtColor(frame_display, frame_display, COLOR_BGR2GRAY);


    // }







    //
    //// Stereo Calibrate
    //














    //
    //// Stereo Rectify
    //








    //
    //// Save Parameters to a file
    //


    // Save files as yaml using FileStorage
    // string output_file = "baseball_params.yaml";
    FileStorage baseball_params("baseball_params.yaml", FileStorage::WRITE);
    // Left camera calibration
    baseball_params << "intrinsic_left" << camera_matrix_left;
    baseball_params << "distortion_left" << dist_coeffs_left;
    // // Right camera calibration
    // baseball_params << "intrinsic_right" << camera_matrix_right;
    // baseball_params << "distortion_right" << dist_coeffs_right;
    // // Stereo calibration
    // baseball_params << "fundamental" << fundamental_matrix;
    // baseball_params << "translation" << tvec;
    // baseball_params << "rotation" << rmat;
    // baseball_params << "essential" << essential_matrix;
    // // Stereo rectification
    // baseball_params << "rmat_left" << rmat_left;
    // baseball_params << "rmat_right" << rmat_right;
    // baseball_params << "pmat_left" << pmat_left;
    // baseball_params << "pmat_right" << pmat_right;
    // baseball_params << "q" << Q;

    baseball_params.release();

    return 0;
}


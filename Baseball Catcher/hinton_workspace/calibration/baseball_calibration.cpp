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
    int const MIN_NUMBER_CALIB_LEFT = 0;
    int const MAX_NUMBER_CALIB_LEFT = 31;
    int const IMAGE_COUNT_LEFT = MAX_NUMBER_CALIB_LEFT - MIN_NUMBER_CALIB_LEFT + 1;

    String const FOLDER_CALIB_RIGHT = "right_calib\\";
    String const FILE_PREFIX_CALIB_RIGHT = "RightCameraR";
    int const MIN_NUMBER_CALIB_RIGHT = 0;
    int const MAX_NUMBER_CALIB_RIGHT = 31;
    int const IMAGE_COUNT_RIGHT = MAX_NUMBER_CALIB_RIGHT - MIN_NUMBER_CALIB_RIGHT + 1;

    String const FOLDER_CALIB_STEREO = "stereo_calib\\";
    String const FILE_PREFIX_STEREO_LEFT = "StereoL";
    String const FILE_PREFIX_STEREO_RIGHT = "StereoR";
    int const MIN_IMAGE_NUMBER_CALIB_STEREO = 0;
    int const MAX_IMAGE_NUMBER_CALIB_STEREO = 31;
    int const IMAGE_COUNT_STEREO = MAX_IMAGE_NUMBER_CALIB_STEREO - MIN_IMAGE_NUMBER_CALIB_STEREO + 1;

    // Choose which stereo image pair to generate Q off of
    String const STEREO_RECTIFICATION_PAIR_NUMBER = "4";


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
    vector<Point2f> chessboard_corners, chessboard_corners_left, chessboard_corners_right;
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

    // Loop through each calibration image and extract a list of image points
    for (int i = MIN_NUMBER_CALIB_LEFT; i <= MAX_NUMBER_CALIB_LEFT; ++i) {
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
    cout << "Calibrating LEFT camera with " << object_points_vector_left.size() << " valid images out of a total of " << IMAGE_COUNT_LEFT << endl;

    // Calibrate the LEFT camera with the extracted points
    calibrateCamera(object_points_vector_left, image_points_vector_left, frame_display.size(), camera_matrix_left, dist_coeffs_left, rvecs, tvecs);


    //
    //// Right Camera Calibrate
    //


    // Loop through each calibration image and extract a list of image points
    for (int i = MIN_NUMBER_CALIB_RIGHT; i <= MAX_NUMBER_CALIB_RIGHT; ++i) {
        // Load the image
        calib_image_file_right.str("");
        calib_image_file_right.clear();
        calib_image_file_right << FOLDER_CALIB_RIGHT << FILE_PREFIX_CALIB_RIGHT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;
        frame = imread(calib_image_file_right.str(), CV_LOAD_IMAGE_COLOR);

        // Exit if no images were grabbed
        if( frame.empty() ) {
            cout <<  "Could not open or find the image. Did you unzip the calibration images?" << std::endl ;
            // Show the image file path
            cout << "Image File: " << calib_image_file_right.str() << endl;
            return -1;
        }

        //  Get a grayscale copy to work on
        frame.copyTo(frame_display);
        cvtColor(frame_display, frame_display, COLOR_BGR2GRAY);

        // Clear corner values, so that it doesn't grow
        image_points_right.clear();

        //// Process the image
        pattern_was_found = findChessboardCorners(frame_display, PATTERN_SIZE, chessboard_corners, FIND_CHESSBOARD_FLAGS);
        if(!pattern_was_found){
            cout << "Chessboard Corners not found!!" << endl;
            cout << "Image File: " << calib_image_file_right.str() << endl;
            // if chessboard is not found, omit the image
            continue;
        }

        cornerSubPix(frame_display, chessboard_corners, Size(11,11), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1 ));

        // Make sure that we have all the corners we expect...
        CV_Assert(chessboard_corners.size() == CHESSBOARD_CORNERS_COUNT);

        // Extract all the 2-d corner points into image_point_rights
        for (int i = 0; i < chessboard_corners.size(); ++i) {
            image_points_right.push_back(chessboard_corners[i]);
        }
        // cout << image_points_right << endl_right;

        // Save the 2-d and 3-d vectors to a vector of vectors
        object_points_vector_right.push_back(object_points);
        image_points_vector_right.push_back(image_points_right);
    }


    // Initialize a 3x3 matrix that will be overwritten (64F is a double, not a float)
    Mat camera_matrix_right = Mat::zeros(3, 3, CV_64F);
    // Initialize 8x1 coefficient matrix (64F is a double, not a float)
    Mat dist_coeffs_right = Mat::zeros(8, 1, CV_64F);
    // // Throwaway values - we don't care about rvec and tvec until stereo
    // vector<Mat> rvecs, tvecs;

    // There should be equal amounts of elements
    CV_Assert(object_points_vector_right.size() == image_points_vector_right.size());
    cout << "Calibrating RIGHT camera with " << object_points_vector_right.size() << " valid images out of a total of " << IMAGE_COUNT_RIGHT << endl;

    // Calibrate the LEFT camera with the extracted points
    calibrateCamera(object_points_vector_right, image_points_vector_right, frame_display.size(), camera_matrix_right, dist_coeffs_right, rvecs, tvecs);


    //
    //// Stereo Calibrate
    //

    // These are the pixel points of the located corners in the image
    vector<Point2f> image_points_stereo_left;
    vector<Point2f> image_points_stereo_right;
    // Vector of vectors
    vector<vector<Point2f>> image_points_vector_stereo_left;
    vector<vector<Point2f>> image_points_vector_stereo_right;
    vector<vector<Point3f>> object_points_vector_stereo;

    Mat frame_left; // allocate an image buffer object
    Mat frame_display_left; // the frame to work with

    Mat frame_right; // allocate an image buffer object
    Mat frame_display_right; // the frame to work with

    stringstream calib_image_file_stereo_left;
    stringstream calib_image_file_stereo_right;

    // Loop through each pair of stereo images and extract image points
    for (int i = MIN_IMAGE_NUMBER_CALIB_STEREO; i <= MAX_IMAGE_NUMBER_CALIB_STEREO; ++i) {

        // Load the image
        calib_image_file_stereo_left.str("");
        calib_image_file_stereo_left.clear();
        calib_image_file_stereo_right.str("");
        calib_image_file_stereo_right.clear();

        // Calculate the full path to the image file
        calib_image_file_stereo_left << FOLDER_CALIB_STEREO << FILE_PREFIX_STEREO_LEFT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;
        calib_image_file_stereo_right << FOLDER_CALIB_STEREO << FILE_PREFIX_STEREO_RIGHT << setw(2) << setfill('0') << to_string(i) << IMAGE_FILE_SUFFIX;

        // Load the image
        frame_left = imread(calib_image_file_stereo_left.str(), CV_LOAD_IMAGE_COLOR);
        frame_right = imread(calib_image_file_stereo_right.str(), CV_LOAD_IMAGE_COLOR);

        // Exit if no images were grabbed
        if( frame_left.empty() ) {
            cout <<  "Could not open or find the left image. Did you unzip the calibration images?" << std::endl ;
            // Show the image file path
            cout << "Left Image File: " << calib_image_file_stereo_left.str() << endl;
            return -1;
        }

        // Exit if no images were grabbed
        if( frame_right.empty() ) {
            cout <<  "Could not open or find the right image. Did you unzip the calibration images?" << std::endl ;
            // Show the image file path
            cout << "Right Image File: " << calib_image_file_stereo_right.str() << endl;
            return -1;
        }

        // Clear corner values, so that it doesn't grow to more than 40 points
        image_points_stereo_left.clear();
        image_points_stereo_right.clear();

        //  Get a grayscale copy to work on
        frame_left.copyTo(frame_display_left);
        frame_right.copyTo(frame_display_right);
        cvtColor(frame_display_left, frame_display_left, COLOR_BGR2GRAY);
        cvtColor(frame_display_right, frame_display_right, COLOR_BGR2GRAY);

        //// Process the image
        pattern_was_found = findChessboardCorners(frame_display_left, PATTERN_SIZE, chessboard_corners_left, FIND_CHESSBOARD_FLAGS);
        if(!pattern_was_found){
            cout << "Left Chessboard Corners not found!!" << endl;
            cout << "Left Image File: " << calib_image_file_stereo_left.str() << endl;
            // if chessboard is not found, omit the image pair
            continue;
        }

        pattern_was_found = findChessboardCorners(frame_display_right, PATTERN_SIZE, chessboard_corners_right, FIND_CHESSBOARD_FLAGS);
        if(!pattern_was_found){
            cout << "Right Chessboard Corners not found!!" << endl;
            cout << "Right Image File: " << calib_image_file_stereo_right.str() << endl;
            // if chessboard is not found, omit the image pair
            continue;
        }

        cornerSubPix(frame_display_left, chessboard_corners_left, Size(11,11), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1 ));
        cornerSubPix(frame_display_right, chessboard_corners_right, Size(11,11), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1 ));

        // cout << corners << endl;
        // cout << "Corners Size: " << corners.size() << endl;
        // cout << "Corners Calculated Size: " << (CHESSBOARD_ROWS-1)*(CHESSBOARD_COLUMNS-1) << endl;

        // Make sure that we have all the corners we expect...
        CV_Assert(chessboard_corners_left.size() == (CHESSBOARD_ROWS-1)*(CHESSBOARD_COLUMNS-1));
        CV_Assert(chessboard_corners_right.size() == (CHESSBOARD_ROWS-1)*(CHESSBOARD_COLUMNS-1));
        CV_Assert(chessboard_corners_left.size() == chessboard_corners_right.size());

        // Extract all the 2-d corner points into imagePoints
        for (int i = 0; i < chessboard_corners_left.size(); ++i) {
            image_points_stereo_left.push_back(chessboard_corners_left[i]);
            image_points_stereo_right.push_back(chessboard_corners_right[i]);
        }
        // cout << image_points_stereo_left << endl;
        // cout << image_points_stereo_right << endl;

        // Make sure that the # of object points and image points are the same
        CV_Assert(object_points.size() == image_points_stereo_left.size());
        CV_Assert(image_points_stereo_right.size() == image_points_stereo_left.size());

        // Save the 2-d and 3-d vectors to a vector of vectors
        object_points_vector_stereo.push_back(object_points);
        image_points_vector_stereo_left.push_back(image_points_stereo_left);
        image_points_vector_stereo_right.push_back(image_points_stereo_right);
    }



    // Make sure that the # of image point vectors and object point vectors are all the same
    CV_Assert(object_points_vector_stereo.size() == image_points_vector_stereo_left.size());
    CV_Assert(image_points_vector_stereo_left.size() == image_points_vector_stereo_right.size());

    Mat rmat, tvec;
    Mat essential_matrix, fundamental_matrix;

    //
    //// Use left and right camera parameters
    //
    // NOTE: In opencv, the first camera is always the left camera, the second is the right
    // Use CV_CALIB_USE_INTRINSIC_GUESS in order to use the camera parameters generated in task 1 instead of overwriting them
    stereoCalibrate(object_points_vector_stereo, image_points_vector_stereo_left, image_points_vector_stereo_right, camera_matrix_left, dist_coeffs_left, camera_matrix_right, dist_coeffs_right, frame_display_left.size(), rmat, tvec, essential_matrix, fundamental_matrix);


    //
    //// Stereo Rectify
    //



    // Load the image
    calib_image_file_stereo_left.str("");
    calib_image_file_stereo_left.clear();
    calib_image_file_stereo_right.str("");
    calib_image_file_stereo_right.clear();

    // Calculate the full path to the image file
    calib_image_file_stereo_left << FOLDER_CALIB_STEREO << FILE_PREFIX_STEREO_LEFT << setw(2) << setfill('0') << STEREO_RECTIFICATION_PAIR_NUMBER << IMAGE_FILE_SUFFIX;
    calib_image_file_stereo_right << FOLDER_CALIB_STEREO << FILE_PREFIX_STEREO_RIGHT << setw(2) << setfill('0') << STEREO_RECTIFICATION_PAIR_NUMBER << IMAGE_FILE_SUFFIX;

    // Load the image
    frame_left = imread(calib_image_file_stereo_left.str(), CV_LOAD_IMAGE_COLOR);
    frame_right = imread(calib_image_file_stereo_right.str(), CV_LOAD_IMAGE_COLOR);

    // Exit if no images were grabbed
    if( frame_left.empty() ) {
        cout <<  "Could not open or find the left stereo image. Did you unzip the calibration images?" << std::endl ;
        // Show the image file path
        cout << "Left Image File: " << calib_image_file_stereo_left.str() << endl;
        return -1;
    }

    // Exit if no images were grabbed
    if( frame_right.empty() ) {
        cout <<  "Could not open or find the right stereo image. Did you unzip the calibration images?" << std::endl ;
        // Show the image file path
        cout << "Right Image File: " << calib_image_file_stereo_right.str() << endl;
        return -1;
    }

    Mat rmat_left, rmat_right;
    Mat pmat_left, pmat_right;
    Mat Q;

    // 1 is left, 2 is right
    stereoRectify(camera_matrix_left, dist_coeffs_left, camera_matrix_right, dist_coeffs_right, frame_left.size(), rmat, tvec, rmat_left, rmat_right, pmat_left, pmat_right, Q);


    //
    //// Save Parameters to a file
    //

    // Save files as yaml using FileStorage
    // string output_file = "baseball_params.yaml";
    FileStorage baseball_params("baseball_params.yaml", FileStorage::WRITE);
    // Left camera calibration
    baseball_params << "intrinsic_left" << camera_matrix_left;
    baseball_params << "distortion_left" << dist_coeffs_left;
    // Right camera calibration
    baseball_params << "intrinsic_right" << camera_matrix_right;
    baseball_params << "distortion_right" << dist_coeffs_right;
    // Stereo calibration
    baseball_params << "fundamental" << fundamental_matrix;
    baseball_params << "translation" << tvec;
    baseball_params << "rotation" << rmat;
    baseball_params << "essential" << essential_matrix;
    // Stereo rectification
    baseball_params << "rmat_left" << rmat_left;
    baseball_params << "rmat_right" << rmat_right;
    baseball_params << "pmat_left" << pmat_left;
    baseball_params << "pmat_right" << pmat_right;
    baseball_params << "q" << Q;

    baseball_params.release();

    return 0;
}


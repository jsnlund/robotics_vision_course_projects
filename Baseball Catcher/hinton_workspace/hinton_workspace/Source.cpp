// Project 3 Workspace for Michael Hinton
//
// abs diff to locate baseball
// Calculate ROI
// TODO: Calibrate camera?
// TODO: Look at HW 3 task 5 code for real-world points?

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
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
    int keypress = 0;

    int const MIN_IMAGE_NUMBER = 1;
    int const MAX_IMAGE_NUMBER = 59;

    // allocate an image buffer objects
    Mat frame_left;
    Mat frame_left_display;
    Mat frame_left_prev;
    Mat frame_left_diff;
    Mat frame_left_thresh;
    Mat frame_left_first;

    Mat frame_right;
    Mat frame_right_display;
    Mat frame_right_prev;
    Mat frame_right_diff;
    Mat frame_right_thresh;
    Mat frame_right_first;

    // ROI Initial Offset Constants
    int const ROI_LEFT_X_DEFAULT = 320;
    int const ROI_LEFT_Y_DEFAULT = 60;
    int const ROI_LEFT_WIDTH_DEFAULT = 100;
    int const ROI_LEFT_HEIGHT_DEFAULT = 100;

    int const ROI_RIGHT_X_DEFAULT = 235;
    int const ROI_RIGHT_Y_DEFAULT = 55;
    int const ROI_RIGHT_WIDTH_DEFAULT = ROI_LEFT_WIDTH_DEFAULT;
    int const ROI_RIGHT_HEIGHT_DEFAULT = ROI_LEFT_HEIGHT_DEFAULT;

    // Dynamic ROI variables
    int roi_left_x = ROI_LEFT_X_DEFAULT;
    int roi_left_y = ROI_LEFT_Y_DEFAULT;
    int roi_left_width = ROI_LEFT_WIDTH_DEFAULT;
    int roi_left_height = ROI_LEFT_HEIGHT_DEFAULT;

    int roi_right_x = ROI_RIGHT_X_DEFAULT;
    int roi_right_y = ROI_RIGHT_Y_DEFAULT;
    int roi_right_width = ROI_RIGHT_WIDTH_DEFAULT;
    int roi_right_height = ROI_RIGHT_HEIGHT_DEFAULT;

    // Create Default ROI
    Rect LEFT_ROI_DEFAULT = Rect(ROI_LEFT_X_DEFAULT,ROI_LEFT_Y_DEFAULT,ROI_LEFT_WIDTH_DEFAULT,ROI_LEFT_HEIGHT_DEFAULT);
    Rect RIGHT_ROI_DEFAULT = Rect(ROI_RIGHT_X_DEFAULT,ROI_RIGHT_Y_DEFAULT,ROI_RIGHT_WIDTH_DEFAULT,ROI_RIGHT_HEIGHT_DEFAULT);

    Rect left_roi = LEFT_ROI_DEFAULT;
    Rect right_roi = RIGHT_ROI_DEFAULT;

    // When activity is generated in the initial ROI, trigger ball_in_flight bool
    // Or maybe when width of ball is too great - i.e. when too close
    bool ball_in_flight = false;

    stringstream current_image_file_left;
    stringstream current_image_file_right;

    // A vector to hold an array of corner coordinates
    vector<Point2f> corners_left, corners_right;

    // TODO: Erosion kernel - create one, or use default via Mat()?
    // Mat erosionKernel = getStructuringElement(int shape, Size ksize, Point anchor=Point(-1,-1))

    // Load the first image
    int i = MIN_IMAGE_NUMBER;
    current_image_file_left << FOLDER << IMAGE_PREFIX_LEFT << setw(2) << setfill('0') << to_string(MIN_IMAGE_NUMBER) << IMAGE_FILE_SUFFIX;
    current_image_file_right << FOLDER << IMAGE_PREFIX_RIGHT << setw(2) << setfill('0') << to_string(MIN_IMAGE_NUMBER) << IMAGE_FILE_SUFFIX;
    frame_left_first = imread(current_image_file_left.str(), CV_LOAD_IMAGE_GRAYSCALE);
    frame_right_first = imread(current_image_file_right.str(), CV_LOAD_IMAGE_GRAYSCALE);

    frame_left_first.copyTo(frame_left_prev);
    frame_right_first.copyTo(frame_right_prev);

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
        frame_left = imread(current_image_file_left.str(), CV_LOAD_IMAGE_GRAYSCALE);
        frame_right = imread(current_image_file_right.str(), CV_LOAD_IMAGE_GRAYSCALE);

        // Exit if no images were grabbed
        if( frame_left.empty() ) {
            cout <<  "Could not open or find the left image" << std::endl;
            // Show the image file path
            cout << "Left Image File: " << current_image_file_left.str() << endl;
            return -1;
        }

        // Exit if no images were grabbed
        if( frame_right.empty() ) {
            cout <<  "Could not open or find the right image" << std::endl;
            // Show the image file path
            cout << "Right Image File: " << current_image_file_left.str() << endl;
            return -1;
        }

        //
        //// Process the image
        //

        // Absolute difference
        absdiff(frame_left, frame_left_prev, frame_left_diff);
        absdiff(frame_right, frame_right_prev, frame_right_diff);

        // Try out erode
        #define ITERATIONS 1
        erode(frame_left_diff, frame_left_diff, Mat(), Point(-1,-1), ITERATIONS);
        // erode(frame_right_diff, frame_right_diff, Mat());
        // http://docs.opencv.org/3.0-beta/doc/py_tutorials/py_imgproc/py_morphological_ops/py_morphological_ops.html


        // Try other blurring factors and sizes
        // Sizes must be an odd number
        // Try Size(31,31), 15, 15 to get rid of curtain movement
        GaussianBlur(frame_left_diff, frame_left_diff, Size(7,7), 3.0, 3.0);
        // GaussianBlur(frame_right_diff, frame_right_thresh, Size(7,7), 3.0, 3.0);
        // GaussianBlur(frame_left_diff, frame_left_diff, Size(31,31), 15.0, 15.0);
        GaussianBlur(frame_right_diff, frame_right_thresh, Size(31,31), 15.0, 15.0);


        threshold(frame_left_diff, frame_left_thresh, 5, 256, THRESH_BINARY);
        threshold(frame_right_thresh, frame_right_thresh, 5, 256, THRESH_BINARY);
        // adaptiveThreshold(frame_right_diff, frame_right_thresh, 5, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, 0.0);


        // corner detection
        corners_left.clear();
        corners_right.clear();
        goodFeaturesToTrack(frame_left_thresh(left_roi), corners_left, 10, 0.01, 10);
        goodFeaturesToTrack(frame_right_thresh(right_roi), corners_right, 10, 0.01, 10);
        // cornerSubPix(frame_left_thresh, corners_left, Size(5,5), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 ) );
        // cornerSubPix(frame_right_thresh, corners_right, Size(5,5), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 ) );

        cvtColor(frame_left_thresh, frame_left_thresh, COLOR_GRAY2BGR);
        cvtColor(frame_right_thresh, frame_right_thresh, COLOR_GRAY2BGR);
        // Draw detected corners
        for (int i = 0; i < corners_left.size(); ++i){
            circle(frame_left_thresh, Point(ROI_LEFT_X_DEFAULT+corners_left[i].x,ROI_LEFT_Y_DEFAULT+corners_left[i].y), 20, Scalar(200,80,0), 1);
        }
        for (int i = 0; i < corners_right.size(); ++i){
            circle(frame_right_thresh, Point(ROI_RIGHT_X_DEFAULT+corners_right[i].x,ROI_RIGHT_Y_DEFAULT+corners_right[i].y), 20, Scalar(100,180,80), 1);
        }


        // Let the corners trigger the ball in flight bool once
        if(ball_in_flight || (corners_left.size() > 0 && corners_left.size() > 0)){
            cout << "L corners:" << corners_left.size() << endl;
            cout << "R corners:" << corners_right.size() << endl << endl;
            ball_in_flight = true;
        }



        cvtColor(frame_left, frame_left_display, COLOR_GRAY2BGR);
        cvtColor(frame_right, frame_right_display, COLOR_GRAY2BGR);

        if(ball_in_flight) {
            // Draw RoI as a rectangle
            rectangle(frame_left_thresh, left_roi, Scalar(200,80,0));
            rectangle(frame_right_thresh, right_roi, Scalar(100,180,80));
            rectangle(frame_left_display, left_roi, Scalar(200,80,0));
            rectangle(frame_right_display, right_roi, Scalar(100,180,80));
        }

        // Show the image output for a sanity check
        imshow("Left", frame_left_display);
        imshow("Right", frame_right_display);

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
            // TODO: When ball gets too close, set ball_in_flight to false so ROI can be reset to the default
            // Reset
            left_roi = LEFT_ROI_DEFAULT;
            right_roi = RIGHT_ROI_DEFAULT;
            ball_in_flight = false;
        }
    }

    return 0;
}
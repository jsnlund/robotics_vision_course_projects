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

    int const IMAGE_WIDTH = 640;
    int const IMAGE_HEIGHT = 480;

    // Keyboard codes
    int const ESC_KEY = 27;
    int keypress = 0;

    int const MIN_IMAGE_NUMBER = 1;
    int const MAX_IMAGE_NUMBER = 59;

    // allocate an image buffer objects
    Mat frame_left; // The current frame
    Mat frame_left_display; // The current frame with color and drawings
    Mat frame_left_prev;  // The previous frame
    Mat frame_left_diff; // Diff between current and prev
    Mat frame_left_thresh; // Threshold between diff
    Mat frame_left_canny;
    Mat frame_left_first; // The image before the first detected ball movement
    Mat frame_left_first_diff; // Diff between current and first
    Mat frame_left_first_diff_thresh;
    Mat frame_left_and;
    Mat frame_left_nxor;

    Mat frame_right; // The current frame
    Mat frame_right_display; // The current frame with color and drawings
    Mat frame_right_prev; // The previous frame
    Mat frame_right_diff; // Diff between current and prev
    Mat frame_right_thresh; // Threshold between diff
    Mat frame_right_canny;
    Mat frame_right_first; // The image before the first detected ball movement
    Mat frame_right_first_diff; // Diff between current and first
    Mat frame_right_first_diff_thresh;
    Mat frame_right_and;
    Mat frame_right_nxor;

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
    Rect ROI_DEFAULT_LEFT = Rect(ROI_DEFAULT_X_LEFT,ROI_DEFAULT_Y_LEFT,ROI_DEFAULT_WIDTH_LEFT,ROI_DEFAULT_HEIGHT_LEFT);
    Rect ROI_DEFAULT_RIGHT = Rect(ROI_DEFAULT_X_RIGHT,ROI_DEFAULT_Y_RIGHT,ROI_DEFAULT_WIDTH_RIGHT,ROI_DEFAULT_HEIGHT_RIGHT);

    // Dynamic ROIs
    // Fields: x, y, width, height
    Rect roi_left = ROI_DEFAULT_LEFT;
    Rect roi_right = ROI_DEFAULT_RIGHT;

    // When activity is generated in the initial ROI, trigger ball_in_flight bool
    // Or maybe when width of ball is too great - i.e. when too close
    bool ball_in_flight = false;

    stringstream current_image_file_left;
    stringstream current_image_file_right;

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



    //
    //// Load in parameters from file
    //// Read in intrinsic and distortion parameters from task 1
    //

    // // Initialize a 3x3 intrinsic matrix
    // Mat camera_matrix_left, camera_matrix_right;
    // // Initialize 5x1 coefficient matrix
    // Mat dist_coeffs_left, dist_coeffs_right;
    // // Read yaml files using FileStorage
    // FileStorage fs_left("baseball_left_parameters.yaml", FileStorage::READ);
    // FileStorage fs_right("baseball_right_parameters.yaml", FileStorage::READ);
    // fs_left["distortion"] >> dist_coeffs_left;
    // fs_left["intrinsic"] >> camera_matrix_left;
    // fs_right["distortion"] >> dist_coeffs_right;
    // fs_right["intrinsic"] >> camera_matrix_right;

    // // Read in the fundamental matrix from task 2
    // Mat fundamental_matrix, essential_matrix;
    // Mat rmat, tvec;

    // FileStorage stereo_params("baseball_stereo_parameters.yaml", FileStorage::READ);
    // stereo_params["fundamental"] >> fundamental_matrix;
    // stereo_params["rotation"] >> rmat;
    // stereo_params["translation"] >> tvec;
    // // stereo_params["essential"] >> essential_matrix;

    // // Read in the stereo rectify parameters R and P for left and right
    // Mat rmat_left, rmat_right, pmat_left, pmat_right, Q;
    // FileStorage stereo_rectify_params("baseball_stereo_rectify_params.yaml", FileStorage::READ);
    // stereo_rectify_params["rmat_left"] >> rmat_left;
    // stereo_rectify_params["rmat_right"] >> rmat_right;
    // stereo_rectify_params["pmat_left"] >> pmat_left;
    // stereo_rectify_params["pmat_right"] >> pmat_right;
    // stereo_rectify_params["Q"] >> Q;









    // Load the first image
    int i = MIN_IMAGE_NUMBER;
    current_image_file_left << FOLDER << IMAGE_PREFIX_LEFT << setw(2) << setfill('0') << to_string(MIN_IMAGE_NUMBER) << IMAGE_FILE_SUFFIX;
    current_image_file_right << FOLDER << IMAGE_PREFIX_RIGHT << setw(2) << setfill('0') << to_string(MIN_IMAGE_NUMBER) << IMAGE_FILE_SUFFIX;
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
        // GaussianBlur(frame_right_diff, frame_right_diff, Size(7,7), 3.0, 3.0);
        // GaussianBlur(frame_left_diff, frame_left_diff, Size(31,31), 15.0, 15.0);
        GaussianBlur(frame_right_diff, frame_right_diff, Size(31,31), 15.0, 15.0);


        threshold(frame_left_diff, frame_left_thresh, 5, 256, THRESH_BINARY);
        threshold(frame_right_diff, frame_right_thresh, 5, 256, THRESH_BINARY);
        // adaptiveThreshold(frame_right_diff, frame_right_thresh, 5, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 7, 0.0);


        // corner detection
        corners_left.clear();
        corners_right.clear();
        goodFeaturesToTrack(frame_left_thresh(roi_left), corners_left, 10, 0.01, 10);
        goodFeaturesToTrack(frame_right_thresh(roi_right), corners_right, 10, 0.01, 10);
        // cornerSubPix(frame_left_thresh, corners_left, Size(5,5), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 ) );
        // cornerSubPix(frame_right_thresh, corners_right, Size(5,5), Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001 ) );

        // Let the corners trigger the ball in flight bool once
        if(ball_in_flight == false && (corners_left.size() > 0 && corners_left.size() > 0)){
            // cout << "L corners:" << corners_left.size() << endl;
            // cout << "R corners:" << corners_right.size() << endl << endl;
            // Because we found corners, set the previous image as the first image
            frame_left_prev.copyTo(frame_left_first);
            frame_right_prev.copyTo(frame_right_first);
            ball_in_flight = true;
        }

        cvtColor(frame_left, frame_left_display, COLOR_GRAY2BGR);
        cvtColor(frame_right, frame_right_display, COLOR_GRAY2BGR);

        if(ball_in_flight) {
            absdiff(frame_left, frame_left_first, frame_left_first_diff);
            absdiff(frame_right, frame_right_first, frame_right_first_diff);

            // Show the abs difference between first and curr frames
            imshow("Left Diff", frame_left_first_diff);
            imshow("Right Diff", frame_right_first_diff);

            // GaussianBlur(frame_left_first_diff, frame_left_first_diff_thresh, Size(7,7), 3.0, 3.0);
            // GaussianBlur(frame_right_diff, frame_right_thresh, Size(7,7), 3.0, 3.0);
            GaussianBlur(frame_left_first_diff, frame_left_first_diff_thresh, Size(31,31), 15.0, 15.0);
            GaussianBlur(frame_right_first_diff, frame_right_first_diff_thresh, Size(31,31), 15.0, 15.0);

            threshold(frame_left_first_diff_thresh, frame_left_first_diff_thresh, 5, 256, THRESH_BINARY);
            threshold(frame_right_first_diff_thresh, frame_right_first_diff_thresh, 5, 256, THRESH_BINARY);

            // // Canny
            // Canny(frame_left_first_diff_thresh, frame_left_canny, 10, 200, 3);
            // imshow("Left Canny", frame_left_canny);

            // Find Contours
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
            findContours(frame_left_first_diff_thresh.clone()(roi_left), contours_left, hierarchy_left, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_left.x, roi_left.y));
            findContours(frame_right_first_diff_thresh.clone()(roi_right), contours_right, hierarchy_right, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(roi_right.x, roi_right.y));
            // cout << "Left Contours: " << contours_left.size() << endl;

            cout << "contours_left count: " << contours_left.size() << endl;
            cout << "contours_right count: " << contours_right.size() << endl;

            // Find the centroid of the ball from the moments
            // See https://en.wikipedia.org/wiki/Image_moment#Central_moments
            // See also http://docs.opencv.org/3.1.0/dd/d49/tutorial_py_contour_features.html#gsc.tab=0
            // center x = m10 / m00; center y = m01 / m00
            if(contours_left.size() > 0){
                // cout << "contours_left[0]: " << contours_left[0] << endl;
                ball_contour_left = contours_left[0];
                ball_m_left = moments(ball_contour_left);
                ball_centroid_left = Point2i(int(ball_m_left.m10 / ball_m_left.m00), int(ball_m_left.m01 / ball_m_left.m00));
                cout << "left centroid: " << ball_centroid_left << endl;
            }
            if(contours_right.size() > 0) {
                // cout << "contours_right[0]: " << contours_right[0] << endl;
                ball_contour_right = contours_right[0];
                ball_m_right = moments(ball_contour_right);
                ball_centroid_right = Point2i(int(ball_m_right.m10 / ball_m_right.m00), int(ball_m_right.m01 / ball_m_right.m00));
                cout << "right centroid: " << ball_centroid_right << endl;
            }

            // bitwise_and(frame_left_first_diff_thresh, frame_left_thresh, frame_left_and);
            // imshow("Left AND", frame_left_and);

            //
            //// Draw
            //

            cvtColor(frame_left_first_diff_thresh, frame_left_first_diff_thresh, COLOR_GRAY2BGR);
            cvtColor(frame_right_first_diff_thresh, frame_right_first_diff_thresh, COLOR_GRAY2BGR);
            if(contours_left.size() > 0 && contours_right.size() > 0){
                // cout << "drawContours!" << endl;
                drawContours(frame_left_first_diff_thresh, contours_left, -1, Scalar(0,0,255), 3, 8, hierarchy_left, 2, Point() );
                // Draw the centroid of the ball
                circle(frame_left_first_diff_thresh, ball_centroid_left, 5, Scalar(200,80,0), 1);
                // TODO: recalculate the roi
                int x_margin_left = roi_left.width/2;
                int y_margin_left = roi_left.height/2;
                // Ball is moving to the left
                while(ball_centroid_left.x - roi_left.x < x_margin_left && roi_left.x > 0){
                    roi_left.x--;
                }
                // Ball is moving to the right
                while(ball_centroid_left.x - roi_left.x > x_margin_left && roi_left.x < IMAGE_WIDTH-1){
                    roi_left.x++;
                }
                // Ball is moving up
                while(ball_centroid_left.y - roi_left.y < y_margin_left && roi_left.y > 0){
                    roi_left.y--;
                }
                // Ball is moving down
                while(ball_centroid_left.y - roi_left.y > y_margin_left && roi_left.y < IMAGE_HEIGHT-1){
                    roi_left.y++;
                }

                // cout << "drawContours!" << endl;
                drawContours(frame_right_first_diff_thresh, contours_right, -1, Scalar(100,180,80), 3, 8, hierarchy_right, 2, Point() );
                // Draw the centroid of the ball
                circle(frame_right_first_diff_thresh, ball_centroid_right, 5, Scalar(200,80,0), 1);
                // TODO: recalculate the roi
                int x_margin_right = roi_right.width/2;
                int y_margin_right = roi_right.height/2;
                // Ball is moving to the left
                while(ball_centroid_right.x - roi_right.x < x_margin_right && roi_right.x > 0){
                    roi_right.x--;
                }
                // Ball is moving to the right
                while(ball_centroid_right.x - roi_right.x > x_margin_right && roi_right.x < IMAGE_WIDTH-1){
                    roi_right.x++;
                }
                // Ball is moving up
                while(ball_centroid_right.y - roi_right.y < y_margin_right && roi_right.y > 0){
                    roi_right.y--;
                }
                // Ball is moving down
                while(ball_centroid_right.y - roi_right.y > y_margin_right && roi_right.y < IMAGE_HEIGHT-1){
                    roi_right.y++;
                }

            }

            imshow("Left Thresh", frame_left_first_diff_thresh);
            imshow("Right Thresh", frame_right_first_diff_thresh);

            // Draw RoI as a rectangle, only when ball is in motion
            rectangle(frame_left_thresh, roi_left, Scalar(0,0,255));
            rectangle(frame_right_thresh, roi_right, Scalar(100,180,80));
            rectangle(frame_left_display, roi_left, Scalar(0,0,255));
            rectangle(frame_right_display, roi_right, Scalar(100,180,80));


            // Print the coordinates of the centroid of the ball
            stringstream coordinates_left;
            coordinates_left << "(" << to_string((int)ball_centroid_left.x) << ", " << to_string((int)ball_centroid_left.y) << /*", " << to_string((int)ball_centroid_left.z) <<*/ ")";
            stringstream coordinates_right;
            coordinates_right << "(" << to_string((int)ball_centroid_right.x) << ", " << to_string((int)ball_centroid_right.y) << /*", " << to_string((int)ball_centroid_right.z) <<*/ ")";
            putText(frame_left_display, coordinates_left.str(), Point2f(10, 50), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0,165,255), 2);
            putText(frame_right_display, coordinates_right.str(), Point2f(10, 50), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0,200,0), 2);
        }

        //
        //// Draw
        //

        cvtColor(frame_left_thresh, frame_left_thresh, COLOR_GRAY2BGR);
        cvtColor(frame_right_thresh, frame_right_thresh, COLOR_GRAY2BGR);
        // Draw detected corners
        for (int i = 0; i < corners_left.size(); ++i){
            circle(frame_left_thresh, Point(ROI_DEFAULT_LEFT.x+corners_left[i].x,ROI_DEFAULT_LEFT.y+corners_left[i].y), 20, Scalar(200,80,0), 1);
        }
        for (int i = 0; i < corners_right.size(); ++i){
            circle(frame_right_thresh, Point(ROI_DEFAULT_RIGHT.x+corners_right[i].x,ROI_DEFAULT_RIGHT.y+corners_right[i].y), 20, Scalar(100,180,80), 1);
        }







        // Show the image output for a sanity check
        imshow("Left", frame_left_display);
        imshow("Right", frame_right_display);

        // imshow("Left Diff", frame_left_diff);
        // imshow("Right Diff", frame_right_diff);

        // imshow("Left Threshold", frame_left_thresh);
        // imshow("Right Threshold", frame_right_thresh);


        // Need this for images to display, or else output windows just show up gray
        keypress = waitKey(30);

        // Update prev frames with current frames
        frame_left.copyTo(frame_left_prev);
        frame_right.copyTo(frame_right_prev);

        i++;
        if(i > MAX_IMAGE_NUMBER) {
            i = MIN_IMAGE_NUMBER;
            // TODO: When ball gets too close, set ball_in_flight to false so ROI can be reset to the default
            // Reset roi
            roi_left = ROI_DEFAULT_LEFT;
            roi_right = ROI_DEFAULT_RIGHT;
            ball_in_flight = false;
        }
    }

    return 0;
}
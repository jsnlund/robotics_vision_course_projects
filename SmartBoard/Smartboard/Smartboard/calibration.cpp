#include <opencv2/opencv.hpp>

#include "calibration.h"
#include "globals.h"

using namespace std;
using namespace cv;


// Initialize constants set in the header file
const int HUE_CALIB = 55;
const int HUE_BUFFER_CALIB = 15;
const int MIN_SAT_CALIB = (int) 0.3 * 255;
const int MAX_SAT_CALIB = (int) 1.0 * 255;
const int MIN_VAL_CALIB = (int) 0.1 * 255;
const int MAX_VAL_CALIB = (int) 1.0 * 255;

// const Size CALIBRATION_FRAME_SIZE = Size(IMAGE_WIDTH,IMAGE_HEIGHT);
const Size CALIBRATION_FRAME_SIZE = Size(IMAGE_WIDTH*1.8,IMAGE_HEIGHT*1.8);



// if calculate_transform is false, a green rectangle is produced on the projector frame and the code tries to find it in the camera frame (returns an empty Mat)
// if calculate_transform is true, it takes the currently detected calibration rectangle and calculates the perspective transform Mat from that

Mat calibrate_smartboard(Mat *frame_camera, Mat *frame_projector, bool calculate_transform) {
    // Save off a pristine version of the input camera image
    Mat frame_camera_original;
    (*frame_camera).copyTo(frame_camera_original);

    // Create the calibration image
    Mat frame_calibration = Mat(CALIBRATION_FRAME_SIZE, CV_8UC3);

    // New calibration image
    frame_calibration.setTo(GREEN);

    // Copy the calibration rectangle to the projector frame
    frame_calibration.copyTo(*frame_projector);

    // This will hold what we think is the calibration green square contour
    vector<Point> current_calibration_cotour;
    Point current_calibration_cotour_centroid;

    // Try out HSV
    Mat frame_camera_hsv, frame_camera_hsv_binary;
    cvtColor(*frame_camera, frame_camera_hsv, COLOR_BGR2HSV);

    // NOTE: To adjust HSV values in header file
    // InRange will create a threshold-like binary image
    // See opencv/sources/samples/cpp/camshiftdemo.cpp
    inRange(frame_camera_hsv, Scalar(HUE_CALIB - HUE_BUFFER_CALIB, MIN_SAT_CALIB, MIN_VAL_CALIB), Scalar(HUE_CALIB + HUE_BUFFER_CALIB, MAX_SAT_CALIB, MAX_VAL_CALIB), frame_camera_hsv_binary);

    // imshow("Camera 2 binary", frame_camera_hsv_binary);
    // Do erode and dilate
    int const ITERATIONS = 5;
    erode(frame_camera_hsv_binary, frame_camera_hsv_binary, Mat(), Point(-1,-1), ITERATIONS);
    dilate(frame_camera_hsv_binary, frame_camera_hsv_binary, Mat(), Point(-1,-1), ITERATIONS);
    // imshow("Camera 2 binary eroded", frame_camera_hsv_binary);


    // Find contours
    Mat frame_contours;
    cvtColor(frame_camera_hsv_binary, frame_contours, COLOR_GRAY2BGR);

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    // Use CV_RETR_EXTERNAL instead of TREE, so we only grab outer-most contours
    // Use CV_CHAIN_APPROX_SIMPLE to store less points
    findContours(frame_camera_hsv_binary.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    // cout << "contours count: " << contours.size() << endl;
    float const MIN_CONTOUR_AREA_THRESHOLD = 3000;

    if(contours.size() > 0){
        for (int i = 0; i < contours.size(); ++i){
            vector<Point> contour = contours[i];
            // Find the centroid  from the moments
            // center x = m10 / m00; center y = m01 / m00
            Moments ms = moments(contour);

            double contour_area = contourArea(contours[i], false);

            if(contour_area > MIN_CONTOUR_AREA_THRESHOLD){
                // cout << "contour: " << contours[i] << endl;
                // cout << "contour_area: " << contour_area <<  endl;
                Point2d centroid = Point2d(ms.m10 / ms.m00, ms.m01 / ms.m00);
                // cout << "centroid: " << centroid << endl;
                circle(frame_contours, centroid, 5, GREEN, 3);
                vector<vector<Point>> temp_contours;
                temp_contours.push_back(contour);
                // Draw the contour around the calibration rectangle
                drawContours(frame_contours, temp_contours, -1, BLUE, 3, 8);

                // Save off the calibration rectangle contour and centroid to globals
                current_calibration_cotour = contour;
                current_calibration_cotour_centroid = centroid;
                break;
            }
        }
    }

    // Set the camera frame output
    frame_contours.copyTo(*frame_camera);

    if(calculate_transform){
        // The user indicated that they want to use the current contour as the calibration image.
        // So calculate the perspective transform!

        // Get 4 points for perspective transform (the four corners of the image)
        vector<Point2f> edges_calibration_rectangle, edges_projector;

        // Map those 4 corners to the edges of a blank frame
        edges_projector.push_back(Point2f(0, 0)); // Upper left
        edges_projector.push_back(Point2f(IMAGE_WIDTH-1, 0)); // Upper right
        edges_projector.push_back(Point2f(0, IMAGE_HEIGHT-1)); // Lower left
        edges_projector.push_back(Point2f(IMAGE_WIDTH-1, IMAGE_HEIGHT-1)); // Lower right

        // cout << "current_calibration_cotour: " << current_calibration_cotour << endl;
        // cout << "current_calibration_cotour_centroid: " << current_calibration_cotour_centroid << endl;

        // Loop through all the points of the current contour and find the four corners

        // Create 4 Point placeholder values for these
        // Initialize to -1, -1 so I know it is uninitialized
        Point upper_left = Point(-1,-1);
        Point upper_right = Point(-1,-1);
        Point lower_left = Point(-1,-1);
        Point lower_right = Point(-1,-1);

        double centroid_x = current_calibration_cotour_centroid.x;
        double centroid_y = current_calibration_cotour_centroid.y;

        for (int i = 0; i < current_calibration_cotour.size(); ++i) {
            Point temp_point = current_calibration_cotour[i];
            // cout << "temp_point: " << temp_point << endl;

            // This method of detecting rectangle corners assumes that the corners are not directly vertical or horizontal to the centroid
            // So this won't work on a diamond shape

            double x = temp_point.x;
            double y = temp_point.y;
            // Calculate the absolute distance from the centroid
            double distance = sqrt(pow(centroid_x-x, 2) + pow(centroid_y-y, 2));
            // cout << "distance: " << distance << endl;

            // Determine which region the point is in based off the centroid:
            // Upper left, upper right, lower left, or lower right
            if(x < centroid_x && y <= centroid_y){
                // Now that we know what region we are in, calculate the distance of the old candidate point
                // Initialize
                if(upper_left.x == -1) {
                    // cout << "initializing " << temp_point << " to upper_left!" << endl;
                    upper_left = temp_point;
                    continue;
                }
                double distance_old = sqrt(pow(centroid_x-upper_left.x, 2) + pow(centroid_y-upper_left.y, 2));
                // Determine which distance is greater
                if(distance > distance_old){
                    // Set the new upper left point!
                    // cout << "setting " << temp_point << " to upper_left!" << endl;
                    upper_left = temp_point;
                }
            }
            if(x >= centroid_x && y < centroid_y){
                // Now that we know what region we are in, calculate the distance of the old candidate point
                // Initialize
                if(upper_right.x == -1) {
                    // cout << "initializing " << temp_point << " to upper_right!" << endl;
                    upper_right = temp_point;
                    continue;
                }
                double distance_old = sqrt(pow(centroid_x-upper_right.x, 2) + pow(centroid_y-upper_right.y, 2));
                if(distance > distance_old){
                    // Set the new upper left point!
                    // cout << "setting " << temp_point << " to upper_right!" << endl;
                    upper_right = temp_point;
                }
            }
            if(x <= centroid_x && y > centroid_y){
                // Now that we know what region we are in, calculate the distance of the old candidate point
                // Initialize
                if(lower_left.x == -1) {
                    // cout << "initializing " << temp_point << " to lower_left!" << endl;
                    lower_left = temp_point;
                    continue;
                }
                double distance_old = sqrt(pow(centroid_x-lower_left.x, 2) + pow(centroid_y-lower_left.y, 2));
                if(distance > distance_old){
                    // Set the new upper left point!
                    // cout << "setting " << temp_point << " to lower_left!" << endl;
                    lower_left = temp_point;
                }
            }
            if(x > centroid_x && y >= centroid_y){
                // Now that we know what region we are in, calculate the distance of the old candidate point
                // Initialize
                if(lower_right.x == -1) {
                    // cout << "initializing " << temp_point << " to lower_right!" << endl;
                    lower_right = temp_point;
                    continue;
                }
                double distance_old = sqrt(pow(centroid_x-lower_right.x, 2) + pow(centroid_y-lower_right.y, 2));
                if(distance > distance_old){
                    // Set the new upper left point!
                    // cout << "setting " << temp_point << " to lower_right!" << endl;
                    lower_right = temp_point;
                }
            }

            // // A corner is defined as:
            // //      *the smallest x and y combo (upper left corner)
            // //      *the largest x and smallest y combo (upper right corner)
            // //      *the smallest x and largest y combo (lower left corner)
            // //      *the largest x and y combo (lower right corner)

        }

        // If these fail, it means the points were never initialized properly
        CV_Assert(upper_left.x != -1);
        CV_Assert(upper_right.x != -1);
        CV_Assert(lower_left.x != -1);
        CV_Assert(lower_right.x != -1);

        // cout << "upper_left: " << upper_left << endl;
        // cout << "upper_right: " << upper_right << endl;
        // cout << "lower_left: " << lower_left << endl;
        // cout << "lower_right: " << lower_right << endl;

        // Save off all 4 corners of the calibration rectangle, use them for creating the perspective transform
        // Make sure the order matches the order of edges_projector
        edges_calibration_rectangle.push_back(upper_left);
        edges_calibration_rectangle.push_back(upper_right);
        edges_calibration_rectangle.push_back(lower_left);
        edges_calibration_rectangle.push_back(lower_right);

        // Calculate the perspective transform
        Mat transform_matrix = getPerspectiveTransform(edges_calibration_rectangle, edges_projector);

        // Test the perspective transform on the calibration image
        Mat frame_perspective_test;
        warpPerspective(frame_camera_original, frame_perspective_test, transform_matrix, Size(IMAGE_WIDTH, IMAGE_HEIGHT));

        imshow("Perspective Transform", frame_perspective_test);

        // use perspectiveTransform() to transform only select points instead of an entire image
        // perspectiveTransform(random_points, random_points_transformed, transform_matrix);

        return transform_matrix;
    }
    else {
        // Return an empty matrix as the perspective transform
        return Mat();
    }



}
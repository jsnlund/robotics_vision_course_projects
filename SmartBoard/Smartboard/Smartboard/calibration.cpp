#include <opencv2/opencv.hpp>
#include <iomanip>

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

// Make the calibration image bigger, so that is will sho up
// TODO: If this is bigger, it makes it so that other normal-sized images can't operate on it
const Size CALIBRATION_FRAME_SIZE = Size(IMAGE_WIDTH,IMAGE_HEIGHT);
// const Size CALIBRATION_FRAME_SIZE = Size(IMAGE_WIDTH*1.8,IMAGE_HEIGHT*1.8);



int const CHESSBOARD_COLUMNS = 16;
int const CHESSBOARD_ROWS = 12;


int const CHESSBOARD_CORNERS_COLS = CHESSBOARD_COLUMNS - 1;
int const CHESSBOARD_CORNERS_ROWS = CHESSBOARD_ROWS - 1;




// if calculate_transform is false, a green rectangle is produced on the projector frame and the code tries to find it in the camera frame (returns an empty Mat)
// if calculate_transform is true, it takes the currently detected calibration rectangle and calculates the perspective transform Mat from that


// Create a chessboard image based on the image width and height
// Creates a 16x12 chessboard with white border
void generate_chessboard(Mat *frame_projector){
    (*frame_projector).setTo(WHITE);

    int rows = CHESSBOARD_ROWS+2;
    int cols = CHESSBOARD_COLUMNS+2;

    int sq = IMAGE_WIDTH/cols;
    int sq2 = sq - 1;
    int s;
    // int border = sq2

    for (int i = 1; i < rows-1; ++i){
        if(i % 2 == 0){
            s = 1;
        }
        else{
            s = 2;
        }
        for (int j = 1; j < cols-1; ++j){
            if((j-s) % 2 == 0){
                int x = j*sq;
                int y = i*sq;
                rectangle((*frame_projector), Point(x,y), Point(x+sq2,y+sq2), BLACK, CV_FILLED);//, int lineType=8, int shift=0)
            }
        }
    }
}



Mat chessboard_calibration(Mat *frame_camera, Mat *frame_projector, bool calculate_transform) {
    // Save off a pristine version of the input camera image
    Mat frame_camera_original;
    (*frame_camera).copyTo(frame_camera_original);

    // Paint a chessboard to the projector
    generate_chessboard(frame_projector);

    Size const PATTERN_SIZE =  Size(CHESSBOARD_COLUMNS-1,CHESSBOARD_ROWS-1);
    int const FIND_CHESSBOARD_FLAGS = CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE;

    // Figure out how many corners are in the chessboard
    // int const CHESSBOARD_CORNERS_COUNT = (CHESSBOARD_ROWS-1)*(CHESSBOARD_COLUMNS-1);

    vector<Point2f> corners;
    bool pattern_was_found = false;
    pattern_was_found = findChessboardCorners(*frame_camera, PATTERN_SIZE, corners, FIND_CHESSBOARD_FLAGS);

    if(pattern_was_found){
        cout << "chessboard found!" << endl;

        // Iterate through the corners to extrapolate the outer-most edges of the chessboard
        Point2f upper_left, upper_right, lower_left, lower_right;
        Point2f upper_left_inner, upper_right_inner, lower_left_inner, lower_right_inner;

        // the chessboard corners are the inner corners. We need the outer border of the chessboard. This means outer layer of chessboard squares plus the square-wide white border

        // Extrapolate

        // outer - inner = diff

        // Grab the outer and inner corners of the chessboard
        upper_left = corners[0];
        upper_left_inner = corners[CHESSBOARD_CORNERS_COLS+1];

        upper_right = corners[CHESSBOARD_CORNERS_COLS-1];
        upper_right_inner = corners[2*CHESSBOARD_CORNERS_COLS-2];

        lower_left = corners[(CHESSBOARD_CORNERS_ROWS-1)*CHESSBOARD_CORNERS_COLS];
        lower_left_inner = corners[(CHESSBOARD_CORNERS_ROWS-2)*CHESSBOARD_CORNERS_COLS+1];

        lower_right = corners[CHESSBOARD_CORNERS_ROWS*CHESSBOARD_CORNERS_COLS-1];
        lower_right_inner = corners[(CHESSBOARD_CORNERS_ROWS-1)*CHESSBOARD_CORNERS_COLS-2];

        circle(*frame_camera, upper_left, 5, ORANGE, -1);
        circle(*frame_camera, upper_right, 5, ORANGE, -1);
        circle(*frame_camera, lower_left, 5, ORANGE, -1);
        circle(*frame_camera, lower_right, 5, ORANGE, -1);

        circle(*frame_camera, upper_left_inner, 5, BLUE, -1);
        circle(*frame_camera, upper_right_inner, 5, BLUE, -1);
        circle(*frame_camera, lower_left_inner, 5, BLUE, -1);
        circle(*frame_camera, lower_right_inner, 5, BLUE, -1);

        // Calculate the new
        upper_left.x += 2*(upper_left.x - upper_left_inner.x);
        upper_left.y += 2*(upper_left.y - upper_left_inner.y);
        upper_right.x += 2*(upper_right.x - upper_right_inner.x);
        upper_right.y += 2*(upper_right.y - upper_right_inner.y);
        lower_left.x += 2*(lower_left.x - lower_left_inner.x);
        lower_left.y += 2*(lower_left.y - lower_left_inner.y);
        lower_right.x += 2*(lower_right.x - lower_right_inner.x);
        lower_right.y += 2*(lower_right.y - lower_right_inner.y);

        circle(*frame_camera, upper_left, 5, RED, -1);
        circle(*frame_camera, upper_right, 5, RED, -1);
        circle(*frame_camera, lower_left, 5, RED, -1);
        circle(*frame_camera, lower_right, 5, RED, -1);


        // TODO: Check the extrapolated chessboard points


        // Draw blue border around projector image to show it has been found
        // line(*frame_projector, Point2f(), centroid_transformed, stylus_color, stylus_width, 8);
        // rectangle(*frame_projector, Point2f(), centroid_transformed, stylus_color, stylus_width, 3);
        rectangle(*frame_projector, Rect(0,0,IMAGE_WIDTH,IMAGE_HEIGHT), BLUE, 10`);




        // imshow("Verify Chessboard", *frame_camera);



        // The chessboard

        return Mat();
    }
    else {
        cout << "chessboard not found!" << endl;
        // cout << "Could not find chessboard!!!" << endl;
        return Mat();
    }



}




Mat green_rectangle_calibration(Mat *frame_camera, Mat *frame_projector, bool calculate_transform) {
    // Save off a pristine version of the input camera image
    Mat frame_camera_original;
    (*frame_camera).copyTo(frame_camera_original);

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


    // Print what the center of the camera sees in RGB and HSV
    Point2i image_center = Point2i(IMAGE_WIDTH/2, IMAGE_HEIGHT/2);

    stringstream coordinates, hsv_pixel_value, bgr_pixel_value;
    Vec3b hsv_pixel = frame_camera_hsv.at<Vec3b>(image_center.x, image_center.y);
    Vec3b bgr_pixel = frame_camera_original.at<Vec3b>(image_center.x, image_center.y);

    cout << "frame_camera_original: " << endl;
    cout << frame_camera_original.size() << endl;
    // cout << frame_camera_original.shape << endl;
    // cout << frame_camera_original.dtype() << endl;

    // Channels is type()/2 + 1
    // type()%2 = 8U if 0, 16U if 2, 32F if 5, 64F if 6
    CV_Assert(frame_camera_original.type() == CV_8UC3);

    circle(frame_contours, Point(320, 240), 2, ORANGE, -1);
    circle(frame_camera_original, Point(320, 240), 10, ORANGE, 2);
    imshow("Real", frame_camera_original);

    // cout << CV_32F << endl;
    // cout << CV_32U << endl;
    // cout << CV_ << endl;
    // cout << frame_camera_original.rows << endl;
    // cout << frame_camera_original.cols << endl;
    // cout << frame_camera_original.at<uint>(0,0) << endl;
    // cout << frame_camera_original.at<float>(0,0) << endl;
    // cout << frame_camera_original.at<double>(0,0) << endl;
    // cout << frame_camera_original.at<char>(0,0) << endl;
    // cout << frame_camera_original.height << endl;
    // cout << frame_camera_original.width << endl;
    // cout << frame_camera_hsv.type() << endl;
    // cout << frame_camera_hsv.type() << endl;

    cout << "hsv: " << hsv_pixel << endl;
    cout << "bgr: " << bgr_pixel << endl;





    hsv_pixel_value << "HSV at (" << image_center.x << "," << image_center.y << "): (" << setprecision(3) << (int)hsv_pixel[0] << ", " << (int)hsv_pixel[1] << ", " << (int)hsv_pixel[2] << ")";
    bgr_pixel_value << "BGR at (" << image_center.x << "," << image_center.y << "): (" << setprecision(3) << (int)bgr_pixel[0] << ", " << (int)bgr_pixel[1] << ", " << (int)bgr_pixel[2] << ")";

    putText(frame_contours, hsv_pixel_value.str(), Point2f(10, IMAGE_HEIGHT - 20), FONT_HERSHEY_SIMPLEX, 0.6, ORANGE, 2);
    putText(frame_contours, bgr_pixel_value.str(), Point2f(10, IMAGE_HEIGHT - 40), FONT_HERSHEY_SIMPLEX, 0.6, ORANGE, 2);


    // putText(frame_contours, coordinates_left, Point2f(image_points_left[i].x - 10, image_points_left[i].y - 30), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0,165,255), 2);


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

        // // Test the perspective transform on the calibration image
        // Mat frame_perspective_test;
        // warpPerspective(frame_camera_original, frame_perspective_test, transform_matrix, Size(IMAGE_WIDTH, IMAGE_HEIGHT));
        // imshow("Perspective Transform", frame_perspective_test);

        // use perspectiveTransform() to transform only select points instead of an entire image
        // perspectiveTransform(random_points, random_points_transformed, transform_matrix);

        return transform_matrix;
    }
    else {
        // Return an empty matrix as the perspective transform
        return Mat();
    }
}




// if calculate_transform is false, a green rectangle is produced on the projector frame and the code tries to find it in the camera frame (returns an empty Mat)
// if calculate_transform is true, it takes the currently detected calibration rectangle and calculates the perspective transform Mat from that

Mat calibrate_smartboard(Mat *frame_camera, Mat *frame_projector, bool calculate_transform) {
    // Save off a pristine version of the input camera image
    Mat frame_camera_original;
    (*frame_camera).copyTo(frame_camera_original);

    // Calibration method
    // return green_rectangle_calibration(frame_camera, frame_projector, calculate_transform);
    return chessboard_calibration(frame_camera, frame_projector, calculate_transform);
}


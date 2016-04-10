#include <opencv2/opencv.hpp>

#include "drawing.h"
#include "globals.h"

using namespace std;
using namespace cv;

// This is the center point of stylus from the last frame, if it existed
Point2d prev_pt = Point(-1,-1);

// Initialize constants set in the header file
const int HUE_MARKER_BUFFER = 15;
const int MIN_SAT = (int) 0.3 * 255;
const int MAX_SAT = (int) 1.0 * 255;
const int MIN_VAL = (int) 0.1 * 255;
const int MAX_VAL = (int) 1.0 * 255;
const int HUE_MARKER = 55;
const Scalar GREEN_LOWER_HSV =  Scalar(HUE_MARKER - HUE_MARKER_BUFFER, MIN_SAT, MIN_VAL);
const Scalar GREEN_UPPER_HSV =  Scalar(HUE_MARKER + HUE_MARKER_BUFFER, MAX_SAT, MAX_VAL);
// greenLower = (35, 60, 50) #(40,int(0.3*255),int(0.1*255))# (80,60,140)#
// greenUpper = (64, 255, 255) #(70,int(1*255),int(1*255))# (130,100,200)#




// pts = deque(maxlen=args["buffer"])
// blurme = True
// center = (320,240)
// lineThick = 2
// lineMin = 2
// eraseThick = 30
// eraseMin = 5
// red = (0,0,255)
// green = (0,255,0)
// blue = (255,0,0)
// orange = (0,80,200)
// markerColors = [red, green, blue,orange]
// lineColor = 0
// dwnarrow = 63233
// uparrow = 63232
// larrow = 63234
// rarrow = 63235
// liveVideo = True
// cornerWidth = 75
// width = 640
// height = 480




// TODO: Implement other functions here
// set_stylus_width
// set_stylus_color
// erase
// clear
//


// TODO: Create helper function to find the stylus, then have draw and erase both use it
// Since this is a private function, do not expose it in the header
vector<Point> find_stylus_contour(Mat *frame_camera){
    Mat frame_camera_hsv, frame_camera_hsv_binary;
    cvtColor(*frame_camera, frame_camera_hsv, COLOR_BGR2HSV);


    inRange(frame_camera_hsv, GREEN_LOWER_HSV, GREEN_UPPER_HSV, frame_camera_hsv_binary);

    // Do erode and dilate
    int const ITERATIONS = 5;
    erode(frame_camera_hsv_binary, frame_camera_hsv_binary, Mat(), Point(-1,-1), ITERATIONS);
    dilate(frame_camera_hsv_binary, frame_camera_hsv_binary, Mat(), Point(-1,-1), ITERATIONS);

    // Find contours
    Mat frame_contours;
    cvtColor(frame_camera_hsv_binary, frame_contours, COLOR_GRAY2BGR);

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    // Use CV_RETR_EXTERNAL instead of TREE, so we only grab outer-most contours
    // Use CV_CHAIN_APPROX_SIMPLE to store less points
    findContours(frame_camera_hsv_binary.clone(), contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    if(contours.size() > 0){
        // Find the contour with the largest area and assume that it is the stylus
        double largest_area = 0.0;
        int largest_contour_index = -1;
        for (int i = 0; i < contours.size(); ++i) {
            double a = contourArea(contours[i]);
            if(a > largest_area){
                largest_contour_index = i;
            }
        }

        vector<Point> contour = contours[largest_contour_index];
        return contour;
    }
    else {
        // No contours were found... Return empty vector
        // cout << "The stylus wasn't found" << endl;
        CV_Assert(vector<Point>().empty());
        return vector<Point>();
    }
}




void draw(Mat *frame_camera, Mat *frame_projector, Mat perspective_transform) {

    // Create the initial ink frame and set it to all black
    Mat frame_ink = Mat(Size(IMAGE_WIDTH,IMAGE_HEIGHT), CV_8UC3);
    frame_ink.setTo(BLACK);
    (*frame_projector).setTo(BLACK);

    // Find the contour with the largest area
    vector<Point> contour = find_stylus_contour(frame_camera);
    if(contour.empty()){
        // The point wasn't found... No need to draw anything I guess
        prev_pt = Point(-1,-1);
        return;
    }

    // Get centroid and enclosing circle radius of contour
    Point2f centroid;
    float stylus_enclosing_radius;
    minEnclosingCircle(contour, centroid, stylus_enclosing_radius);

    // // get centroid of max area contour
    // Moments ms = moments(contour);
    // centroid = Point2d(ms.m10 / ms.m00, ms.m01 / ms.m00);

    // Draw the center point of the stylus
    circle(*frame_camera, centroid, 5, BLUE, -1);

    circle(*frame_camera, centroid, stylus_enclosing_radius, Scalar(200, 200, 0), 2);


    // if point inside projector area - secondary priority
    // transform centroid to projector frame - prev_pt will already be in projector frame
    // draw line from prev_pt to centroid

    // circle(frame_contours, centroid, 5, GREEN, 3);

    prev_pt = centroid;
}
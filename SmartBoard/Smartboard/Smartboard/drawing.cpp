#include <opencv2/opencv.hpp>

#include "globals.h"
#include "drawing.h"

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
// markerColors = [red, green, blue,orange]
// lineColor = 0
// dwnarrow = 63233
// uparrow = 63232
// larrow = 63234
// rarrow = 63235
// liveVideo = True
// cornerWidth = 75

vector<Scalar> stylus_colors = {
    RED,
    // GREEN, // This doesn't work with our color detection...
    BLUE,
    ORANGE,
    WHITE,
    GRAY,
};
int stylus_colors_index = 0;
Scalar stylus_color = stylus_colors[stylus_colors_index];




int stylus_width = 3;

// Have a saved color, for when switching between erase and draw
Scalar stylus_color_saved = RED;
int stylus_width_saved = 3;

// Create the initial ink frame and set it to all black
Mat frame_ink = Mat(Size(IMAGE_WIDTH,IMAGE_HEIGHT), CV_8UC3);

// frame_ink.setTo(BLACK);

// TODO: Implement other functions here
// set_stylus_width
// set_stylus_color
// erase
// clear/reset ink frame
//


// Create helper function to find the stylus, then have draw and erase both use it
// Since this is a private function, do not expose it in the header
vector<Point> find_stylus_contour(Mat *frame_camera, Mat perspective_transform){
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
            vector<Point> contour = contours[i];
            // get centroid of max area contour
            Moments ms = moments(contour);
            Point2d centroid = Point2d(ms.m10 / ms.m00, ms.m01 / ms.m00);
            vector<Point2d> centroid_array = {centroid};
            vector<Point2d> centroid_array_transformed = {centroid};
            // Check to see if the centroid, when transformed, is within the image. If not, ignore it!
            perspectiveTransform(centroid_array, centroid_array_transformed, perspective_transform);
            Point2d centroid_transformed = centroid_array_transformed[0];
            // cout << "centroid_transformed:" << centroid_transformed << endl;
            // Skip the point if it is outside the ROI of the board
            if(centroid_transformed.x < 0 || centroid_transformed.x > IMAGE_WIDTH-1 || centroid_transformed.y < 0 || centroid_transformed.y > IMAGE_HEIGHT-1){
                // cout << "point is outside ROI! Must not be stylus..." << endl;
                continue;
            }


            double a = contourArea(contour);
            if(a > largest_area){
                largest_contour_index = i;
            }
        }

        if (largest_contour_index != -1){
            // We found a good stylus candidate contour
            return contours[largest_contour_index];
        }
        else {
            return vector<Point>();
        }
    }
    else {
        // No contours were found... Return empty vector
        // cout << "The stylus wasn't found" << endl;
        CV_Assert(vector<Point>().empty());
        return vector<Point>();
    }
}



void draw(Mat *frame_camera, Mat *frame_projector, Mat perspective_transform) {
    (*frame_projector).setTo(BLACK);

    // Find the contour with the largest area
    vector<Point> contour = find_stylus_contour(frame_camera, perspective_transform);
    if(contour.empty()){
        // Stylus wasn't found, so just paint the ink frame and return
        add(*frame_projector, frame_ink, *frame_projector);
        return;
    }

    // Get centroid and enclosing circle radius of contour
    Point2f centroid;
    float stylus_enclosing_radius;
    minEnclosingCircle(contour, centroid, stylus_enclosing_radius);


    // Draw the center point of the stylus
    circle(*frame_camera, centroid, 5, BLUE, -1);
    circle(*frame_camera, centroid, stylus_enclosing_radius, Scalar(200, 200, 0), 2);

    // TODO: if point inside projector area - secondary priority

    // transform centroid to projector frame - prev_pt will already be in projector frame
    vector<Point2f> points, points_transformed;
    // Transform the centroid to the projector coordinates
    points.push_back(centroid);

    // use perspectiveTransform() to transform only select points instead of an entire image
    perspectiveTransform(points, points_transformed, perspective_transform);

    Point2f centroid_transformed = points_transformed[0];

    // Create a throw-away frame for the stylus
    Mat frame_stylus;
    (*frame_projector).copyTo(frame_stylus);
    // Draw the center point of the stylus
    circle(frame_stylus, centroid_transformed, 5, BLUE, -1);
    // Draw a circle around the stylus
    if(PROJECTOR){
        circle(frame_stylus, centroid_transformed, stylus_enclosing_radius*1.5 + 10, Scalar(200, 200, 0), 2);
    }
    else {
        circle(frame_stylus, centroid_transformed, stylus_enclosing_radius, Scalar(200, 200, 0), 2);
    }

    // draw line from prev_pt to centroid, if prev_pt exists
    if(prev_pt.x != -1){
        const float PT_DIST_DIFF_THRESHOLD = 100;
        float pt_dist_diff = sqrt(pow(prev_pt.x - centroid_transformed.x, 2) + pow(prev_pt.y - centroid_transformed.y, 2) );
        if(pt_dist_diff < PT_DIST_DIFF_THRESHOLD){
            line(frame_ink, prev_pt, centroid_transformed, stylus_color, stylus_width, 8);
        }
        else {
            // "Close up the draw" so it's not as glitchy-looking
            line(frame_ink, prev_pt, prev_pt, stylus_color, stylus_width, 8);
        }
    }

    // Combine frame_ink with the projector frame
    add(*frame_projector, frame_ink, *frame_projector);
    add(*frame_projector, frame_stylus, *frame_projector);

    if(PROJECTOR){
        // Draw black over the stylus, so that the projector light does not mess up the color detection
        circle(*frame_projector, centroid_transformed, stylus_enclosing_radius*1.5, BLACK, -1);
    }



    // Save the transformed centroid as the previous point
    prev_pt = centroid_transformed;
}


void erase(Mat *frame_camera, Mat *frame_projector, Mat perspective_transform) {
    // An erase is fundamentally just drawing black, with the stylus looking red
    save_stylus();

    set_stylus_color(BLACK);
    set_stylus_width(30);

    draw(frame_camera, frame_projector, perspective_transform);

    load_saved_stylus();
}


void set_stylus_color(Scalar color){
    stylus_color = color;
}

void set_stylus_width(int width){
    stylus_width = width;
}

// Saves the current stylus configuration
void save_stylus() {
    stylus_width_saved = stylus_width;
    stylus_color_saved = stylus_color;
}

// Loads the saved stylus configuration
void load_saved_stylus(){
    stylus_color = stylus_color_saved;
    stylus_width = stylus_width_saved;
}


void clear(Mat *frame_projector) {
    frame_ink.setTo(BLACK);
    (*frame_projector).setTo(BLACK);
}


void change_color_right(){
    stylus_colors_index++;
    if(stylus_colors_index >= stylus_colors.size()){
        stylus_colors_index = 0;
    }
    set_stylus_color(stylus_colors[stylus_colors_index]);
};
void change_color_left(){
    stylus_colors_index--;
    if(stylus_colors_index < 0){
        stylus_colors_index = stylus_colors.size()-1;
    }
    set_stylus_color(stylus_colors[stylus_colors_index]);
};
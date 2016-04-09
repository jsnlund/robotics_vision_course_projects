#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// #include "initialize.h"
#include "calibration.h"
#include "drawing.h"
#include "control.h"
#include "globals.h"


// Main control loop
int main(){
    cout << "Starting Smartboard...\n";

    //
    //// Initialization
    //

    //
    //// Set up video capture objects
    //
    VideoCapture camera_1(0);
    VideoCapture camera_2(1);

    // Check to see if the cameras were opened properly
    bool camera_1_opened = false;
    bool camera_2_opened = false;
    if(!camera_1.isOpened()) {
        cout << "Camera 1 could not be opened!" << endl;
    }
    else{
        camera_1_opened = true;
    }

    if(!camera_2.isOpened()) {
        cout << "Camera 2 could not be opened!" << endl;
    }
    else{
        camera_2_opened = true;
    }




    // Final output of the projector
    Mat frame_projector;
    // Final output image of the camera
    Mat frame_camera;

    // if(FULLSCREEN){
    //     namedWindow("Projector", CV_WINDOW_NORMAL);
    //     cvSetWindowProperty("Projector", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    // }


    // Set up processing controls
    int keypress = 0;
    int processingMode = 0;

    Mat perspective_transform;


    // Main loop
    while (keypress != ESC_KEY) {



        switch(processingMode){
            // Calibration Mode
            case SPACE_KEY:
                perspective_transform = calibrate_smartboard(&frame_camera, &frame_projector, true);
                break;
            case KEY_C:
                perspective_transform = calibrate_smartboard(&frame_camera, &frame_projector, false);
                break;
            default:
            // Drawing mode
            case KEY_1:
                // Detection
                // Draw points
                // set display frames
                break;
        }

        // Display the projector view
        imshow("Projector", frame_projector);
        // Display the camera (laptop) view
        imshow("Projector", frame_camera);


        keypress = waitKey(30);
        // If no key was pressed, it is -1
        if(keypress >= 0){
            processingMode = keypress;
        }
    }





    return 0;
}



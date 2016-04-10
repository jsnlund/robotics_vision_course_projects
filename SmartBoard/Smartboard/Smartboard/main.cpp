#include <opencv2/opencv.hpp>
// opencv imcludes iostream already for cout...

using namespace cv;
using namespace std;

#include "calibration.h"
#include "drawing.h"
#include "globals.h"

//
////  Define constants for the main control loop
//

// Keyboard codes
int const ESC_KEY = 27; // Exit program
int const SPACE_KEY = 32; // capture perspective transform

int const KEY_C = 99; // enter calibrate mode
int const KEY_D = 100; // draw
int const KEY_E = 101; // erase

int const KEY_Q = 113; // quit/stop
int const KEY_W = 119; //



int const KEY_1 = 49; // 1
int const KEY_2 = 50; // 2
int const KEY_3 = 51; // 3
int const KEY_4 = 52; // 4
int const KEY_5 = 53; // 5
int const KEY_6 = 54; // 6
int const KEY_7 = 55; // 7
int const KEY_8 = 56; // 8
int const KEY_9 = 57; // 9



// Main control loop
int main(){
    cout << "Starting Smartboard...\n";

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
    Mat frame_projector = Mat(Size(IMAGE_WIDTH, IMAGE_HEIGHT), CV_8UC3);
    frame_projector.setTo(ORANGE);
    // Final output image of the camera
    Mat frame_camera = Mat(Size(IMAGE_WIDTH, IMAGE_HEIGHT), CV_8UC3);
    frame_camera.setTo(ORANGE);

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
        camera_2 >> frame_camera;


        switch(processingMode){
            //
            // Calibration Mode
            //
            case KEY_C:
                // Acquire mode - continual
                perspective_transform = calibrate_smartboard(&frame_camera, &frame_projector, false);
                break;
            case SPACE_KEY:
                // Calculate transform mode
                perspective_transform = calibrate_smartboard(&frame_camera, &frame_projector, true);
                // This should only happen once, so...
                // TODO: Immediately transfer to drawing mode? Or just reset and let the user enter drawing mode?
                processingMode = -1;
                break;
            //
            // Drawing mode
            //
            case KEY_D:
                if(perspective_transform.empty()){
                    cout << "perspective_transform has not been calculated yet! Please calibrate" << endl;
                    // Reset processing mode
                    processingMode = -1;
                }
                else {
                    // TODO: If in drawing mode, enable different commands? Ignore other commands?
                    // Detection
                    // Draw points
                    // set display frames
                    draw(&frame_camera, &frame_projector, perspective_transform);
                }
                break;
            default:
                // Nothing is happening - just show live feed
                break;
        }


        // Display the projector view
        imshow("Projector", frame_projector);

        // Display the camera (laptop) view
        imshow("Camera", frame_camera);

        keypress = waitKey(30);
        // If no key was pressed, it is -1
        if(keypress >= 0){
            processingMode = keypress;
        }
    }





    return 0;
}



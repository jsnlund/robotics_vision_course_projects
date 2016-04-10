#pragma once

//
// Header file guidlines:
//
// Only expose variables and functions in the header file that you want other files to see.
// You don't need to define every global variable or function in the header file that is in the cpp file.
// Declare things in header files, and define things (initialize) in cpp files.
// When defining constants, use "extern const" in the header without defining it, and then define/implement it in the cpp file.
// Do NOT use "using namespace" in headers - only in cpp files.
// Do NOT include cpp files in headers.

void draw(cv::Mat *frame_camera, cv::Mat *frame_projector, cv::Mat perspective_transform);
void erase(cv::Mat *frame_camera, cv::Mat *frame_projector, cv::Mat perspective_transform);
void clear(cv::Mat *frame_projector);


// TODO: Implement other functions here
// set_stylus_width
// set_stylus_color
// erase
// clear



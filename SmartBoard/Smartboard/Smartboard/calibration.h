// calibration.h will contain global variables and functions for calibration.cpp
#pragma once

// Declare things in header files, and define things (initialize) in cpp files

cv::Mat calibrate_smartboard(cv::Mat *frame_camera, cv::Mat *frame_projector, bool calculate_transform);

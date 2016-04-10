//create empty mat same size as chess

Mat frame_calibration = Mat(CALIBRATION_FRAME_SIZE, CV_8UC3);
frame_calibration.setTo(BLACK);

// int rows = frame_calibration.rows;
// int cols = frame_calibration.cols;

cv::Size s = frame_calibration.size();
height = s.height;
width = s.width;

int rows = 12;
int cols = 16;

int sq = width/cols;
int sq2 = sq - 1;
int s;

for (int i = 0; i < rows; ++i){
	if(i % 2 == 0){ s = 0; }
	else{ s = 1; }
	for (int j = 0; j < cols; ++j){
		if((j-s) % 2 == 0){
			int x = j*sq
			int y = i*sq
			rectangle(chess, Point(x,y), Point(x+sq2,y+sq2), WHITE, CV_FILLED)//, int lineType=8, int shift=0)
		}
	}
}


frame_calibration.copyTo(*frame_projector);
//create empty mat same size as chess

Mat frame_calibration = Mat(CALIBRATION_FRAME_SIZE, CV_8UC3);
frame_calibration.setTo(WHITE);

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
// int border = sq2

for (int i = 1; i < rows-1; ++i){
	if(i % 2 == 0){ s = 0; }
	else{ s = 1; }
	for (int j = 1; j < cols-1; ++j){
		if((j-s) % 2 == 0){
			int x = j*sq
			int y = i*sq
			rectangle(chess, Point(x,y), Point(x+sq2,y+sq2), BLACK, CV_FILLED)//, int lineType=8, int shift=0)
		}
	}
}


frame_calibration.copyTo(*frame_projector);
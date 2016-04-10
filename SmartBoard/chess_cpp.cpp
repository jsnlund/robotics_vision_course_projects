//create empty mat same size as chess


cv:Mat mat;
int rows = mat.rows;
int cols = mat.cols;

cv::Size s = frame_camera.size();
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
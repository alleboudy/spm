#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <chrono>
using namespace cv;
using namespace std;

template<typename T> 
T *PrepareFrame(cv::Mat &in, uchar * dst, int &min, int &max) { 
	T *out = new T[in.rows * in.cols]; 
	for (int i = 0; i < in.rows; ++i) 
		for (int j = 0; j < in.cols; ++j) {
			Vec3b intensity = in.at<Vec3b>(i, j);//changing to grayscale while at it :D
			out[i * (in.cols) + j] = (intensity.val[0]+intensity.val[1]+intensity.val[2])/3; 
			max=out[i * (in.cols) + j]>max?out[i * (in.cols) + j]:max;
			min=out[i * (in.cols) + j]<min?out[i * (in.cols) + j]:min;
			dst[i * (in.cols) + j]=0;
		}
	return out; 
} 
#define XY2I(Y,X,COLS) (((Y) * (COLS)) + (X)) 
// returns the gradient in the x direction 
static inline long xGradient(uchar * image, long cols, long x, long y) { 
	return image[XY2I(y-1, x-1, cols)] + 
		2*image[XY2I(y, x-1, cols)] + 
		image[XY2I(y+1, x-1, cols)] - 
		image[XY2I(y-1, x+1, cols)] - 
		2*image[XY2I(y, x+1, cols)] - 
		image[XY2I(y+1, x+1, cols)]; 
} 
// returns the gradient in the y direction 
static inline long yGradient(uchar * image, long cols, long x, long y) { 
	return image[XY2I(y-1, x-1, cols)] + 
		2*image[XY2I(y-1, x, cols)] + 
		image[XY2I(y-1, x+1, cols)] - 
		image[XY2I(y+1, x-1, cols)] - 
		2*image[XY2I(y+1, x, cols)] - 
		image[XY2I(y+1, x+1, cols)]; 
} 




int main(int argc, char* argv[])
{

 if(argc != 4) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath sobel \n [ sobel for the sobel filter, otherwise contrast stretching is applied]"<<endl;
      return(-1); 
    }
	
	bool sobel=(string(argv[3])=="sobel");

	VideoCapture cap(argv[1]);
	VideoWriter vwr;
	if (!cap.isOpened())
		throw "Error when reading video file";
	
	long cols=(int)cap.get(CV_CAP_PROP_FRAME_WIDTH), rows = (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	Size S = Size(cols,rows);

	vwr.open(argv[2], CV_FOURCC('M','P','4','2'), cap.get(CV_CAP_PROP_FPS), S,false);
	
	if (!vwr.isOpened())
		throw "Error when opening the vide writer";


	auto started = std::chrono::high_resolution_clock::now();

	uchar * src;
	uchar * dst;
	Mat * frame ;

	while (true)
	{
		frame = new Mat();
		if(cap.read(*frame)){
				dst = new uchar[rows * cols];
				int min=255, max=0;
				src=PrepareFrame<uchar>(*frame,dst, min, max);
				
				for (int y = 1; y < rows-1; ++y){
					for (int x = 1; x < cols-1; ++x){
						if(sobel){
						const long gx = xGradient(src, cols, x, y); 
						const long gy = yGradient(src, cols, x, y); 
						long sum = abs(gx) + abs(gy); 
						if (sum > 255) sum = 255; 
						else if (sum < 0) sum = 0; 
						dst[y*cols+x] = sum; 
					}else{
						dst[y*cols+x] = 255.0 / (max - min)*(src[y*cols+x] - min);
						}
					}
				}


				


					(*frame) = Mat(rows, cols, CV_8U, dst, Mat::AUTO_STEP);
					
					vwr.write(*frame);

				}
	else{
		break;	
	}

	
	delete frame;
	}
	

	vwr.release();
	cap.release();

	auto done = std::chrono::high_resolution_clock::now();

std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count()<<endl;

return 0;

}


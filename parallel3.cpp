#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <chrono>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <ff/parallel_for.hpp>

using namespace cv;
using namespace std;
using namespace ff;

//This function does the preprocessing required for both filter

template<typename T> 
T *PrepareFrame(cv::Mat &in,uchar * dst, int &min, int &max) { 
	T *out = new T[in.rows * in.cols]; 
	for (int i = 0; i < in.rows; ++i) 
		for (int j = 0; j < in.cols; ++j) {
			Vec3b intensity = in.at<Vec3b>(i, j);//changing to grayscale while at it :D
			out[i * (in.cols) + j] = (intensity.val[0]+intensity.val[1]+intensity.val[2])/3; 
			dst[i * (in.cols) + j]=0;
			max=out[i * (in.cols) + j]>max?out[i * (in.cols) + j]:max;
			min=out[i * (in.cols) + j]<min?out[i * (in.cols) + j]:min;
		}
	return out; 
} 
//very useful for vectorization and fast access to the pixels values

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

 if(argc != 5) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 3 sobel \n where 3 is the number of workers to use [ sobel for the sobel filter, otherwise contrast stretching is applied]"<<endl;
      return(-1); 
    }
	
	bool sobel=(string(argv[4])=="sobel");

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
	ParallelFor pr(atoi(argv[3]));
	while (true)
	{
		dst = new uchar[rows * cols];
		frame = new Mat();
		if(cap.read(*frame)){
				
				int min=255, max=0;
				 src=PrepareFrame<uchar>(*frame,dst,min,max);

				pr.parallel_for(1,rows-1,[src,cols,rows,sobel,min,max,dst](const long y) { 
					for (int x = 1; x < cols-1; ++x){
						if(sobel){
						const long gx = xGradient(src, cols, x, y); 
						const long gy = yGradient(src, cols, x, y); 
						long sum = abs(gx) + abs(gy); 
						if (sum > 255) sum = 255; 
						else if (sum < 0) sum = 0; 
						dst[y*cols+x] = sum; 
					}else{
						dst[y*cols+x] = 255 / (max - min)*(src[y*cols+x] - min);
						}
					}
				});


				


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


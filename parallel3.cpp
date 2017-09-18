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
/* ----- utility function ------- */ 
template<typename T> 
T *Mat2uchar(cv::Mat &in) { 
	T *out = new T[in.rows * in.cols]; 
	for (int i = 0; i < in.rows; ++i) 
		for (int j = 0; j < in.cols; ++j) {
			Vec3b intensity = in.at<Vec3b>(i, j);//changing to grayscale while at it :D
			out[i * (in.cols) + j] = (intensity.val[0]+intensity.val[1]+intensity.val[2])/3; 
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

 if(argc != 5) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 3 sobel \n where 3 is the number of workers to use [ sobel for the sobel filter, otherwise contrast stretching is applied]"<<endl;
      return(-1); 
    }
	
	bool sobel=(string(argv[4])=="sobel");
	//cout<<"filter is sobel?: "<<sobel<<argv[3]<<endl;

	VideoCapture cap(argv[1]);
	VideoWriter vwr;
	if (!cap.isOpened())
		throw "Error when reading video file";
	//Mat* frame;

	/*int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

	// Transform from int to char via Bitwise operators
	char EXT[] = { (char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16), (char)((ex & 0XFF000000) >> 24), 0 };
*/
	long cols=(int)cap.get(CV_CAP_PROP_FRAME_WIDTH), rows = (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	Size S = Size(cols,rows);

	vwr.open(argv[2], CV_FOURCC('M','P','4','2'), cap.get(CV_CAP_PROP_FPS), S,false);
	//cout << "Input frame resolution: Width=" << S.width << "  Height=" << S.height
	//	<< " of nr#: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	//cout << "Input codec type: " << EXT << endl;
	if (!vwr.isOpened())
		throw "Error when opening the vide writer";

	//namedWindow("w", 1);

	auto started = std::chrono::high_resolution_clock::now();

	uchar * src;
	Mat * frame ;
	ParallelFor pr(atoi(argv[3]));
	while (true)
	{
		frame = new Mat();
		if(cap.read(*frame)){
				
				src=Mat2uchar<uchar>(*frame);
				//bool sob=sobel;
				double min, max;
				if(!sobel)
				cv::minMaxLoc(*frame, &min, &max);

					//well, no need for the parallel for then :D
				pr.parallel_for(1,rows-1,[src,cols,rows,sobel,min,max](const long y) { 
					for (int x = 1; x < cols-1; ++x){
						if(sobel){
						const long gx = xGradient(src, cols, x, y); 
						const long gy = yGradient(src, cols, x, y); 
						long sum = abs(gx) + abs(gy); 
						if (sum > 255) sum = 255; 
						else if (sum < 0) sum = 0; 
						src[y*cols+x] = sum; 
					}else{
						src[y*cols+x] = 255 / (max - min)*(src[y*cols+x] - min);
						}
					}
				});


				


					(*frame) = Mat(rows, cols, CV_8U, src, Mat::AUTO_STEP);
					
					vwr.write(*frame);

				}
	else{
		break;	
	}

		//imshow("w", inverted);
	//	waitKey(20); // waits to display frame
	//delete src;
	delete frame;
	}
	//waitKey(0); // key press to close window

	//cout << "done" << endl;

	vwr.release();
	cap.release();

	auto done = std::chrono::high_resolution_clock::now();

std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count()<<endl;

return 0;

}


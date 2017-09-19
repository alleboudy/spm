#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include "opencv2/videoio.hpp"
#include <thread>
#include <chrono>

using namespace cv;
using namespace std;



/* ----- utility function ------- */ 
template<typename T> 
T *PrepareFrame(Mat &in, uchar * dst, int &min, int &max) { 
	T *out = new T[in.rows * in.cols]; 
	for (int i = 0; i < in.rows; ++i) 
		for (int j = 0; j < in.cols; ++j) {
			Vec3b intensity = in.at<Vec3b>(i, j);//changing to grayscale while at it :D
			out[i * (in.cols) + j] = (intensity.val[0]+intensity.val[1]+intensity.val[2])/3; 
			dst[i * (in.cols) + j]=0;//while at it, setting the resulting frame to 0
			max=out[i * (in.cols) + j]>max?out[i * (in.cols) + j]:max;
			min=out[i * (in.cols) + j]<min?out[i * (in.cols) + j]:min;
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




void ProcessFrame(Mat frame,Mat* resultStorage ,bool sobel,int cols,int rows)
{ 
	
	 uchar * dst = new uchar[rows * cols];
	int min=255, max=0;
	uchar * src=PrepareFrame<uchar>(frame,dst,min,max);
	 

				

					//well, no need for the parallel for then :D
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
						dst[y*cols+x] = 255 / (max - min)*(src[y*cols+x] - min);
						}
					}
				}


				


					(*resultStorage) = Mat(rows, cols, CV_8U, dst, Mat::AUTO_STEP);
					//delete src;
					//delete dst;
					
}

int main(int argc, char* argv[])
{

 if(argc != 5) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 2 sobel"<<" where 2 is the number of threads to use and sobel is the filter, otherwise contrast stretching"<<endl;
      return(-1); 
    }
	bool sobel=(string(argv[4])=="sobel");
	int bufferSize=atoi(argv[3]);
	VideoCapture cap(argv[1]);
	VideoWriter vwr;
	if (!cap.isOpened())
		throw "Error when reading video file";
	

	/*int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

	// Transform from int to char via Bitwise operators
	char EXT[] = { (char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16), (char)((ex & 0XFF000000) >> 24), 0 };
*/
	int cols=(int)cap.get(CV_CAP_PROP_FRAME_WIDTH),rows = (int)cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	Size S = Size(cols,rows);

	vwr.open(argv[2], CV_FOURCC('M','P','4','2'), cap.get(CV_CAP_PROP_FPS), S,false);
	//cout << "frame Width=" << S.width << ",  Height=" << S.height<< " number of frames: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	//cout << "Input codec type: " << EXT << endl;
	if (!vwr.isOpened())
		throw "Error when opening the vide writer";

	auto started = std::chrono::high_resolution_clock::now();
	//namedWindow("w", 1);
	Mat workersFrames[bufferSize];
	//thread workersThreads[bufferSize];// array of bufferSize threads
	//int ctr=0;

	int wrkrCntr=0;
	vector<thread*> workersThreads;
	while (true)
	{
		Mat*  frame = new Mat();
			//cout<<"###wrkrCntr###"<<wrkrCntr<<endl;
	    	if(cap.read(*frame)){
				workersFrames[wrkrCntr] =  Mat();
	    		workersThreads.push_back(new thread(ProcessFrame,(*frame), &(workersFrames[wrkrCntr]),sobel,cols,rows));
	    	
	    	//workersThreads[wrkrCntr]=thread(ProcessFrame,*frame, &(workersFrames[wrkrCntr]),sobel,cols,rows);
	    	wrkrCntr++;//one frame = one worker
	    	//ctr++;
	    		if (workersThreads.size()==bufferSize)//bs=4 -> instantiated 4 threads already idx[0->3]
	    		{
	    			//cout<<"flushing threads results"<<endl;
	    			for (size_t x = 0; x < wrkrCntr; ++x)
	    			{
	    				//error, sometimes the same frame reappears later again :S
	    				try{
	    				(*(workersThreads[x])).join();
	    				}
	    				 catch (const std::exception& e) { cout<<e.what()<<endl; }
	    				vwr.write(((workersFrames[x])));
	    				//workersFrames[x] =  Mat();//making sure it won't be reused!
	    				//cout<<"flushed: "<<x<<endl;
	    			}
	    			
	    			wrkrCntr=0;
	    			//workersFrames.clear();
	    			workersThreads.clear();
	    		}
	    		
			}		

		
	    else{
		//cout << "end of video file" << endl;
		break;	
		}
		
		delete frame;
	}
	
	if(wrkrCntr>0){
		//cout<<"Final Flush :D"<<endl;
		//cout<<wrkrCntr<<endl;
		for(size_t i=0;i<wrkrCntr;i++){
			//cout<<i<<endl;
				(*(workersThreads[i])).join();
	    		vwr.write(((workersFrames[i])));
		}
		
	}
//	delete frame;
	//cout << "done" << endl;

	vwr.release();
	cap.release();
auto done = std::chrono::high_resolution_clock::now();

std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count()<<endl;

return 0;
}



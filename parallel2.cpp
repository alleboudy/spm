#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include "opencv2/videoio.hpp"
//#include <opencv2/imgproc/imgproc.hpp>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <thread>
#include <chrono>
using namespace cv;
using namespace std;
using namespace ff;



//This function does the preprocessing required for both filter
template<typename T> 
T *PrepareFrame(cv::Mat &in, uchar * dst, int &min, int &max) { 
	T *out = new T[in.rows * in.cols]; 
	for (int i = 0; i < in.rows; ++i) 
		for (int j = 0; j < in.cols; ++j) {
			Vec3b intensity = in.at<Vec3b>(i, j);//changing to grayscale 
			out[i * (in.cols) + j] = (intensity.val[0]+intensity.val[1]+intensity.val[2])/3; 
			dst[i * (in.cols) + j]=0;// setting the resulting frame to 0
			max=out[i * (in.cols) + j]>max?out[i * (in.cols) + j]:max;//keeping the max and min values
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


struct Emitter : ff_node_t<Mat> {
    VideoCapture cap;
    Emitter(VideoCapture inputVideoPath):cap(inputVideoPath) {}
   
    Mat * svc(Mat *) {
	while(true) {
		Mat * frame = new Mat();
	    if(cap.read(*frame)){
  		ff_send_out(frame);}
	    else{
		//cout << "end of video file" << endl;
		break;	
		}
	}
	cap.release();
	return EOS;
    }
}; 



struct Worker : ff_node_t<Mat> {
	//int numSubWrkrs=1;
	bool sobel=false;
	Worker(bool sobelApply){
	//numSubWrkrs = numberOfSubWorkers;
	sobel = sobelApply; 
	}
   	Mat * svc(Mat* frame) {



   	long cols=(*frame).cols, rows = (*frame).rows;
	uchar * dst = new uchar[rows * cols];
	int min=255, max=0;
	uchar * src=PrepareFrame<uchar>(*frame,dst,min,max);
		

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

	
	
   	
	return frame;
  }
}; 


struct Collector: ff_node_t<Mat> {
	VideoWriter vwr;

    Collector(string outputVideoPath,int ex, Size S,int fps) {
		vwr.open(outputVideoPath, ex, fps, S,false);
		}
    Mat *svc (Mat * frame) {
	if (!vwr.isOpened()){
		cerr<< "Error when opening the vide writer"<<endl;
		vwr.release();
		return EOS;
		}

	
	vwr.write(*frame);
	delete frame;
	return GO_ON;
    }

 void svc_end(){ 
	
        vwr.release(); 
    }  

}; 


int main(int argc, char* argv[])
{
	auto started = std::chrono::high_resolution_clock::now();

    if(argc != 5) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 2 sobel"<<endl<<"where 2 is the number of workers , sobel is the filter to apply [ sobel for the sobel filter, otherwise contrast stretching is applied] "<<endl; 
      return(-1); 
    }
    

	VideoCapture cap(argv[1]);

	if (!cap.isOpened()){
		cerr<< "Error, the video file was not opened!"<<endl;
		cap.release();
		return -1;
	}
	

    Size S = Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH),(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));
    int fps=cap.get(CV_CAP_PROP_FPS);
   
    int numOfWorkers = atoi(argv[3]);
    
    Emitter emitter(cap);
    
    
    bool aplySobel=(string(argv[4])=="sobel");
    ff_OFarm<Mat> ofarm( [numOfWorkers,aplySobel]() {
            vector<unique_ptr<ff_node> > wrkrptrs; 
            for(size_t i=0; i<numOfWorkers; i++){ 
                wrkrptrs.push_back(make_unique<Worker>(aplySobel));
		}
            return wrkrptrs;
        } ());

    Collector collector(argv[2],CV_FOURCC('M','P','4','2'),S,fps);
    ofarm.setEmitterF(emitter);
    ofarm.setCollectorF(collector);
    
    if (ofarm.run_and_wait_end()<0) {
        cerr<<"runtime error, exiting!"<<endl;
        return -1;
    }
  
    auto done = std::chrono::high_resolution_clock::now();

	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count()<<endl;

    return 0;



}


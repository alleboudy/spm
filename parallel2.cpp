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


struct Emitter : ff_node_t<Mat> {
    VideoCapture cap;
    Emitter(VideoCapture inputVideoPath):cap(inputVideoPath) {}
    Mat * svc(Mat *) {
	while(true) {
		Mat * frame = new Mat();
	    if(cap.read(*frame)){
	   // cvtColor(*frame, *frame, CV_RGB2GRAY); 
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

	//cout<<endl<<"hello from the other side"<<endl;
	/*bitwise_not(*frame, *frame);
	flip(*frame, *frame, 0);*/

   	long cols=(*frame).cols, rows = (*frame).rows;
	//ParallelFor pr(numSubWrkrs);
	uchar * src=Mat2uchar<uchar>(*frame);
	bool sob=sobel;
	double min, max;
	if(!sobel)
	cv::minMaxLoc(*frame, &min, &max);

	/*if (numSubWrkrs>1)
	{
		pr.parallel_for(1,rows-1,[src,cols,rows,sob,min,max](const long y) { 
			for(long x = 1; x < cols - 1; x++){ 
				if(sob){
				const long gx = xGradient(src, cols, x, y); 
				const long gy = yGradient(src, cols, x, y); 
				// approximation of sqrt(gx*gx+gy*gy) 
				long sum = abs(gx) + abs(gy); 
				if (sum > 255) sum = 255; 
				else if (sum < 0) sum = 0; 
				src[y*cols+x] = sum;
			}else{

				src[y*cols+x] = 255 / (max - min)*(src[y*cols+x] - min);

			}

				 
			} 
		}); 

	}else*/{	
			//well, no need for the parallel for then :D

		for (int y = 1; y < rows-1; ++y)
	{
		for (int x = 1; x < cols-1; ++x)
		{

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
	}


	}


		(*frame) = Mat(rows, cols, CV_8U, src, Mat::AUTO_STEP);

	
	
   	
	return frame;
  }
}; 


struct Collector: ff_node_t<Mat> {
VideoWriter vwr;
//vector<Mat>buffer;
//int outBufferSize=0;
    Collector(string outputVideoPath,int ex, Size S,int fps/*,int bufferSize*/) {
		vwr.open(outputVideoPath, ex, fps, S,false);
		//outBufferSize = bufferSize;
		}
    Mat *svc (Mat * frame) {
	if (!vwr.isOpened()){
		cerr<< "Error when opening the vide writer"<<endl;
		vwr.release();
		return EOS;
		}

	/*buffer.push_back((*frame).clone());
	if(buffer.size()>=outBufferSize){
	//cout<<"flushing the output buffer..."<<endl;
	for(size_t i=0;i<buffer.size();i++){
	vwr.write(buffer[i]);
	}
	buffer.clear();
	
	}*/
	vwr.write(*frame);
	delete frame;
	return GO_ON;
    }

 void svc_end(){ 
	/*//cout<<"closing the collector"<<endl;
	if(buffer.size()>0){
		//cout<<"Finaaaaal Fluuuuush [pun intended :D ] ..."<<endl;
		for(size_t i=0;i<buffer.size();i++){
			vwr.write(buffer[i]);

		}
	buffer.clear();
	
	}*/
        vwr.release(); 
    }  

}; 


int main(int argc, char* argv[])
{
	auto started = std::chrono::high_resolution_clock::now();

    if(argc != 5) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 2 100 sobel"<<endl<<"where 2 is the number of workers , sobel is the filter to apply [ sobel for the sobel filter, otherwise contrast stretching is applied] "<<endl; 
      return(-1); 
    }
    

	VideoCapture cap(argv[1]);

	if (!cap.isOpened()){
		cerr<< "Error, the video file was not opened!"<<endl;
		cap.release();
		return -1;
	}
	

    /*int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC)); 
    // Transform from int to char via Bitwise operators
    char EXT[] = { (char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16), (char)((ex & 0XFF000000) >> 24), 0 };
    */Size S = Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH),(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));
    int fps=cap.get(CV_CAP_PROP_FPS);
    //cout << "Input codec type: " << EXT << endl;
    //cout << "Frame  width=" << S.width << " ,   height=" << S.height<< " number of frames: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
    int numOfWorkers = atoi(argv[3]);
    
    Emitter emitter(cap);
    /*unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
	//cout<<"number of cores: "<<concurentThreadsSupported<<endl;
    int possibleNumOfCores = (concurentThreadsSupported-numOfWorkers)/numOfWorkers;
    int numOfSubWorkers=1;
    if ((string(argv[5])=="sub") &&possibleNumOfCores>0){
    	numOfSubWorkers =  possibleNumOfCores;
    	//cout<<"Sub parallelizing with: "<<numOfSubWorkers<<" cores"<<endl;
    }*/
    
    bool aplySobel=(string(argv[4])=="sobel");
	//cout<<"number of subworkers: "<<numOfSubWorkers<<endl;
    ff_OFarm<Mat> ofarm( [numOfWorkers,aplySobel]() {
            vector<unique_ptr<ff_node> > wrkrptrs; 
            for(size_t i=0; i<numOfWorkers; i++){ 
                wrkrptrs.push_back(make_unique<Worker>(aplySobel));
		}
            return wrkrptrs;
        } ());

    Collector collector(argv[2],CV_FOURCC('M','P','4','2'),S,fps/*,atoi(argv[4])*/);
    ofarm.setEmitterF(emitter);
    ofarm.setCollectorF(collector);
    
   // ffTime(START_TIME);
    if (ofarm.run_and_wait_end()<0) {
        cerr<<"runtime error, exiting!"<<endl;
        return -1;
    }
   // ffTime(STOP_TIME);
   // std::cout <<ffTime(GET_TIME) <<endl;    
    auto done = std::chrono::high_resolution_clock::now();

std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(done-started).count()<<endl;

    return 0;



}


#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/videoio.hpp"
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>

using namespace cv;
using namespace std;
using namespace ff;




struct Emitter : ff_node_t<Mat> {
    VideoCapture cap;
    Emitter(VideoCapture inputVideoPath):cap(inputVideoPath) {}
    Mat * svc(Mat *) {
	while(true) {
		Mat * frame = new Mat();
	    if(cap.read(*frame))
  		ff_send_out(frame);
	    else{
		cout << "end of video file" << endl;
		break;	
		}
	}
	cap.release();
	return EOS;
    }
}; 



struct Worker : ff_node_t<Mat> {
   	Mat * svc(Mat* frame) {
	bitwise_not(*frame, *frame);
	flip(*frame, *frame, 0);
	return frame;
  }
}; 


struct Collector: ff_node_t<Mat> {
VideoWriter vwr;
    Collector(string outputVideoPath,int ex, Size S,int fps) {
		vwr.open(outputVideoPath, ex, fps, S);
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
    if(argc != 4) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 2"<<endl<<"where 2 is the number of workers"<<endl; 
      return(-1); 
    }
    
	VideoCapture cap(argv[1]);

	if (!cap.isOpened()){
		cerr<< "Error, the video file was not opened!"<<endl;
		cap.release();
		return -1;
	}
	

    int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC)); 
    // Transform from int to char via Bitwise operators
    char EXT[] = { (char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16), (char)((ex & 0XFF000000) >> 24), 0 };
    Size S = Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH),(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));
    int fps=cap.get(CV_CAP_PROP_FPS);
    cout << "Input codec type: " << EXT << endl;
    cout << "Frame  width=" << S.width << " ,   height=" << S.height<< " number of frames: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
    int numOfWorkers = atol(argv[3]);
    ff_Pipe<> pipe(make_unique<Emitter>(cap));
    ff_OFarm<Mat> ofarm( [numOfWorkers]() {
            vector<unique_ptr<ff_node> > wrkrptrs; 
            for(size_t i=0; i<numOfWorkers; i++){ 
                wrkrptrs.push_back(make_unique<Worker>());
		}
            return wrkrptrs;
        } ());

    Collector collector(argv[2],ex,S,fps);
    ofarm.setCollectorF(collector);
    pipe.add_stage(ofarm);
    ffTime(START_TIME);
    if (pipe.run_and_wait_end()<0) {
        cerr<<"runtime error, exiting!"<<endl;
        return -1;
    }
    ffTime(STOP_TIME);
    std::cout <<"Run time"<< ffTime(GET_TIME) << " ms"<<endl;    
    return 0;



}


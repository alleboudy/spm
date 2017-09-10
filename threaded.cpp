#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/videoio.hpp"
#include <thread>
#include <future>
using namespace cv;
using namespace std;

Mat ProcessFrame(Mat frame)
{ 
	bitwise_not(frame, frame);
	flip(frame, frame, 0);
	return frame;
}

int main(int argc, char* argv[])
{
 if(argc != 4) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath 2 "<<" where 2 is the number of threads to use"<<endl;
      return(-1); 
    }
	
	//int bufferSize=atoi(argv[4]);
	VideoCapture cap(argv[1]);
	VideoWriter vwr;
	if (!cap.isOpened())
		throw "Error when reading video file";
	Mat frame;

	int ex = static_cast<int>(cap.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

	// Transform from int to char via Bitwise operators
	char EXT[] = { (char)(ex & 0XFF), (char)((ex & 0XFF00) >> 8), (char)((ex & 0XFF0000) >> 16), (char)((ex & 0XFF000000) >> 24), 0 };

	Size S = Size((int)cap.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
		(int)cap.get(CV_CAP_PROP_FRAME_HEIGHT));

	vwr.open(argv[2], ex, cap.get(CV_CAP_PROP_FPS), S);
	cout << "frame Width=" << S.width << ",  Height=" << S.height<< " number of frames: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	cout << "Input codec type: " << EXT << endl;
	if (!vwr.isOpened())
		throw "Error when opening the vide writer";

	//namedWindow("w", 1);
	vector<future<Mat>> workersPromises;
	int ctr=0;
	while (true)
	{
		Mat * frame = new Mat();
	    	if(cap.read(*frame)){
			workersPromises.push_back(future<Mat>( async(ProcessFrame, (*frame).clone())));
			cout<<++ctr<<endl;
			if(workersPromises.size()>=bufferSize){
				cout<<"flushing frames..."<<endl;
				for(size_t i;i<workersPromises.size();i++){
					vwr.write(workersPromises[i].get());
					cout<<"should be writing frame now"<<endl;
					//delete workersPromises[i];
				}
				workersPromises.clear();
			
			}		

		}
	    else{
		cout << "end of video file" << endl;
		break;	
		}

		delete frame;
		
	
	}
	if(workersPromises.size()>0){
		cout<<"Final Flush :D"<<endl;
		for(size_t i;i<workersPromises.size();i++){
				Mat result = workersPromises[i].get();
				vwr.write(result);
		}
		workersPromises.clear();
	}
	cout << "done" << endl;

	vwr.release();
	cap.release();

}


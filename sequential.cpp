#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/videoio.hpp"
using namespace cv;
using namespace std;

int main(int argc, char* argv[])
{
 if(argc != 3) {
      cout << "Invalid arguments"<<endl<< "Example usage: " << argv[0] << " inputVideoPath outputVideoPath "<<endl;
      return(-1); 
    }
	
	//reading the video
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
	cout << "Input frame resolution: Width=" << S.width << "  Height=" << S.height
		<< " of nr#: " << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;
	cout << "Input codec type: " << EXT << endl;
	if (!vwr.isOpened())
		throw "Error when opening the vide writer";

	//namedWindow("w", 1);

	while (true)
	{
		cap >> frame;
		if (frame.empty()){
			
		cout << "end of video" << endl;
		break;
			
		}
		Mat inverted(frame.rows, frame.cols, CV_8UC3);
		bitwise_not(frame, inverted);
		flip(inverted, inverted, 0);
		vwr.write(inverted);

		//imshow("w", inverted);
	//	waitKey(20); // waits to display frame
	}
	//waitKey(0); // key press to close window

	cout << "done" << endl;

	vwr.release();
	cap.release();

}


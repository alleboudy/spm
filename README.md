# This repo is my submission for the SPM exam of Unipi

http://didawiki.cli.di.unipi.it/doku.php/magistraleinformaticanetworking/spm/start

Using FastFlow to parallelize the application of a filter via OpenCV 3 on a video

parallel.cpp is the implementation using fastflow

sequential.cpp is a simplesequential implementation

threaded.cpp is the c++11 threading implementation

The current filter is just flipping the video frames and inverting their pixels intensities, but of course it is easy to modify this.


## How to compile
Will need to have OpenCV 3 installed and the fastflow root folder present on the machine 

In the project repository main folder run the build.sh file or type in a terminal the following:

g++ -I$FF_ROOT -I/usr/local/include -I/usr/local/include/opencv/ -I/usr/local/include/opencv2 -L/usr/lib/x86_64-linux-gnu/ -g -o run parallel.cpp -lopencv_videoio -lopencv_core -std=c++11 -pthread

Where $FF_ROOT is where the fastflow root folder

“run” is the name of the resulting compiled binary

“parallel.cpp” is the parallel implementation [could also do sequential.cpp or threaded.cpp instead]

other imports are for the openCV

## To run the compiled Binary type
#### For the parallel.cpp 
./run path/to/input/video/file.mp4 path/to/output/video/file.mp4 NumberOfWorkersForTheFarm SizeOfOutputBuffer

path/to/input/video/file.mp4 is where the input video resides on the desk

path/to/output/video/file.mp4 is where we wish to have the output video

NumberOfWorkersForTheFarm is the degree of parallelism of the farm

SizeOfOutputBuffer number of frames allowed in the output buffer before flushing the buffer

#### For the sequential.cpp
/run path/to/input/video/file.mp4 path/to/output/video/file.mp4 

#### For the threaded.cpp
./run path/to/input/video/file.mp4 path/to/output/video/file.mp4 NumberOfWorkers

path/to/input/video/file.mp4 is where the input video resides on the desk

path/to/output/video/file.mp4 is where we wish to have the output video

NumberOfWorkers is the number of threads to use



Thanks.


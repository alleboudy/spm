g++ -I$FF_ROOT -I/usr/local/include -I/usr/local/include/opencv/ -I/usr/local/include/opencv2 -L/usr/lib/x86_64-linux-gnu/ -g -o run parallel.cpp -lopencv_videoio -lopencv_core -std=c++11 -pthread


#!/bin/bash
for i in {1...25}
do
 ./threaded chaplin.mp4 athchap.avi $i sobel
done

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <boost/filesystem.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <stdio.h>
using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;

// MakeHistogram creates a histogram from the Mat data.
Mat MakeHistogram(Mat data, const int knumbins) {
    Mat histogram = Mat( knumbins, 1, CV_16UC1, Scalar(0) );
	
	for (int j = 0; j < data.rows; j++) { 
		// data must be greater than 0 and less than numbins
		if( data.at<int>(j,0) < knumbins ) {
			// count the number in that row by adding 1 to the histogram
			histogram.at<int>( data.at<int>(j,0),0 ) += 1;
		}
		else {
			perror("Not enough bins or negative values.\n"); 
	    }
	}
	
	return histogram;
}

// Loads data from YAML (.yml) files into the vector data
int LoadMat(vector<Mat> *data_vector, const path dir) {
	Mat temp;
	string file = dir.native();
	
	try {
		FileStorage fs( file, FileStorage::READ );
		fs["SIFT_membership"] >> temp;
		fs.release();
		data_vector->push_back(temp);
		return 1;//returns the number of files loaded.
	}
	catch (Exception& e) {
		cout << "Failed to read";
		return 0;
	}
}

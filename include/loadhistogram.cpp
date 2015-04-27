#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <boost/filesystem.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <stdio.h>
using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace boost::posix_time;

Mat histogram( Mat data, const int numbins )
//numbins > 0
{
	Mat histogram = Mat( numbins, 1, CV_16UC1, Scalar(0) );
	
	
	for( int j = 0; j < data.rows; j++ ) //for every row in that Mat
	{
		if( data.at<int>(j,0) < numbins )
		//data must be greater than 0 and less than numbins
		{
			histogram.at<int>( data.at<int>(j,0),0 ) += 1;
			//count the number in that row by adding 1 to the histogram
		}
		else
			perror("Not enough bins or negative values.\n"); 
		
	}
	
	return histogram;
}

int loadMat( vector<Mat> *data_vector, const path dir )
//loads data from YAML (.yml) files into the vector data
{
	Mat temp;
	string file = dir.native();
	
	try{
		FileStorage fs( file, FileStorage::READ );
		fs["SIFT_membership"] >> temp;
		fs.release();
		data_vector->push_back(temp);
		return 1;//returns the number of files loaded.
	}
	catch (Exception& e){
		cout << "Failed to read";
		return 0;
	}
} 

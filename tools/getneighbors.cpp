#ifndef __SEGMENT_HPP_INCLUDED__
#define __SEGMENT_HPP_INCLUDED__

#include "include/frame.hpp"

#include <opencv2/opencv.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;

int saveMat( const Mat data, const string name_field, const string name_file, path dir );
Mat loadMat( const string name_field, const path dir );


// This function will return a folder of images from argv[3] around frame argv[1] for width on either side.
// Last edit Sun 26 Apr 2015 07:29:24 PM PDT  
int main( int argc, char** argv )
{
	// Specify a frame and a neighborhood size
	int middle = std::strtof(argv[1], nullptr);
	int width = std::strtof(argv[2], nullptr);

	// Create an output directory named according to the middle 
	string m = std::to_string(middle);
	create_directory(path("./output/" + m));

	VideoCapture cap(argv[3]); // open the default camera
	if(!cap.isOpened())  // check if we succeeded
		return -1;

	Size fsize( cap.get(CV_CAP_PROP_FRAME_WIDTH ), cap.get(CV_CAP_PROP_FRAME_HEIGHT) );
	double fps = cap.get(CV_CAP_PROP_FPS);

	cout 	<< "MODE: " << cap.get(CV_CAP_PROP_MODE) << endl
		<< "FRAME RATE: " << fps << endl
		<< "FRAME SIZE: " << fsize << endl;
		 
	namedWindow("preview",1);

	vector<frame> frames;
	int videoEnd = cap.get(CV_CAP_PROP_FRAME_COUNT);

	int start = middle - width;
	int end = middle + width;
	for(int pos = start; pos < end; pos += 3)
	//sample the video 2 times a second
	{
		cap.set( CV_CAP_PROP_POS_FRAMES, pos );
		frame thisframe;
		thisframe.index = pos;

	if( cap.read(thisframe.img) ); // Returns false if no frame returned
	{	
		if(thisframe.img.rows > 0 && thisframe.img.cols > 0)
			imshow( "preview", thisframe.img );
		frames.push_back( thisframe );
	}
	
	if( waitKey(30) >= 0 ) break; // Check for ESC key abort
	
	// Save the frame to a file.
	string number = std::to_string(thisframe.index);		
	string name = "./output/" + m + "/image" + number + ".png";
	imwrite( name, thisframe.img ); 
	}
	
	cout << frames.size() << endl;

	waitKey(300);

	cout << "DONE." << endl;

	// the camera will be deinitialized automatically in VideoCapture destructor
	return 0;
}

#endif

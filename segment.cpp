/*The purpose of this program is to automatically divide videos into segments based on the content of their scenes.*/

#ifndef __SEGMENT_HPP_INCLUDED__
#define __SEGMENT_HPP_INCLUDED__

#include "include/frame.hpp"
#include "include/break.hpp"

#include <opencv2/opencv.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;

int SaveMat(const Mat data, const string name_field, const string name_file, path dir);
Mat LoadMat(const string name_field, const path dir);

int main(int argc, char** argv) {
  int tag = std::strtof(argv[1], nullptr);// Get a name for the output folder.
	string t = std::to_string(tag);
	create_directory(path("./output/" + t));

	VideoCapture cap(argv[3]);// Open the default camera
	if (!cap.isOpened()) return -1;// Check if we succeeded.
        
	Size fsize(cap.get(CV_CAP_PROP_FRAME_WIDTH), cap.get(CV_CAP_PROP_FRAME_HEIGHT));
	double fps = cap.get(CV_CAP_PROP_FPS);
	
	cout << "MODE: " << cap.get(CV_CAP_PROP_MODE) << endl
		   << "FRAME RATE: " << fps << endl
		   << "FRAME SIZE: " << fsize << endl;
	
	namedWindow("preview",1);
    
	vector<Frame> frames;// Make a vector to store the frames.
	int videoEnd = cap.get(CV_CAP_PROP_FRAME_COUNT);

	//TODO: Autocalculate 2fps from frame rate.
	for (int pos = 0; pos < videoEnd; pos += 15) {
	// Sample the video 2 times a second 1:15 ratio for 30fps. 
		cap.set( CV_CAP_PROP_POS_FRAMES, pos );
		
		Frame thisframe;
		thisframe.index = pos;

	if ( cap.read(thisframe.img) ) {// Checks that the Frame is not NULL.	
		if ( thisframe.img.rows > 0 && thisframe.img.cols > 0 )// Preview the frame if it is non-zero in size.
			imshow("preview", thisframe.img);
		frames.push_back(thisframe);
	}
	  if( waitKey(30) >= 0 ) break;// Check for abort recording via ESC keypress. 
	}
	
	cout << frames.size() << endl;// Display the number of frames captured.

	waitKey(300); // This is here to make the popup window appear?

  // Create a dictionary from sample

  Mat dictionary;

  // If a ditionary doesn't already exist then make one.
  if( !exists(path("./output/dictionary.yaml")) ) {
    cout << "Creating dicitonary..." << endl;
    CreateDictionary( &frames, &dictionary, "SIFT", "SIFT", true );
    SaveMat( dictionary, "dictionary", "dictionary.yaml", path("./output/") );
  } else {
    cout << "Loading dicitonary..." << endl;
    CreateDictionary( &frames, &dictionary, "SIFT", "SIFT", false );
    dictionary = LoadMat( "dictionary", path("./output/dictionary.yaml") );
  }

  //cout << "Culling..." << endl;
  //cull( &frames, 2 );

  initModule_nonfree();
  Ptr<DescriptorExtractor> dextractor = DescriptorExtractor::create("SIFT");
  Ptr<DescriptorMatcher> dmatcher = DescriptorMatcher::create("FlannBased");

  BOWImgDescriptorExtractor Biggie(dextractor, dmatcher);
  Biggie.setVocabulary(dictionary);

  cout << "Computing BOWdescriptors" << endl;
  for (int i = 0; i < frames.size(); i++) {
  // Compute BOW descritor for each of the frames.
	  Biggie.compute(frames.at(i).img, frames.at(i).keypoints,
                   frames.at(i).bag, &frames.at(i).rawhist, &frames.at(i).descriptors );
	  frames.at(i).bag = MakeHistogram(frames.at(i));
  }

  cout << "Searching for breaks..." << endl;
  vector<int> breaks = SearchForBreaks(&cap, &frames, &Biggie, CV_COMP_CORREL, std::strtof(argv[2], nullptr)); 

  ///SaveMat( bag, "BOW", "TestVideo.yaml", path("./") );

  cout << "Saving breaks..." << endl;
  Mat break_locations = Mat( breaks, false);
  break_locations.convertTo( break_locations, CV_32F, 1/fps, 0 );
  SaveMat( break_locations, "Scene Changes", "TestVideo.yaml", path("./output/") );
  cout << "DONE." << endl;

  // the camera will be deinitialized automatically in VideoCapture destructor
  return 0;
}

//saves data to a YAML file with at directory/name_file
int SaveMat( const Mat data, const string name_field, const string name_file, path dir ) {
	if( !exists(dir) ) {
		create_directory(dir);
	}
	
	dir = path(dir.native() + name_file);
	
	try {
		if( !exists(dir) ) {
			FileStorage fs(	dir.native(), FileStorage::WRITE );
			fs << name_field << data;
			fs.release();
		} else {
			FileStorage fs(	dir.native(), FileStorage::APPEND );
			fs << name_field << data;
			fs.release();
		}
	} catch (Exception& e) {
		printf("Failed to write %s\n", dir.c_str());
		return 1;
	}
	return 0;
}

//loads data from a YAML file at directory/name_file
Mat LoadMat( const string name_field, const path dir ) {
	string file_dir = dir.native();
	Mat data;
	
	try {
		FileStorage fs(file_dir, FileStorage::READ);
		fs[name_field] >> data;
		fs.release();
		return data;
	} catch (Exception& e) {
		cout << "Failed to read";
		return data;
	}
}

#endif

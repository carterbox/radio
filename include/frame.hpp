#ifndef __FRAME_HPP_INCLUDED__
#define __FRAME_HPP_INCLUDED__

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <boost/thread.hpp>

using namespace cv;
using namespace std;

struct Frame{
	int index = 0;
	Mat img;
	bool newscene = false;
	vector<KeyPoint> keypoints;
	Mat bag;
	vector<vector<int>> rawhist;
	Mat descriptors;
};

// Reduces the size of a Fame vector by 1:ratio.
void Cull (vector<Frame>* frames, int ratio) {
	vector<Frame> temp;
	for ( int i = 0; i < frames->size(); i+=ratio )
		temp.push_back(frames->at(i));
  frames->resize(0);
	*frames = temp;
	return;
}

/* Applies the given FeatureDetector and Feature Extractor to the given vector of Frames start at with start and skipping frames according to the stride */
void GetPoints( int start, int stride, vector<Frame>* F, Ptr<DescriptorExtractor> describer, Ptr<FeatureDetector> detector ) {
	for (int i = start; i < F->size(); i += stride) {
		printf("Thread %i: Detecting features...    ", start);
		detector->detect(F->at(i).img, F->at(i).keypoints);
		printf("FOUND %lu FEATURES\n", F->at(i).keypoints.size());

		//printf("Computing descriptors...  ");
		describer->compute(F->at(i).img, F->at(i).keypoints, F->at(i).descriptors);
		//printf("COMPUTED.\n");
	}
	return;
}

/* Creates a dictionary from a vector of frames using the specified detector and descriptor. If new_dic is false, then it still extracts features from the frames but does not run a clustering algorithm to create the dictionary. This function is parallelized with the boost library. */ 
void CreateDictionary( vector<Frame>* frames, Mat* dictionary, const string kdetector_type, const string kdescriptor_type, bool new_dic )
{
	initModule_nonfree();
	
	Ptr<DescriptorExtractor> describer = DescriptorExtractor::create(kdescriptor_type);
	Ptr<FeatureDetector> detector = FeatureDetector::create(kdetector_type);
	
	TermCriteria criteria( CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 100, 0.001 );
	int dictionary_size = 300;
  int retries = 3;
  int flags = KMEANS_PP_CENTERS;
	BOWKMeansTrainer Bot( dictionary_size, criteria, retries, KMEANS_PP_CENTERS );
	
	cout << "Made descriptors" << endl;
	
	const int kmax_threads = boost::thread::hardware_concurrency() - 2;
	CV_Assert( kmax_threads > 0 );
	boost::thread_group workers;
	
	for (int i = 0; i < kmax_threads; i++) {
		workers.create_thread(boost::bind(&GetPoints, i, kmax_threads, frames, describer, detector)); 
	}
	workers.join_all();
	
	for (int i = 0; i < frames->size(); i++)	{
		if ( frames->at(i).keypoints.size() > 0 )
			Bot.add(frames->at(i).descriptors);
	}
	
	if ( new_dic ) {
		cout << "Constructing vocab from " << Bot.descripotorsCount() << " descriptors." << endl;
		*dictionary = Bot.cluster();
	}
	
	return;
}
#endif

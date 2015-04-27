#ifndef __FRAME_HPP_INCLUDED__
#define __FRAME_HPP_INCLUDED__

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <boost/thread.hpp>

using namespace cv;
using namespace std;

struct frame{
	int index = 0;
	Mat img;
	bool newScene = false;
	vector<KeyPoint> keypoints;
	Mat bag;
	vector<vector<int>> rawhist;
	Mat descriptors;
};

void cull( vector<frame>* frames, int oneOutOf )
//reduces the size of frames by 1:oneOutof
{
	vector<frame> temp;
	for( int i = 0; i < frames->size(); i+=oneOutOf )
		temp.push_back(frames->at(i));
	
	frames->resize(0);
	//delete frames; //?? I'm trying to prevent memory leaks
	*frames = temp;
	return;
}

void getPoints( int start, int stride, vector<frame>* F, Ptr<DescriptorExtractor> describer, Ptr<FeatureDetector> detector )
//extracts features and decriptors from a frame
{
	for(int i = start; i < F->size(); i += stride)
	{
		printf("Thread %i: Detecting features...    ", start);
		detector->detect( F->at(i).img, F->at(i).keypoints );
		printf("FOUND %lu FEATURES\n", F->at(i).keypoints.size());

		//printf("Computing descriptors...  ");
		describer->compute( F->at(i).img, F->at(i).keypoints, F->at(i).descriptors );
		//printf("COMPUTED.\n");
	}
	return;
}

void createDictionary( vector<frame>* frames, Mat* dictionary,
					   const string fdetector, const string fdescriptor, bool newDic )
//creates a dictionary from a vector of frames
{
	initModule_nonfree();
	
	Ptr<DescriptorExtractor> describer = DescriptorExtractor::create(fdescriptor);
	Ptr<FeatureDetector> detector = FeatureDetector::create(fdetector);
	
	TermCriteria criteria( CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 100, 0.001 );
    int dictionarySize = 300;
    int retries = 3;
    int flags = KMEANS_PP_CENTERS;
	BOWKMeansTrainer Bot( dictionarySize, criteria, retries, KMEANS_PP_CENTERS );
	
	cout << "Made descriptors" << endl;
	
	int maxThreads = boost::thread::hardware_concurrency() - 2;
	CV_Assert( maxThreads > 0 );
	boost::thread_group workers;
	
	for(int i = 0; i < maxThreads; i++)
	{
		workers.create_thread( boost::bind(&getPoints, i, maxThreads, frames, describer, detector)); 
	}
	workers.join_all();
	
	for(int i = 0; i < frames->size(); i++)
	{
		if( frames->at(i).keypoints.size() > 0 )
			Bot.add(frames->at(i).descriptors);
	}
	
	if (newDic)
	{
		cout << "Constructing vocab from " << Bot.descripotorsCount() << " descriptors." << endl;
		*dictionary = Bot.cluster();
	}

	
	return;
}
#endif

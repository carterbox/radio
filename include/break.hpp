#ifndef __BREAK_HPP_INCLUDED__
#define __BREAK_HPP_INCLUDED__

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <stack>
#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;

struct Frame;

int SaveMat( const Mat data, const string name_field, const string name_file, path dir );

// Converts the raw data into a histogram that compareHist likes
Mat MakeHistogram( Frame F ) {
	Mat bag = Mat::zeros(1, 300, CV_32FC1);
	//cout << "rawhist size: " << F.rawhist.size() << endl;
	// If there were no points detected, return the zero histogram.	
	if ( F.rawhist.size() == 0 ) {
		cout << "returned zero histogram" << endl;
		return bag;
	} else {
		// Assume no detection points have an empty vector.
		for (int i = 0; i < F.rawhist.size(); i++) {
			bag.at<float>(i) = F.rawhist.at(i).size();
		}
	}
}

// Return true if the criteria are met for the frames to be in the same scene.
// TODO: Make less than threshold comparison if greater_than == false.
bool SameScene(Frame A, Frame B, const int method, bool greater_than, const double threshold ) {
	CV_Assert(A.bag.isContinuous());
	CV_Assert(B.bag.isContinuous());

	double result = cv::compareHist(A.bag, B.bag, method);	
	printf(" Comparison of %i and %i : %f \n", A.index, B.index, result);
	if ( result > threshold ) {
		return true;
	} else {
		return false;
	}
}

void SearchWorker( boost::mutex* safevector, VideoCapture* video, vector<int>* breaks,
                   BOWImgDescriptorExtractor* Biggie, stack<Frame>* todo, const int method, const double threshold ) {
	int insize = todo->size();
	cout << insize << " Size in" << endl;
	//assert(breaks->empty()); //not empty because threading	
	assert(todo->size() > 2);
	Frame A = todo->top(); todo->pop();
	A.newscene = true;
	
	safevector->lock();
	breaks->push_back(A.index);
	safevector->unlock();
	
	Ptr<DescriptorExtractor> describer = DescriptorExtractor::create("SIFT");
	Ptr<FeatureDetector> detector = FeatureDetector::create("SIFT");
	
	//determine if A and B are in the same scene
	while ( !todo->empty() ) {
		Frame B = todo->top(); todo->pop();
		
		// Update progress bar. Compare B with next  Frame. 
		// TODO: make a progress bar
		
		if ( SameScene(A, B, method, true, threshold) ) {
			A = B;
		} else { //spawn a new node three frames after A
			int newnode = (A.index + B.index)/2;
			cout << "Spawned new node at " << newnode << endl;
			if ( newnode == A.index ) {
			// There is no new node. Mark B as the start of a new scene. Compare B with next  Frame.
				B.newscene = true;
				safevector->lock();
				breaks->push_back(B.index);
				safevector->unlock();				
				A = B;
			} else { // There is a new node. Push B onto the stack. Push the new node onto the stack. Compare A with the new node.
				todo->push(B);
				
				video->set( CV_CAP_PROP_POS_FRAMES, newnode );
				Frame newframe;
        newframe.index = newnode;
        video->read(newframe.img);
        		
        detector->detect(newframe.img, newframe.keypoints);
        describer->compute(newframe.img, newframe.keypoints, newframe.descriptors);
        		
    		Biggie->compute(newframe.img, newframe.keypoints, newframe.bag, &newframe.rawhist, &newframe.descriptors);
        newframe.bag = MakeHistogram(newframe);
     		todo->push(newframe);
			}
		}
		if( waitKey(3) >= 0 ) break;
	}
	
	assert( todo->empty() );
	//cout << endl << "FOUND " << breaks->size() << " BREAKS." << endl;
	//assert( breaks->size() < insize );
	//The number of frames should be larger or equal to what is was before.
	
	return;
}

vector<int> SearchForBreaks(VideoCapture* video, vector<Frame>* frames,
						  BOWImgDescriptorExtractor* Biggie,
							const int method, const double threshold ) {
	vector<int> breaks;
	vector< stack<Frame> > todo;
	
	cout << "Making TODO list..." << endl;
	//put all the frames into todo;
	
	const int kmax_threads = 1;//boost::thread::hardware_concurrency() - 2;
	CV_Assert( kmax_threads > 0 );	

	int size = frames->size();		
	int subsize = size/kmax_threads;
	for (int i = 0; i < kmax_threads; i++ ) {
		stack<Frame> temp;        
		for (int j = i*subsize; j < (i+1)*subsize; j++ ) {				
			temp.push(frames->back());
			frames->pop_back();
		}
		todo.push_back(temp);
	}
	
	if (false) {//!frames->empty())
		stack<Frame> temp;
		while ( !frames->empty() ) { // push any division remainders	
			temp.push(frames->back());
			frames->pop_back();
		}
		todo.push_back(temp);
	}	
	
	boost::thread_group workers;
	
	cout << "Searching..." << endl;
		
	boost::mutex safevector;
	for (int i = 0; i < todo.size(); i++) {
		workers.create_thread(boost::bind(&SearchWorker, &safevector, video, &breaks, Biggie, &todo.at(i), method, threshold)); 
	}
	workers.join_all();

//TODO: Make method for better merging of breaks by the threads	
	
return breaks;
}

#endif

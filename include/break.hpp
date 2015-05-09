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

int SaveMat(const Mat data, const string name_field, const string name_file, path dir);

// Converts the raw data into a histogram that cv::compareHist likes
Mat MakeHistogram(Frame F) {
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

void SearchWorker(boost::mutex* safevector, VideoCapture* video,
                  vector<int>* breaks, BOWImgDescriptorExtractor* Biggie,
                  stack<Frame>* todo, const int method, const double threshold ) {
    int insize = todo->size(); // Note the size of frames to be processed
    cout << insize << " size in" << endl;
    assert(todo->size() > 2);
    
    // Mark the start of the todo list as a new scene.
    Frame A = todo->top(); todo->pop();
    A.newscene = true;
	
	// Keep the list of scene breaks safe when recording this scene break.
    safevector->lock();
    breaks->push_back(A.index);
    safevector->unlock();
	
    Ptr<DescriptorExtractor> describer = DescriptorExtractor::create("SIFT");
    Ptr<FeatureDetector> detector = FeatureDetector::create("SIFT");
	
	// Determine if A and B are in the same scene.
	while ( !todo->empty() ) {
		Frame B = todo->top(); todo->pop();
		
		// Update progress bar. Compare B with next Frame. 
		// TODO: make a progress bar
		
		// If A and B are in the same scene, then move forward.
		if ( SameScene(A, B, method, true, threshold) ) {
			A = B;
		// Else try to spawn a new node between A and B. 
		} else { 
			int newnode = (A.index + B.index)/2;
			cout << "Spawned new node at " << newnode << endl;
			if ( newnode == A.index ) {
			// There is no node. Mark B as the start of a new scene. Compare B with next Frame.
				B.newscene = true;
				// Stay safe when adding this break to the list.
				safevector->lock();
				breaks->push_back(B.index);
				safevector->unlock();				
				A = B;
			} else { // New node. Push B onto the stack.
			         // Push the new node onto the stack.
			         // Compare A with the new node.
				todo->push(B);
				
				// Get the image data for the new node.
				video->set( CV_CAP_PROP_POS_FRAMES, newnode );
				Frame newframe;
                newframe.index = newnode;
                video->read(newframe.img);
        		// Calculate the keypoints for the new node.
                detector->detect(newframe.img, newframe.keypoints);
                describer->compute(newframe.img, newframe.keypoints, newframe.descriptors);
        		// Create a BOW histogram for the new node.
    		    Biggie->compute(newframe.img, newframe.keypoints, newframe.bag,
    		                    &newframe.rawhist, &newframe.descriptors);
                newframe.bag = MakeHistogram(newframe);
     		    todo->push(newframe);
	        }
		}
		if( waitKey(3) >= 0 ) break;
	}
	assert( todo->empty() );
	//cout << endl << "FOUND " << breaks->size() << " BREAKS." << endl;
	return;
}

vector<int> SearchForBreaks(VideoCapture* video, vector<Frame>* frames,
						    BOWImgDescriptorExtractor* Biggie,
							const int method, const double threshold ) {
	vector<int> breaks; // For storing locations of identified breaks.
	vector< stack<Frame> > todo; // For keeping track of what still need to be searched.
	
	cout << "Making TODO list..." << endl;
	// Put all the frames into todo.
	
	const int kmax_threads = 1;//boost::thread::hardware_concurrency() - 2;
	assert( kmax_threads > 0 );

    // Divide the work into smaller chunks.
	int size = frames->size();
	int subsize = size/kmax_threads;
	for (int i = 0; i < kmax_threads; i++) {
        stack<Frame> temp;
        for (int j = i*subsize; j < (i+1)*subsize; j++) {				
		    temp.push(frames->back());
		    frames->pop_back();
		}
		todo.push_back(temp);
	}
	
	// Check for and push any division remainders.
	if (false) {//!frames->empty())
		stack<Frame> temp;
		while ( !frames->empty() ) {	
			temp.push(frames->back());
			frames->pop_back();
		}
		todo.push_back(temp);
	}	
	
	// Spawn some threads using Boost and have them each search a subset.
	boost::thread_group workers;
	cout << "Searching..." << endl;
	boost::mutex safevector; // Mutex for keeping &breaks safe.
	for (int i = 0; i < todo.size(); i++) {
		workers.create_thread(boost::bind(&SearchWorker, &safevector, video,
		&breaks, Biggie, &todo.at(i), method, threshold));
	}
	workers.join_all();

//TODO: Make method for better merging of breaks by the threads	
	
return breaks;
}

#endif

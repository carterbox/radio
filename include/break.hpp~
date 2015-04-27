#ifndef __BREAK_HPP_INCLUDED__
#define __BREAK_HPP_INCLUDED__

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <stack>
#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;

struct frame;

int saveMat( const Mat data, const string name_field, const string name_file, path dir );

Mat makeHistogram( frame F )
//converts the raw data into a histogram that compareHist likes
{
	Mat bag = Mat::zeros(1, 300, CV_32FC1);
	
	//cout << "rawhist size: " << F.rawhist.size() << endl;
	if( F.rawhist.size() == 0 )
	{
	//If there were no points detected, return the zero histogram.	
		cout << "returned zero histogram" << endl;
		return bag;
	}
	else
	{
		for(int i = 0; i < F.rawhist.size(); i++)
		//Assume no detection poitns have an empty vector.
		{
			bag.at<float>(i) = F.rawhist.at(i).size();
		}	
	}
}

bool sameScene( frame A, frame B, const int method, bool greaterThan, const double threshold )
//Return true if the criteria are met for the frames to be in the same scene.
//TODO: Make less than threshold comparison if greaterThan == false.
{
	CV_Assert(A.bag.isContinuous());
	CV_Assert(B.bag.isContinuous());

	double result = compareHist(A.bag, B.bag, method);	
	printf(" Comparison of %i and %i : %f \n", A.index, B.index, result);
	if( result > threshold )
	{
		return true;
	}
	else
		return false;
}

void searchWorker( boost::mutex* safevector, VideoCapture* video, vector<int>* breaks,
			  BOWImgDescriptorExtractor* Biggie,
			  stack<frame>* todo, const int method, const double threshold )
{
	int insize = todo->size();
	cout << insize << " Size in" << endl;
	//assert(breaks->empty()); //not empty because threading	
	assert(todo->size() > 2);
	frame A = todo->top(); todo->pop();
	A.newScene = true;
	
	safevector->lock();
	breaks->push_back(A.index);
	safevector->unlock();
	
	Ptr<DescriptorExtractor> describer = DescriptorExtractor::create("SIFT");
	Ptr<FeatureDetector> detector = FeatureDetector::create("SIFT");
	
	while(!todo->empty())
	//determine if A and B are in the same scene
	{
		frame B = todo->top(); todo->pop();
		
		if(sameScene( A, B, method, true, threshold ))
		//Update progress bar. Compare B with next frame. 
		//TODO: make a progress bar
		{
			A = B;
		}
		else
		//spawn a new node three frames after A
		{
			int newNode = (A.index + B.index)/2;
			cout << "Spawned new node at " << newNode << endl;
			if( newNode == A.index )
			//There is no new node. Mark B as the start of a new scene. Compare
			//B with next frame.
			{
				B.newScene = true;
				safevector->lock();
				breaks->push_back(B.index);
				safevector->unlock();				
				A = B;
			}
			else
			//There is a new node. Push B onto the stack. Push the new node onto
			//the stack. Compare A with the new node.
			{
				todo->push(B);
				
				video->set( CV_CAP_PROP_POS_FRAMES, newNode );
				frame newframe;
        		newframe.index = newNode;
        		video->read(newframe.img);
        		
        		detector->detect( newframe.img, newframe.keypoints );
        		describer->compute( newframe.img, newframe.keypoints, newframe.descriptors );
        		
    			Biggie->compute( newframe.img, newframe.keypoints,
    					newframe.bag, &newframe.rawhist, &newframe.descriptors );
        		newframe.bag = makeHistogram( newframe );
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

vector<int> searchForBreaks( VideoCapture* video, vector<frame>* frames,
							BOWImgDescriptorExtractor* Biggie,
							const int method, const double threshold )
{
			
	vector<int> breaks;
	vector< stack<frame> > todo;
	
	cout << "Making TODO list..." << endl;
	//put all the frames into todo;
	
	int maxThreads = 1;//boost::thread::hardware_concurrency() - 2;
	CV_Assert( maxThreads > 0 );	

	int size = frames->size();		
	int subsize = size/maxThreads;
	for(int i = 0; i < maxThreads; i++ )
	{
		stack<frame> temp;        
		for(int j = i*subsize; j < (i+1)*subsize; j++ )		
		{				
			temp.push(frames->back());
			frames->pop_back();
		}
		todo.push_back(temp);
	}
	
	if(false)//!frames->empty())
	{
		stack<frame> temp;
		while(!frames->empty())
		//push any division remainders	
		{
			temp.push(frames->back());
			frames->pop_back();
		}
		todo.push_back(temp);
	}	
	
	boost::thread_group workers;
	
	cout << "Searching..." << endl;
		
	boost::mutex safevector;
	for(int i = 0; i < todo.size(); i++)
	{
		workers.create_thread( boost::bind(&searchWorker, &safevector, video, &breaks, Biggie, &todo.at(i), method, threshold )); 
	}
	workers.join_all();

//TODO: Make method for better merging of breaks by the threads	
	
return breaks;
}



#endif

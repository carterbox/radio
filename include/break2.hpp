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
	
	if( F.rawhist.size() == 0 )
	//If there were no points detected, return the zero histogram.	
		return bag;
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
	Mat Abag = makeHistogram( A );
	Mat Bbag = makeHistogram( B );
	
	//cout << "before assertions" << endl;
	
	CV_Assert(Abag.isContinuous());
	CV_Assert(Bbag.isContinuous());
		
	//cout << "after assertions" << endl;	
	double result = compareHist(Abag, Bbag, method);	
	cout << result << endl;
	if( result > threshold )
	{
		return true;
	}
	else
		return false;
}

void searchWorker( VideoCapture* video, vector<frame>* frames,
			  BOWImgDescriptorExtractor* Biggie,
			  stack<frame>* todo, const int method, const double threshold )
{
	int insize = todo->size();
	cout << insize << " Size in" << endl;
	assert(frames->empty()); assert(todo->size() > 2);
	frame A = todo->top(); todo->pop();
	A.newScene = true;
	
	while(!todo->empty())
	//determine if A and B are in the same scene
	{
		frame B = todo->top(); todo->pop();
		
		if(sameScene( A, B, method, true, threshold ))
		//Update progress bar. Compare B with next frame. 
		//TODO: make a progress bar
		{
			frames->push_back(A);
			A = B;
		}
		else
		//spawn a new node half way between frame A and B
		{
			int newNode = (A.index + B.index)/2; 
			if( newNode = A.index )
			//There is no new node. Mark B as the start of a new scene. Compare
			//B with next frame.
			{
				B.newScene = true;
				frames->push_back(A);
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
        		
        		vector<vector<int>> noThanks;
    			Biggie->compute( newframe.img, newframe.keypoints,
    					newframe.bag, &noThanks, &newframe.descriptors );
        		
        		todo->push(newframe);
			}
		}
	}
	frames->push_back(A);
	
	assert( todo->empty() );
	cout << frames->size() << " Size out" << endl;
	assert( insize <= frames->size() );
	//The number of frames should be larger or equal to what is was before.
	
	return;
}

vector<int> searchForBreaks( VideoCapture* video, vector<frame>* frames,
							BOWImgDescriptorExtractor* Biggie,
							const int method, const double threshold )
{
			
	vector<int> breaks;
	stack<frame> todo;
	
	cout << "Making TODO list..." << endl;
	//put all the frames into todo;
	while(!frames->empty())
	{
		todo.push(frames->back());
		frames->pop_back();
	}
	
	cout << "Searching..." << endl;
	searchWorker( video, frames, Biggie, &todo, method, threshold );
	
	cout << "tallying scene breaks..." << endl;
	for(int i = 0; i < frames->size(); i++)
	//go through the new frames list and find all the new scene locations
	{
		if(frames->at(i).newScene)
			breaks.push_back(frames->at(i).index);
	}
	
return breaks;
}



#endif

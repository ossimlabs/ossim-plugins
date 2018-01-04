//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "Descriptor.h"
#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/xfeatures2d.hpp>

using namespace cv;
namespace ATP
{
std::vector<KeyPoint> Descriptor::akaze(const Mat& img, Mat& des){

	std::vector<KeyPoint> retVal;

    Ptr<AKAZE> detector = AKAZE::create();
    detector->detectAndCompute(img, noArray(), retVal, des);

return retVal;
}



std::vector<KeyPoint> Descriptor::kaze(const Mat& img, Mat& des){
    
	std::vector<KeyPoint> retVal;

    Ptr<KAZE> detector = KAZE::create();
    detector->detectAndCompute(img, noArray(), retVal, des);

return retVal;
}



std::vector<KeyPoint> Descriptor::orb(const Mat& img, Mat& des){
    
	std::vector<KeyPoint> retVal;

    Ptr<ORB> detector = ORB::create();
    detector->detectAndCompute(img, noArray(), retVal, des);

return retVal;
}



std::vector<KeyPoint> Descriptor::sift(const Mat& img, Mat& des){
    
	std::vector<KeyPoint> retVal;

    Ptr<xfeatures2d::SIFT> detector = xfeatures2d::SIFT::create();
    detector->detectAndCompute(img, noArray(), retVal, des);

return retVal;
}



std::vector<KeyPoint> Descriptor::surf(const Mat& img, Mat& des){
    
	std::vector<KeyPoint> retVal;

    Ptr<xfeatures2d::SURF> detector = xfeatures2d::SURF::create();
    detector->detectAndCompute(img, noArray(), retVal, des);

return retVal;
}
}


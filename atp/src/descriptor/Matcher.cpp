//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "Matcher.h"
#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

namespace ATP
{
std::vector< std::vector<cv::DMatch> > Matcher::flann(const cv::Mat& desA, const cv::Mat& desB, int k){
    std::vector< std::vector<DMatch> > retVal;

    if(desA.empty() || desB.empty()) return retVal; 

    if(desA.type()!=CV_32F) {
        desA.convertTo(desA, CV_32F);
    }

    if(desB.type()!=CV_32F) {
        desB.convertTo(desB, CV_32F);
    }

    FlannBasedMatcher matcher;
    matcher.knnMatch(desA, desB, retVal, k);

return retVal;
}



std::vector< std::vector<cv::DMatch> > Matcher::bruteForce(const cv::Mat& desA, const cv::Mat& desB, int k, int errorFunc){
    std::vector< std::vector<DMatch> > retVal;

    if(desA.empty() || desB.empty()) return retVal; 

    BFMatcher matcher(errorFunc, true);
    matcher.knnMatch(desA, desB, retVal, k);

return retVal;
}
}

//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef MATCHER_H
#define MATCHER_H

#include <ossim/base/ossimConstants.h>
#include <opencv/cv.h>

namespace ATP
{
class OSSIMDLLEXPORT Matcher{
public:
	std::vector< std::vector<cv::DMatch> > flann(const cv::Mat& desA, const cv::Mat& desB, int k);
	std::vector< std::vector<cv::DMatch> > bruteForce(const cv::Mat& desA, const cv::Mat& desB, int k, int errorFunc);
};

enum class MatcherType{FLANN, BF};
}

#endif // MATCHER_H

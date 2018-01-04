//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <ossim/base/ossimConstants.h>
#include <opencv/cv.h>

namespace ATP
{
class OSSIMDLLEXPORT Descriptor{
public:
	std::vector<cv::KeyPoint> akaze(const cv::Mat& image, cv::Mat& descriptor);
	std::vector<cv::KeyPoint> kaze(const cv::Mat& image, cv::Mat& descriptor);
	std::vector<cv::KeyPoint> orb(const cv::Mat& image, cv::Mat& descriptor);
	std::vector<cv::KeyPoint> sift(const cv::Mat& image, cv::Mat& descriptor);
	std::vector<cv::KeyPoint> surf(const cv::Mat& image, cv::Mat& descriptor);
};

enum class DescriptorType{AKAZE, KAZE, ORB, SIFT, SURF};
}

#endif // DESCRIPTOR_H

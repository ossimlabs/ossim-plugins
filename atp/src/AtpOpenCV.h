//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef AtpOpenCV_HEADER
#define AtpOpenCV_HEADER

#include <ossim/imaging/ossimImageData.h>
#include <opencv/cv.h>
#include <memory>

class ossimImageData;

//! THESE FUNCTIONS REQUIRE OPENCV

namespace ATP
{
//! Converts an ossimImageData pointer to an IplImage for use in OpenCV.
//! Warning: This function allocates memory, all non-null pointers should be
//! free'd using cvReleaseImage.
IplImage *convertToIpl(const ossimImageData* data);
IplImage *convertToIpl32(const ossimImageData* data);

//! Converts an IPL image to an ossimImageData object:
void copyIplToOid(IplImage* ipl, ossimImageData* oid);
void copyIpl32ToOid(IplImage* ipl, ossimImageData* oid);

}
#endif

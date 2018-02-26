//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include <ossim/imaging/ossimImageData.h>
#include "AtpOpenCV.h"
#include "../AtpCommon.h"

using namespace cv;

namespace ATP
{
#if 1
   #define IPL_PIXEL_DEPTH IPL_DEPTH_8U
   #define PIXEL_TYPE  ossim_uint8
   #define NUM_BYTES_PER_PIXEL 1
#else
   #define IPL_PIXEL_DEPTH IPL_DEPTH_16U
   #define PIXEL_TYPE  ossim_uint16
   #define NUM_BYTES_PER_PIXEL 2
#endif

//*************************************************************************************************
//! Converts an ossimImageData pointer to an IplImage for use in OpenCV.
//! Warning: This function allocates memory, all non-null pointers should be
//! free'd using cvReleaseImage.
//*************************************************************************************************
IplImage *convertToIpl(const ossimImageData* data)
{
   IplImage *ret=NULL;
   int numbytes;

   if(data==NULL)
   {
      CWARN<<"convertToIpl::Invalid data ptr."<<endl;;
      return ret;
   }

   //const unsigned char *dataptr = data->getUcharBuf();
   const PIXEL_TYPE *dataptr = (PIXEL_TYPE*) data->getBuf();
   if (!dataptr)
   {
      CFATAL<<"convertToIpl::ERROR getting the image data! bands/depth not supported"<<endl;
      return ret;
   }

   unsigned int numBands = data->getNumberOfBands();
   numbytes = NUM_BYTES_PER_PIXEL;
   if(numBands==1)
   {
      //numbytes=1;
      //numbytes=2;
   } 
   else 
   {
      // TODO COLOR IMAGES DON"T WORK RIGHT NOW
      numbytes=3;
      // Going to use the function (CvtPlaneToPix(IplImage*src0,src1,src2,src3,dst))
      CFATAL<<"convertToIpl -- ERROR: multiband feature detection not implemented yet!"<<endl;
      return ret;
   } 

   ret = cvCreateImage(cvSize(data->getWidth(), data->getHeight()),IPL_PIXEL_DEPTH, (int)numBands);
   if(ret) 
   {
      // <COPY IMAGE INTO IPLIMAGE STRUCTURE IN ALIGNED FORMAT>
      int lines=data->getHeight();
      for(int y=0;y<lines;y++)
      {
         memcpy(&ret->imageData[y*ret->widthStep],&dataptr[y*ret->width],ret->width*numbytes);
      }
   } 
   else 
   {
      CFATAL<<"convertToIpl::ERROR creating IplImage."<<endl;
   }
   return ret;
}

IplImage *convertToIpl32(const ossimImageData* data)
{
   IplImage *indata,*ret32bit;
   CvSize dim;
   indata=convertToIpl(data);
   dim.height=indata->height;
   dim.width=indata->width;
   ret32bit=cvCreateImage(dim,IPL_DEPTH_32F,indata->nChannels);
   cvConvert(indata,ret32bit);
   cvReleaseImage(&indata);
   return ret32bit;
}

//*************************************************************************************************
//! Converts an IPL 32-bit image to an ossimImageData object:
//*************************************************************************************************
void copyIpl32ToOid(IplImage* ipl, ossimImageData* oid)
{
   //IplImage *tmp8bit;
   IplImage *indata;
   CvSize dim;
   dim.height=ipl->height;
   dim.width=ipl->width;
   indata=cvCreateImage(dim,IPL_PIXEL_DEPTH,ipl->nChannels);
   cvConvert(ipl,indata);
   copyIplToOid(indata, oid);
}

//*************************************************************************************************
//! Converts an IPL X-bit image to an ossimImageData object:
//*************************************************************************************************
void copyIplToOid(IplImage* ipl, ossimImageData* oid)
{
   const PIXEL_TYPE *iplptr = (const PIXEL_TYPE*) ipl->imageData;
   PIXEL_TYPE* oidptr = (PIXEL_TYPE*)oid->getBuf();
   int numbytes = NUM_BYTES_PER_PIXEL; // num bytes per sample
   for(unsigned int y=0;y<oid->getHeight();y++) 
   {
      memcpy(&(oidptr[y*oid->getWidth()]),&iplptr[y*ipl->widthStep],oid->getWidth()*numbytes);
   }
}
}

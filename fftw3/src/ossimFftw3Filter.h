//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimFftw3Filter_HEADER
#define ossimFftw3Filter_HEADER
#include <ossim/imaging/ossimFftFilter.h>

class ossimFftw3Filter : public ossimFftFilter
{
public:
   ossimFftw3Filter(ossimObject* owner=NULL);
   ossimFftw3Filter(ossimImageSource* inputSource);
   ossimFftw3Filter(ossimObject* owner, ossimImageSource* inputSource);
   
protected:
   virtual void runFft(ossimRefPtr<ossimImageData>& input, ossimRefPtr<ossimImageData>& output);

   virtual ~ossimFftw3Filter();

TYPE_DATA
};

#endif

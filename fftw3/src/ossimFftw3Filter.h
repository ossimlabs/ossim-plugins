//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimFftw3Filter_HEADER
#define ossimFftw3Filter_HEADER
#include <ossim/imaging/ossimImageSourceFilter.h>

class ossimScalarRemapper;

class ossimFftw3Filter : public ossimImageSourceFilter
{
public:
   enum TransformDirection  { FORWARD = -1, INVERSE=1 };

   ossimFftw3Filter(ossimObject* owner=NULL);
   ossimFftw3Filter(ossimImageSource* inputSource);
   ossimFftw3Filter(ossimObject* owner,
                  ossimImageSource* inputSource);
   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& rect, ossim_uint32 resLevel=0);
   virtual void initialize();
   virtual ossim_uint32 getNumberOfOutputBands() const;
   virtual double getMinPixelValue(ossim_uint32 band=0)const;
   virtual double getMaxPixelValue(ossim_uint32 band=0)const;
   virtual double getNullPixelValue(ossim_uint32 band=0)const;
   virtual ossimScalarType getOutputScalarType() const;
   
   void setForward();
   void setInverse();
   ossimString getDirectionTypeAsString()const;
   void setDirectionType(const ossimString& directionType);
   void setDirectionType(TransformDirection directionType);
      
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;
   virtual void setProperty(ossimRefPtr<ossimProperty> property);
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;

   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix = 0);
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix = 0)const;
protected:
   void runFft(ossimRefPtr<ossimImageData>& input, ossimRefPtr<ossimImageData>& output);

   virtual ~ossimFftw3Filter();
   ossimRefPtr<ossimImageData> m_tile;
   TransformDirection m_xformDir;
   ossimRefPtr<ossimScalarRemapper> m_scalarRemapper;

TYPE_DATA
};

#endif

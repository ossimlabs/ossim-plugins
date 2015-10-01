//*******************************************************************
//
// License:  See top level LICENSE.txt file.
//
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalWriter.h 19733 2011-06-06 23:35:25Z dburken $
#ifndef ossimGdalWriter_HEADER
#define ossimGdalWriter_HEADER 1

#include <gdal.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/imaging/ossimNBandToIndexFilter.h>

class ossimXmlNode;

class ossimGdalWriter : public ossimImageFileWriter
{
public:
   enum ossimGdalOverviewType
   {
      ossimGdalOverviewType_NONE    = 0,
      ossimGdalOverviewType_NEAREST = 1,
      ossimGdalOverviewType_AVERAGE = 2
   };
   ossimGdalWriter();
   virtual              ~ossimGdalWriter();

   
   ossimGdalWriter(ossimImageSource *inputSource,
                   const ossimFilename& filename);

   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * @param imageTypeList stl::vector<ossimString> list to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   virtual bool hasImageType(const ossimString& imageType) const;

   /**
    * Returns a 3-letter extension from the image type descriptor 
    * (theOutputImageType) that can be used for image file extensions.
    *
    * @param imageType string representing image type.
    *
    * @return the 3-letter string extension.
    */
   virtual ossimString getExtension() const;

   virtual bool isOpen()const;
   
   virtual bool open();
   virtual void close();

   /*!
    * saves the state of the object.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;
   
   /*!
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);

   static void getSupportedWriters(vector<ossimString>& writers);
   
   virtual void setProperty(ossimRefPtr<ossimProperty> property);
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;

   virtual void  setOutputImageType(const ossimString&  type);

   void setLut(const ossimNBandLutDataObject& lut);

protected:
   virtual bool writeFile();
   virtual bool writeBlockFile();
   virtual void writeProjectionInfo(GDALDatasetH dataset);
   
   virtual void writeColorMap(int bands);
   
   virtual void setOptions(ossimKeywordlist& kwl,
                           const char* prefix=NULL);
   void deleteGdalDriverOptions();
   ossimString convertToDriverName(const ossimString& imageTypeName)const;
   
   GDALDataType getGdalDataType(ossimScalarType scalar);
   void getGdalPropertyNames(std::vector<ossimString>& propertyNames)const;
   const ossimRefPtr<ossimXmlNode> getGdalOptions()const;
   ossim_int32 findOptionIdx(const ossimString& name)const;
   void allocateGdalDriverOptions();
   void storeProperty(const ossimString& name,
                      const ossimString& value);
   bool getStoredPropertyValue(const ossimString& name,
                               ossimString& value)const;
   bool validProperty(const ossimString& name)const;
   ossimString gdalOverviewTypeToString()const;
   ossimGdalOverviewType gdalOverviewTypeFromString(const ossimString& typeString)const;
   void buildGdalOverviews();
   void appendGdalOption(const ossimString& name, const ossimString& value, bool replaceFlag = true);

   /**
    * @brief Method called after good writeFile to clean up any unwanted
    * files.
    */
   void postProcessOutput() const;

   bool isLutEnabled()const;

   void checkColorLut();

   /**
    * @brief Check input to see if it's indexed.
    * 
    * @return True if input data is indexed pallete data, false if not.
    */
   bool isInputDataIndexed();
   
   ossimString  theDriverName;
   
   GDALDriverH  theDriver;
   GDALDatasetH theDataset;

   ossimIpt theJpeg2000TileSize;
   ossimKeywordlist theDriverOptionValues;
   
   char** theGdalDriverOptions;
   ossimGdalOverviewType theGdalOverviewType;

   bool                                         theColorLutFlag;
   ossimRefPtr<ossimNBandLutDataObject>         theColorLut;
   ossimFilename                                theLutFilename;
   mutable ossimRefPtr<ossimNBandToIndexFilter> theNBandToIndexFilter;
   
TYPE_DATA
};

#endif

//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file.
//
// Author:  David Burken
//
// Description:  OSSIM wrapper for building gdal overviews (tiff or hfa) from
// an ossim image source.
//
//----------------------------------------------------------------------------
// $Id: ossimGdalOverviewBuilder.h 15833 2009-10-29 01:41:53Z eshirschorn $

#ifndef ossimGdalOverviewBuilder_HEADER
#define ossimGdalOverviewBuilder_HEADER

#include <vector>

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/imaging/ossimOverviewBuilderBase.h>

class ossimImageSource;
class ossimGdalDataset;

/**
 * @brief ossimGdalOverviewBuilder Class to build overviews from the GDAL
 * library.
 */
class ossimGdalOverviewBuilder
   :
   public ossimOverviewBuilderBase
{
public:

   /** @brief Enumerations for the type of GDAL overviews to build. */
   enum ossimGdalOverviewType
   {
      ossimGdalOverviewType_UNKNOWN = 0,
      ossimGdalOverviewTiffNearest  = 1,
      ossimGdalOverviewTiffAverage  = 2,
      ossimGdalOverviewHfaNearest   = 3,
      ossimGdalOverviewHfaAverage   = 4
   };

   /** @brief default constructor */
   ossimGdalOverviewBuilder();

   /** @brief virtual destructor */
   virtual ~ossimGdalOverviewBuilder();

   /**
    * @brief Open that takes a file name.
    * @param file The file to open.
    * @return true on success, false on error.
    */
   bool open(const ossimFilename& file);

   /**
    * @brief Builds the overviews.
    *
    * @return true on success, false on error.
    *
    * @note If setOutputFile was not called the output name will be derived
    * from the image name.  If image was "foo.tif" the overview file will
    * be "foo.rrd" or "foo.ovr".
    */
   virtual bool execute();

      /**
    * @brief Sets the input to the builder. Satisfies pure virtual from
    * ossimOverviewBuilderBase.
    * @param imageSource The input to the builder.
    * @return True on successful initializion, false on error.
    */
   virtual bool setInputSource(ossimImageHandler* imageSource);
   
   /**
    * @brief Sets the output filename.
    * Satisfies pure virtual from ossimOverviewBuilderBase.
    * @param file The output file name.
    */
   virtual void  setOutputFile(const ossimFilename& file);

   /**
    * Returns the output.  This will be derived from the input file if not
    * explicitly set.
    * 
    * @return The output filename.
    */
   virtual ossimFilename getOutputFile() const;

   /**
    * @brief Sets the overview output type.
    *
    * Satisfies pure virtual from ossimOverviewBuilderBase.
    * 
    * Currently handled types are:
    * "ossim_tiff_nearest" and "ossim_tiff_box"
    *
    * @param type This should be the string representing the type.  This method
    * will do nothing if type is not handled and return false.
    *
    * @return true if type is handled, false if not.
    */
   virtual bool setOverviewType(const ossimString& type);

   /**
    * @brief Gets the overview type.
    * Satisfies pure virtual from ossimOverviewBuilderBase.
    * @return The overview output type as a string.
    */
   virtual ossimString getOverviewType() const;

   /**
    * @brief Method to populate class supported types.
    * Satisfies pure virtual from ossimOverviewBuilderBase.
    * @param typeList List of ossimStrings to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /**
    * @return ossimObject* to this object. Satisfies pure virtual.
    */
   virtual ossimObject* getObject();

   /**
    * @return const ossimObject* to this object.  Satisfies pure virtual.
    */
   virtual const ossimObject* getObject() const;

   /**
    * @return true if input is an image handler.  Satisfies pure virtual.
    */
   virtual bool canConnectMyInputTo(ossim_int32 index,
                                    const ossimConnectableObject* obj) const;

   /**
    * @brief Method to set properties.
    * @param property Property to set.
    *
    * @note Currently supported property:
    * name=levels, value should be list of levels separated by a comma with
    * no spaces. Example: "2,4,8,16,32,64"
    */
   virtual void setProperty(ossimRefPtr<ossimProperty> property);

   /**
    * @brief Method to populate the list of property names.
    * @param propertyNames List to populate.  This does not clear the list
    * just adds to it.
    */
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;
   
   /**
    * @brief print method.
    * @return std::ostream&
    */
   virtual std::ostream& print(std::ostream& out) const;

private:

   bool generateHfaStats() const;

   /** @return The gdal resampling string from theOverviewType. */
   ossimString getGdalResamplingType() const;

   /** @return extension like "ovr" or "rrd". */
   ossimString getExtensionFromType() const;
   
   ossimGdalDataset*               theDataset;
   ossimFilename                   theOutputFile;
   ossimGdalOverviewType           theOverviewType;
   std::vector<ossim_int32>        theLevels; // like 2, 4, 8, 16, 32
   bool                            theGenerateHfaStatsFlag;

   /** for rtti stuff */
   TYPE_DATA
};

#endif /* End if "#ifndef ossimGdalOverviewBuilder_HEADER" */

//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  OSSIM wrapper for building nitf, j2k compressed overviews
// using kakadu from an ossim image source.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduNitfOverviewBuilder.h 21693 2012-09-11 15:20:38Z dburken $

#ifndef ossimKakaduNitfOverviewBuilder_HEADER
#define ossimKakaduNitfOverviewBuilder_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/imaging/ossimOverviewBuilderBase.h>
#include <vector>

class ossimImageHandler;
class ossimKakaduCompressor;
class ossimNitfImageHeaderV2_1;

/**
 * @brief ossimKakaduNitfOverviewBuilder Class to build overviews from the
 * Kakadu library.
 */
class OSSIM_PLUGINS_DLL ossimKakaduNitfOverviewBuilder
   :
   public ossimOverviewBuilderBase
{
public:

   /** @brief default constructor */
   ossimKakaduNitfOverviewBuilder();

   /** @brief virtual destructor */
   virtual ~ossimKakaduNitfOverviewBuilder();

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
    * @brief Sets the output filename.
    * Satisfies pure virtual from ossimOverviewBuilderInterface.
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
    * Satisfies pure virtual from ossimOverviewBuilderInterface.
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
    * Satisfies pure virtual from ossimOverviewBuilderInterface.
    * @return The overview output type as a string.
    */
   virtual ossimString getOverviewType() const;

   /**
    * @brief Method to populate class supported types.
    * Satisfies pure virtual from ossimOverviewBuilderInterface.
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
    * @biref Method to set properties.
    *
    * @param property Property to set.
    *
    * @note Currently supported property:
    */
   virtual void setProperty(ossimRefPtr<ossimProperty> property);

   /**
    * @brief Method to populate the list of property names.
    *
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

   /**
    * Set the Image Magnification (IMAG) field in the form of "/2  ".
    * @param hdr The nitf image header.
    * @param starting resolution level.
    */
   void setImagField(ossimNitfImageHeaderV2_1* hdr,
                     ossim_uint32 startingResLevel) const;

   ossimFilename                  m_outputFile;
   ossimFilename                  m_outputFileTmp;
   ossimKakaduCompressor*         m_compressor;

   /** for rtti stuff */
   TYPE_DATA
};

#endif /* End if "#ifndef ossimKakaduNitfOverviewBuilder_HEADER" */

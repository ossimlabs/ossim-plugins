//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM MrSID writer class declaration.
//
//----------------------------------------------------------------------------
// $Id: ossimMrSidWriter.h 1256 2010-08-09 17:11:17Z ming.su $

#ifndef ossimMrSidWriter_HEADER
#define ossimMrSidWriter_HEADER 1

#ifdef OSSIM_ENABLE_MRSID_WRITE

#include <iosfwd>
#include <vector>

#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>

//LizardTech Includes
#include <lti_rawImageReader.h>
#include <MG2ImageWriter.h>
#include <MG3ImageWriter.h>

LT_USE_NAMESPACE(LizardTech);

class ossimKeywordlist;
class ossimMapProjection;

class MrSIDDummyImageReader : public LTIImageReader
{
public:

  MrSIDDummyImageReader(ossimRefPtr<ossimImageSourceSequencer> theInputConnection);
  ~MrSIDDummyImageReader();
  LT_STATUS           initialize();
  lt_int64            getPhysicalFileSize(void) const { return 0; };

private:
  ossimRefPtr<ossimImageSourceSequencer> theInputSource;
  LTIDataType         sampleType;
  LTIPixel*           ltiPixel;
 
  virtual LT_STATUS   decodeStrip( LTISceneBuffer& stripBuffer,
    const LTIScene& stripScene );
  virtual LT_STATUS   decodeBegin( const LTIScene& scene )
  { return LT_STS_Success; };
  virtual LT_STATUS   decodeEnd() { return LT_STS_Success; };
  ossimString getWkt(ossimMapProjection* map_proj);
};

class ossimMrSidWriter : public ossimImageFileWriter
{
public:

   /** @brief default constructor */
   ossimMrSidWriter();

   /* @brief virtual destructor */
   virtual ~ossimMrSidWriter();

   /** @return "ossim_mrsid_writer" */
   virtual ossimString getShortName() const;

   /** @return "ossim mrsid writer" */
   virtual ossimString getLongName()  const;

   /** @return "ossimMrSidWriter" */
   virtual ossimString getClassName()    const;
   
   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * This writer only has one type "sid".
    *
    * @param imageTypeList stl::vector<ossimString> list to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   
   virtual bool isOpen()const;   
   
   virtual bool open();

   virtual void close();
   
   /**
    * saves the state of the object.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;
   
   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);

   /**
    * Will set the property whose name matches the argument
    * "property->getName()".
    *
    * @param property Object containing property to set.
    */
   virtual void setProperty(ossimRefPtr<ossimProperty> property);

   /**
    * @param name Name of property to return.
    * 
    * @returns A pointer to a property object which matches "name".
    */
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;

   /**
    * Pushes this's names onto the list of property names.
    *
    * @param propertyNames array to add this's property names to.
    */
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;

   /**
   * Returns a 3-letter extension from the image type descriptor 
   * (theOutputImageType) that can be used for image file extensions.
   *
   * @param imageType string representing image type.
   *
   * @return the 3-letter string extension.
   */
   virtual ossimString getExtension() const;

   bool hasImageType(const ossimString& imageType) const;

   /**
   * @brief Method to write the image to a stream.
   *
   * @return true on success, false on error.
   */
   virtual bool writeStream();

   void writeMetaDatabase(LTIMetadataDatabase& metadataDatabase);

protected:
   /**
    * @brief Writes the file to disk or a stream.
    * @return true on success, false on error.
    */
   virtual bool writeFile(); 

   ossim_int32 m_fileSize;
   bool        m_makeWorldFile;

 TYPE_DATA
};

#endif /* #ifdef OSSIM_ENABLE_MRSID_WRITE */

#endif /* #ifndef ossimMrSidWriter_HEADER */

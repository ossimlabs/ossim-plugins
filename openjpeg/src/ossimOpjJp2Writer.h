//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Open JPEG (j2k) writer.
//
//----------------------------------------------------------------------------
// $Id: ossimOpjJp2Writer.h 11652 2007-08-24 17:14:15Z dburken $
#ifndef ossimOpjJp2Writer_HEADER
#define ossimOpjJp2Writer_HEADER 1

#include <ossim/base/ossimRtti.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNBandLutDataObject.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimNBandToIndexFilter.h>
#include <ossim/base/ossimIoStream.h>

class ossimOpjCompressor;

class ossimOpjJp2Writer : public ossimImageFileWriter
{
public:

   /* default constructor */
   ossimOpjJp2Writer();

   /* 
    * constructor with typeName
    *   if typeName=="ossim_opj_geojp2", only write geotiff header
    *   if typeName=="ossim_opj_gmljp2", only write gmljp2 header
    *   else, write both headers
    */
   ossimOpjJp2Writer( const ossimString& typeName );

   /* virtual destructor */
   virtual ~ossimOpjJp2Writer();

   /** @return "ossim_openjpeg_writer" */
   virtual ossimString getShortName() const;

   /** @return "ossim open jpeg writer" */
   virtual ossimString getLongName()  const;

   /** @return "ossimOpjReader" */
   virtual ossimString getClassName()    const;
   
   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * This writer has types "ossim_opj_jp2", "ossim_opj_geojp2", and "ossim_opj_gmljp2".
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
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames) const;
   
   /**
    * Returns a 3-letter extension from the image type descriptor 
    * (theOutputImageType) that can be used for image file extensions.
    *
    * @param imageType string representing image type.
    *
    * @return the 3-letter string extension.
    */
   virtual ossimString getExtension() const;

   /**
    * Examples of writers that always generate internal
    * overviews are ossim_kakadu_jp2 and ossim_kakadu_nitf_j2k.
    *
    * @return true if the output of the writer will have
    * internal overviews. The default is false. 
    */
   virtual bool getOutputHasInternalOverviews( void ) const;

   /**
    * @param imageType
    * @return true if "imagetype" is one of: 
    * "image/jp2" or "image/j2k"
    * "ossim_opj_jp2", ossim_opj_geojp2", or "ossim_opj_gmljp2"
    */
   bool hasImageType(const ossimString& imageType) const;

   /**
    * @brief Method to write the image to a stream.
    *
    * @return true on success, false on error.
    */
   virtual bool writeStream();

   /**
    * @brief Sets the output stream to write to.
    *
    * The stream will not be closed/deleted by this object.
    *
    * @param output The stream to write to.
    */
   virtual bool setOutputStream(std::ostream& stream);

private:

   bool writeGeotiffBox(std::ostream* stream, ossimOpjCompressor* compressor);
   bool writeGmlBox(std::ostream* stream, ossimOpjCompressor* compressor);

   /**
    * @brief Hack to copy bytes to vector so we can re-write them. Hack for
    * inserting geotiff and gml boxes in front of jp2c codestream block.
    */
   void copyData( const std::streamoff& pos,
                  ossim_uint32 size,
                  std::vector<ossim_uint8>& data ) const;
   
   /**
    * @brief Writes the file to disk or a stream.
    * @return true on success, false on error.
    */
   virtual bool writeFile();

   std::ostream*       m_outputStream;
   bool                m_ownsStreamFlag;
   bool                m_overviewFlag;
   ossimOpjCompressor* m_compressor;
   bool                m_do_geojp2;
   bool                m_do_gmljp2;

   TYPE_DATA

};

#endif /* #ifndef ossimOpjVoid Writer_HEADER */

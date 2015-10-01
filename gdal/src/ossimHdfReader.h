//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  Class declaration HDF reader.
//
// Specification: ISO/IEC 15444
//
//----------------------------------------------------------------------------
// $Id: ossimHdfReader.h 2645 2011-05-26 15:21:34Z oscar.kramer $

#ifndef ossimHdfReader_HEADER
#define ossimHdfReader_HEADER 1

//ossim includes
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>
#include <ossimGdalTileSource.h>

// Forward class declarations.
class ossimImageData;

class OSSIM_PLUGINS_DLL ossimHdfReader : public ossimImageHandler
{
public:

   /** default construtor */
   ossimHdfReader();
   
   /** virtural destructor */
   virtual ~ossimHdfReader();

   /**
    * @brief Returns short name.
    * @return "ossim_hdf_reader"
    */
   virtual ossimString getShortName() const;
   
   /**
    * @brief Returns long  name.
    * @return "ossim hdf reader"
    */
   virtual ossimString getLongName()  const;

   /**
    * @brief Returns class name.
    * @return "ossimHdfReader"
    */
   virtual ossimString getClassName() const;

   /**
    *  @brief Method to grab a tile(rectangle) from image.
    *
    *  @param rect The zero based rectangle to grab.
    *
    *  @param resLevel The reduced resolution level to grab from.
    *
    *  @return The ref pointer with the image data pointer.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const  ossimIrect& rect,
                                               ossim_uint32 resLevel=0);

   virtual bool getTile(ossimImageData* result, ossim_uint32 resLevel=0);   

   /**
    *  Returns the number of bands in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfInputBands() const;

   /**
    * Returns the number of bands in a tile returned from this TileSource.
    * Note: we are supporting sources that can have multiple data objects.
    * If you want to know the scalar type of an object you can pass in the
    */
   virtual ossim_uint32 getNumberOfOutputBands()const; 
   /**
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileWidth() const;

   /**
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileHeight which
    * returns the output tile height which can be different than the internal
    * image tile height on disk.
    */
   virtual ossim_uint32 getImageTileHeight() const;

   /**
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;
 

   /**
    * @brief Gets number of lines for res level.
    *
    *  @param resLevel Reduced resolution level to return lines of.
    *  Default = 0
    *
    *  @return The number of lines for specified reduced resolution level.
    */
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 resLevel = 0) const;

   /**
    *  @brief Gets the number of samples for res level.
    *
    *  @param resLevel Reduced resolution level to return samples of.
    *  Default = 0
    *
    *  @return The number of samples for specified reduced resolution level.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 resLevel = 0) const;

   /**
    * @brief Open method.
    * @return true on success, false on error.
    */
   virtual bool open();

   /**
    *  @brief Method to test for open file stream.
    *
    *  @return true if open, false if not.
    */
   virtual bool isOpen()const;

   /**
    * @brief Method to close current entry.
    *
    */
   virtual void close();

   /**
    * Returns the image geometry object associated with this tile source or
    * NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);   

   virtual bool setCurrentEntry(ossim_uint32 entryIdx);

    /**
    * @return The number of entries (images) in the image file.
    */
   virtual ossim_uint32 getNumberOfEntries()const;
   
   /**
    * @param entryList This is the list to initialize with entry indexes.
    *
    * @note This implementation returns puts one entry "0" in the list.
    */
   virtual void getEntryList(std::vector<ossim_uint32>& entryList) const;

   /**
    * @return The current entry number.
    *
    */
   virtual ossim_uint32 getCurrentEntry() const { return m_currentEntryRender; }

   bool isSupportedExtension();

   std::vector<ossim_uint32> getCurrentEntryList(ossim_uint32 entryId) const;

   ossimString getEntryString(ossim_uint32 entryId) const;

   ossimString getDriverName();

   virtual bool setOutputBandList(const std::vector<ossim_uint32>& band_list);

private:

  bool isSDSDataset(ossimString fileName);

  ossimRefPtr<ossimGdalTileSource>                              m_gdalTileSource;
  mutable std::vector<ossim_uint32>                             m_entryFileList;
  ossim_uint32                                                  m_numberOfBands;
  ossimScalarType                                               m_scalarType;
  ossim_uint32                                                  m_currentEntryRender;
  ossimRefPtr<ossimImageData>                                   m_tile;
TYPE_DATA
};

#endif /* #ifndef ossimHdfReader_HEADER */

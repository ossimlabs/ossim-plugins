//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  Class declaration MrSID MG4 reader.
//
// Author: Mingjie Su
//
// Specification: ISO/IEC 15444
//
//----------------------------------------------------------------------------
// $Id: ossimMG4LidarReader.h 1524 2010-10-29 19:31:54Z ming.su$

#ifndef ossimMG4LidarReader_HEADER
#define ossimMG4LidarReader_HEADER 1

#include <float.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>

//LizardTech Includes
#include <lidar/MG4PointReader.h>
#include <lidar/Error.h>
#include <lidar/Version.h>

LT_USE_LIDAR_NAMESPACE

// Forward class declarations.
class ossimImageData;
class ossimDpt;

class OSSIM_PLUGINS_DLL ossimMG4LidarReader : public ossimImageHandler
{
public:

   /** default construtor */
   ossimMG4LidarReader();
   
   /** virtural destructor */
   virtual ~ossimMG4LidarReader();

   /**
    * @brief Returns short name.
    * @return "ossim_mrsid_reader"
    */
   virtual ossimString getShortName() const;
   
   /**
    * @brief Returns long  name.
    * @return "ossim mrsid reader"
    */
   virtual ossimString getLongName()  const;

   /**
    * @brief Returns class name.
    * @return "ossimMG4LidarReader"
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
    * @brief Returns the number of decimation levels.
    * 
    * This returns the total number of decimation levels.  It is important to
    * note that res level 0 or full resolution is included in the list and has
    * decimation values 1.0, 1.0
    * 
    * @return The number of decimation levels.
    */
   virtual ossim_uint32 getNumberOfDecimationLevels()const;

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
    * @note There is a bool kdu_compressed_source::close() and a
    * void ossimImageHandler::close(); hence, a new close to avoid conflicting
    * return types.
    */
   virtual void closeEntry();

   /**
    * Returns the image geometry object associated with this tile source or
    * NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

   /**
    * @param Method to get geometry.
    */
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry() const;

   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix=0);   

private:

  ossimProjection* getGeoProjection();

  void getDataType(ossim_int32 channelId);

  void openZoomLevel(ossim_int32 zoom);

  template<typename DTYPE>
     const DTYPE getChannelElement(const ChannelData* channel, size_t idx);

  MG4PointReader*              m_reader;
  Bounds                       m_bounds;
  ossimIrect                   m_imageRect; /** Has sub image offset. */

  ossim_uint32                 m_numberOfSamples;
  ossim_uint32                 m_numberOfLines;
  ossimScalarType              m_scalarType;
  ossimRefPtr<ossimImageData>  m_tile;
TYPE_DATA
};

#endif /* #ifndef ossimMG4LidarReader_HEADER */





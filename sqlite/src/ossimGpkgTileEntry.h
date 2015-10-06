//----------------------------------------------------------------------------
//
// File: ossimGpkgTileEntry.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for GeoPackage tile entry.
//
// Holds a gpkg_tile_matrix_set and a vector of gpkg_tile_matrix.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgTileEntry_HEADER
#define ossimonGpkgTileEntry_HEADER 1

#include "ossimGpkgSpatialRefSysRecord.h"
#include "ossimGpkgTileMatrixRecord.h"
#include "ossimGpkgTileMatrixSetRecord.h"
#include "ossimGpkgNsgTileMatrixExtentRecord.h"

#include <ossim/base/ossimRefPtr.h>

#include <iosfwd>
#include <vector>

class ossimDpt;
class ossimIpt;
class ossimKeywordlist;
class ossimMapProjection;

class ossimGpkgTileEntry
{
public:
   
   /** @brief default constructor */
   ossimGpkgTileEntry();

   /* @brief copy constructor */
   ossimGpkgTileEntry(const ossimGpkgTileEntry& obj);

   /* @brief assignment operator= */
   const ossimGpkgTileEntry& operator=
      (const ossimGpkgTileEntry& obj);

   /** @brief destructor */
   ~ossimGpkgTileEntry();

   /**
    * @brief Sets the tile matrix set.
    * @param set
    */
   void setTileMatrixSet(const ossimGpkgTileMatrixSetRecord& set);

   /** @return tile matrix set. */
   const ossimGpkgTileMatrixSetRecord& getTileMatrixSet() const;

   /**
    * @brief Sets the spatial ref sys.
    * @param srs
    */
   void setSrs( const ossimGpkgSpatialRefSysRecord& srs );

   /** @retrurn Spatial ref sys. */
   const ossimGpkgSpatialRefSysRecord& getSrs() const;

   /**
    * @brief Adds a tile matrix level to array.
    * @param level Level to add.
    */
   void addTileMatrix(const ossimGpkgTileMatrixRecord& level);

   /** @return const reference to the tile matrix. */
   const std::vector<ossimGpkgTileMatrixRecord>& getTileMatrix() const;

   /**
    * @brief Adds a tile matrix extent level to array.
    * @param level Level to add.
    */
   void addTileMatrixExtent(const ossimGpkgNsgTileMatrixExtentRecord& record);

   /** @return const reference to the tile matrix extent. */
   const std::vector<ossimGpkgNsgTileMatrixExtentRecord>&
      getTileMatrixExtent() const;

   /**
    * @brief Sorts the m_tileMatrix by zoom levels with the highest
    * zoom level being at the lowest array index.
    *
    * The highest zoom level is the best resolution in ossim.  In
    * other words if zoom level 21 is highest zoom level this is
    * equal to r0 (reduced resolution 0.
    */
   void sortTileMatrix();

   /**
    * @brief Sorts the m_tileMatrixExtents by zoom levels with the highest
    * zoom level being at the lowest array index.
    *
    * The highest zoom level is the best resolution in ossim.  In
    * other words if zoom level 21 is highest zoom level this is
    * equal to r0 (reduced resolution 0.
    */
   void sortTileMatrixExtents();

   /**
    * @brief Saves the state of object.
    * @param kwl Initialized by this.
    * @param prefix e.g. "image0.". Can be empty.
    */
   void saveState( ossimKeywordlist& kwl,
                   const std::string& prefix ) const;

   ossim_uint32 getNumberOfLines( ossim_uint32 resLevel ) const;
   ossim_uint32 getNumberOfSamples( ossim_uint32 resLevel ) const;
   void getSubImageOffset( ossim_uint32 resLevel, ossimIpt& offset ) const;

   /**
    * @brief Gets the tie point from the first tile matrix extents if initialized
    * else from the tile matrix extents.
    */
   void getTiePoint( ossimDpt& offset ) const;

   /**
    * @brief Gets the gsd from tile matrix.
    * @param index to tile matrix array.
    * @param gsd Initialized by this. Will be nan if matrix is empty.
    */
   void getGsd( ossim_uint32 index, ossimDpt& gsd ) const;

   /**
    * @brief Gets zoom levels of all tile matrixes.
    *
    * @param zoomLevels Intitialized by this.
    */
   void getZoomLevels( std::vector<ossim_int32>& zoomLevels ) const;

   /**
    * @brief Gets zoom level matrix of all tile matrixes.
    *
    * @param zoomLevelMatrixSizes Intitialized by this.
    */
   void getZoomLevelMatrixSizes( std::vector<ossimIpt>& zoomLevelMatrixSizes ) const;

   /**
    * @brief Gets the map projection to include setting the tie and scale.
    * @return Map projection.  Result will be null if matrix is not intialized.
    */
   ossimRefPtr<ossimMapProjection> getNewMapProjection() const;
   
   /**
    * @brief Print method.
    * @param out Stream to print to.
    * @return Stream reference.
    */
   std::ostream& print(std::ostream& out) const;

   /**
    * @brief Validate method.  Prints data from database and computed values.
    * @param out Stream to print to.
    * @return Stream reference.
    */
   std::ostream& printValidate(std::ostream& out) const;
   
   /**
    * @brief Convenience operator << function.
    * @param out Stream to print to.
    * @param obj Object to print.
    */
   friend std::ostream& operator<<(std::ostream& out,
                                   const ossimGpkgTileEntry& obj);
   
private:
   ossimGpkgSpatialRefSysRecord m_srs;
   ossimGpkgTileMatrixSetRecord m_tileMatrixSet;
   std::vector<ossimGpkgTileMatrixRecord> m_tileMatrix;
   std::vector<ossimGpkgNsgTileMatrixExtentRecord> m_tileMatrixExtents;   
};

#endif /* #ifndef ossimGpkgTileEntry_HEADER */

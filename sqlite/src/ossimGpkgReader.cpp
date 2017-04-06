//---
//
// File: ossimGpkgReader.cpp
// 
// License: MIT
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM GeoPackage reader (tile source).
//
//---
// $Id$

#include "ossimGpkgReader.h"
#include "ossimGpkgSpatialRefSysRecord.h"
#include "ossimGpkgTileRecord.h"
#include "ossimGpkgUtil.h"

#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimIoStream.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/imaging/ossimCodecFactoryRegistry.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimImageGeometryRegistry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimStreamReaderInterface.h>

#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimProjection.h>

#include <sqlite3.h>

#include <cmath>
#include <sstream>

RTTI_DEF1(ossimGpkgReader, "ossimGpkgReader", ossimImageHandler)

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id$";
#endif

static ossimTrace traceDebug("ossimGpkgReader:debug");
static ossimTrace traceValidate("ossimGpkgReader:validate");

ossimGpkgReader::ossimGpkgReader()
   :
   ossimImageHandler(),
   m_ih(0),
   m_tile(0),
   m_cacheTile(0),
   m_db(0),
   m_currentEntry(0),
   m_bands(0),
   m_scalar(OSSIM_SCALAR_UNKNOWN),
   m_tileWidth(0),
   m_tileHeight(0),
   m_entries(0)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgReader::ossimGpkgReader entered...\n";
#ifdef OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  " << OSSIM_ID << "\n";
#endif
   }
}

ossimGpkgReader::~ossimGpkgReader()
{
   if (isOpen())
   {
      close();
   }
}

void ossimGpkgReader::allocate()
{
   m_tile = ossimImageDataFactory::instance()->create(this, this);
   m_tile->initialize();
}

ossimRefPtr<ossimImageData> ossimGpkgReader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   if ( !m_tile )
   {
      allocate();
   }
   
   if (m_tile.valid())
   {
      // Image rectangle must be set prior to calling getTile.
      m_tile->setImageRectangle(rect);
      
      if ( getTile( m_tile.get(), resLevel ) == false )
      {
         if (m_tile->getDataObjectStatus() != OSSIM_NULL)
         {
            m_tile->makeBlank();
         }
      }
   }
   
   return m_tile;
}

bool ossimGpkgReader::getTile(ossimImageData* result,
                             ossim_uint32 resLevel)
{
   bool status = false;
   
   //---
   // Not open, this tile source bypassed, or invalid res level,
   // return a blank tile.
   //---
   if( isOpen() && isSourceEnabled() && isValidRLevel(resLevel) &&
       result && (result->getNumberOfBands() == getNumberOfOutputBands()) )
   {
      result->ref(); // Increment ref count.

      if ( resLevel < getNumberOfZoomLevels() )
      {
         status = true;
         
         ossimIrect tileRect = result->getImageRectangle();
         ossimIrect imageRect = getImageRectangle(resLevel);

         //---
         // Format allows for missing tiles so alway make blank in case tile
         // is not filled completely.
         //---
         m_tile->makeBlank();
         
         if ( imageRect.intersects(tileRect) )
         {
            // Make a clip rect.
            ossimIrect clipRect = tileRect.clipToRect( imageRect );
            
            // This will validate the tile at the end.
            fillTile(resLevel, tileRect, clipRect, result);
         }
      }
      else
      {
         status = getOverviewTile(resLevel, result);
      }
      
      result->unref();  // Decrement ref count.
   }

   return status;
}

void ossimGpkgReader::fillTile( ossim_uint32 resLevel,
                                const ossimIrect& tileRect,
                                const ossimIrect& clipRect,
                                ossimImageData* tile )
{
   if ( tile )
   {
      // Get the tile indexes needed to fill the clipRect.
      std::vector<ossimIpt> tileIndexes;
      getTileIndexes( resLevel, clipRect, tileIndexes );
      
      std::vector<ossimIpt>::const_iterator i = tileIndexes.begin();
      while ( i != tileIndexes.end() )
      {
         ossimRefPtr<ossimImageData> id = getTile( resLevel, (*i) );
         if ( id.valid() )
         {
            ossimIrect tileClipRect = clipRect.clipToRect( id->getImageRectangle() );
            id->unloadTile( tile->getBuf(), tileRect, tileClipRect, OSSIM_BSQ );
         }
         ++i;
      }
      
      tile->validate();
   }
}

void ossimGpkgReader::getTileIndexes( ossim_uint32 resLevel,
                                      const ossimIrect& clipRect,
                                      std::vector<ossimIpt>& tileIndexes ) const
{
   ossimIpt tileSize;
   getTileSize( resLevel, tileSize );

   if ( tileSize.x && tileSize.y )
   {
      ossimIrect expandedRect = clipRect;

      // Add the sub image offset if any:
      ossimIpt subImageOffset(0,0);
      if ( m_currentEntry < m_entries.size() )
      {
         m_entries[m_currentEntry].getSubImageOffset( resLevel, subImageOffset );
         expandedRect += subImageOffset;
      }

      expandedRect.stretchToTileBoundary( tileSize );

      ossim_int32 y = expandedRect.ul().y;
      while ( y < expandedRect.lr().y )
      {
         ossim_int32 x = expandedRect.ul().x;
         while ( x < expandedRect.lr().x )
         {
            ossimIpt index( x/tileSize.x, y/tileSize.y );
            tileIndexes.push_back( index );
            x += tileSize.x;
         }
         y += tileSize.y;
      }
   }
}

void ossimGpkgReader::getTileSize( ossim_uint32 resLevel, ossimIpt& tileSize ) const
{
   if ( m_currentEntry < (ossim_uint32)m_entries.size() )
   {
      if ( resLevel < (ossim_uint32)m_entries[m_currentEntry].getTileMatrix().size() )
      {
         tileSize.x =
            (ossim_int32)m_entries[m_currentEntry].getTileMatrix()[resLevel].m_tile_width;
         tileSize.y =
            (ossim_int32)m_entries[m_currentEntry].getTileMatrix()[resLevel].m_tile_height;
      }
   }
}

ossimIrect ossimGpkgReader::getImageRectangle(ossim_uint32 resLevel) const
{
   return ossimIrect(0,
                     0,
                     getNumberOfSamples(resLevel) - 1,
                     getNumberOfLines(resLevel)   - 1);
}

bool ossimGpkgReader::saveState(ossimKeywordlist& kwl,
                                const char* prefix) const
{
   return ossimImageHandler::saveState(kwl, prefix);
}

bool ossimGpkgReader::loadState(const ossimKeywordlist& kwl,
                               const char* prefix)
{
   bool result = false;
   if (ossimImageHandler::loadState(kwl, prefix))
   {
      result = open();
   }
   return result;
}

void ossimGpkgReader::setProperty(ossimRefPtr<ossimProperty> property)
{
   ossimImageHandler::setProperty(property);
}

ossimRefPtr<ossimProperty> ossimGpkgReader::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> prop = 0;
   prop = ossimImageHandler::getProperty(name);
   return prop;
}

void ossimGpkgReader::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   ossimImageHandler::getPropertyNames(propertyNames);
}

bool ossimGpkgReader::open()
{
   static const char M[] = "ossimGpkgReader::open";

   bool status = false;
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " entered..." << "\nFile:  " << theImageFile.c_str() << "\n";
   }

   // Start with a clean slate.
   if (isOpen())
   {
      close();
   }

   std::ifstream str;
   str.open(theImageFile.c_str(), std::ios_base::in | std::ios_base::binary);

   if ( ossim_gpkg::checkSignature( str ) )
   {
      if ( ossim_gpkg::checkApplicationId( str ) == false )
      {
         // Just issue a warning at this point as this requirement is new...
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << M << " WARNING!"
               << "File: " << theImageFile << " does not have required application_id!"
               << "\nProceeding anyway...\n";
         }
      }
      
      int rc = sqlite3_open_v2( theImageFile.c_str(), &m_db, SQLITE_OPEN_READONLY, 0);
      if ( rc == SQLITE_OK )
      {
         m_entries.clear();
         ossim_gpkg::getTileEntries( m_db, m_entries );

         if ( m_entries.size() )
         {
            status = initImageParams();
            
            if (traceDebug())
            {
               std::vector<ossimGpkgTileEntry>::const_iterator i = m_entries.begin();
               while ( i != m_entries.end() )
               {
                  ossimNotify(ossimNotifyLevel_DEBUG) << (*i) << "\n";
                  ++i;
               }
            }
            if (traceValidate())
            {
               std::vector<ossimGpkgTileEntry>::const_iterator i = m_entries.begin();
               while ( i != m_entries.end() )
               {
                  (*i).printValidate( ossimNotify(ossimNotifyLevel_DEBUG) );
                  ++i;
               }
            }
         }
         
      }
   }

   if ( !status )
   {
      close();
   }
   else
   {
      completeOpen();
   }
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " exit status: " << (status?"true":"false") << "\n";
   }

   return status;
}

ossim_uint32 ossimGpkgReader::getTileWidth() const
{
   ossim_uint32 result = getImageTileWidth();
   if (!result)
   {
      ossimIpt tileSize;
      ossim::defaultTileSize(tileSize);
      result = tileSize.x;
   }
   return result;
}

ossim_uint32 ossimGpkgReader::getTileHeight() const
{
   ossim_uint32 result = getImageTileHeight();
   if (!result)
   {
      ossimIpt tileSize;
      ossim::defaultTileSize(tileSize);
      result = tileSize.y;
   }
   return result;
}

ossim_uint32 ossimGpkgReader::getNumberOfLines(ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if ( m_currentEntry < m_entries.size() )
   {
      if ( resLevel < m_entries[m_currentEntry].getTileMatrix().size() )
      {
         result = m_entries[m_currentEntry].getNumberOfLines( resLevel );
      }
      else if ( theOverview.valid() )
      {
         result = theOverview->getNumberOfLines(resLevel);
      }
   }
   return result;
}

ossim_uint32 ossimGpkgReader::getNumberOfSamples(ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if ( m_currentEntry < m_entries.size() )
   {
      if ( resLevel < m_entries[m_currentEntry].getTileMatrix().size() )
      {
         result = m_entries[m_currentEntry].getNumberOfSamples( resLevel );
      }
      else if ( theOverview.valid() )
      {
         result = theOverview->getNumberOfSamples(resLevel);
      }
   }
   return result;
}
ossim_uint32 ossimGpkgReader::getImageTileWidth() const
{
   return m_tileWidth;
}

ossim_uint32 ossimGpkgReader::getImageTileHeight() const
{
   return m_tileHeight;
}

ossimString ossimGpkgReader::getShortName()const
{
   return ossimString("ossim_gpkg_reader");
}
   
ossimString ossimGpkgReader::getLongName()const
{
   return ossimString("ossim Geo Package reader");
}

ossimString  ossimGpkgReader::getClassName()const
{
   return ossimString("ossimGpkgReader");
}

ossim_uint32 ossimGpkgReader::getNumberOfInputBands() const
{
   return m_bands;
}

ossim_uint32 ossimGpkgReader::getNumberOfOutputBands()const
{
   return m_bands;
}

ossimScalarType ossimGpkgReader::getOutputScalarType() const
{
   return m_scalar;
}

bool ossimGpkgReader::isOpen()const
{
   return (m_db != 0 );
}

void ossimGpkgReader::close()
{
   if ( isOpen() )
   {
      ossimImageHandler::close();
   }
   if ( m_db )
   {
      sqlite3_close( m_db );
      m_db = 0;
   }
   m_entries.clear();
}

ossim_uint32 ossimGpkgReader::getNumberOfEntries()const
{
   return (ossim_uint32)m_entries.size();
}

ossim_uint32 ossimGpkgReader::getCurrentEntry()const
{
   return m_currentEntry;
}

void ossimGpkgReader::getEntryList(std::vector<ossim_uint32>& entryList)const
{
   for ( ossim_uint32 i = 0; i < m_entries.size(); ++i )
   {
      entryList.push_back(i);
   }
}

void ossimGpkgReader::getEntryNames(std::vector<ossimString>& entryNames) const
{
   std::vector<ossimGpkgTileEntry>::const_iterator i = m_entries.begin();
   while ( i != m_entries.end() )
   {
      entryNames.push_back( ossimString((*i).getTileMatrixSet().m_table_name) );
      ++i;
   }
}

bool ossimGpkgReader::setCurrentEntry(ossim_uint32 entryIdx )
{
   bool result = true;
   if ( m_currentEntry != entryIdx )
   {
      if ( entryIdx < getNumberOfEntries() )
      {
         m_currentEntry = entryIdx;
         
         if ( isOpen() )
         {
            // Zero out the tile to force an allocate() call.
            m_tile = 0;
            
            // Clear the geometry.
            theGeometry = 0;
            
            // Must clear or openOverview will use last entries.
            theOverviewFile.clear();

            // This intitializes bands, scalar, tile width, height...
            initImageParams();
            
            if ( result )
            {
               completeOpen();
            }
         }
      }
      else
      {
         result = false; // Entry index out of range.
      }
   }
   return result;
}

ossim_uint32 ossimGpkgReader::getNumberOfDecimationLevels() const
{
   // Internal overviews.
   ossim_uint32 result = getNumberOfZoomLevels();

   if (theOverview.valid())
   {
      // Add external overviews.
      result += theOverview->getNumberOfDecimationLevels();
   }

   return result;  
}

ossim_uint32 ossimGpkgReader::getNumberOfZoomLevels() const
{
   ossim_uint32 result = 0;
   
   //---
   // Index 0 is the highest zoom_level which is the best resolution or r0 in ossim terms.
   // In GeoPackage zoom_level 0 is the lowest resolution.
   //---
   if ( m_currentEntry < (ossim_uint32)m_entries.size())
   {
      size_t matrixSize = m_entries[m_currentEntry].getTileMatrix().size();
      if ( matrixSize )
      {
         result =
            m_entries[m_currentEntry].getTileMatrix()[0].m_zoom_level -
            m_entries[m_currentEntry].getTileMatrix()[ matrixSize-1 ].m_zoom_level + 1;
      }
   }

   return result;
}

ossimRefPtr<ossimImageGeometry> ossimGpkgReader::getImageGeometry()
{
   if ( !theGeometry )
   {
      //---
      // Check for external geom - this is a file.geom not to be confused with
      // geometries picked up from dot.xml, dot.prj, dot.j2w and so on.  We
      // will check for that later if the getInternalImageGeometry fails.
      //---
      theGeometry = getExternalImageGeometry();
      
      if ( !theGeometry )
      {
         // Check the internal geometry first to avoid a factory call.
         theGeometry = getInternalImageGeometry();
         
         //---
         // WARNING:
         // Must create/set the geometry at this point or the next call to
         // ossimImageGeometryRegistry::extendGeometry will put us in an infinite loop
         // as it does a recursive call back to ossimImageHandler::getImageGeometry().
         //---
         if ( !theGeometry )
         {
            theGeometry = new ossimImageGeometry();
         }
         
         // Check for set projection.
         if ( !theGeometry->getProjection() )
         {
            // Last try factories for projection.
            ossimImageGeometryRegistry::instance()->extendGeometry(this);
         }
      }

      // Set image things the geometry object should know about.
      initImageParameters( theGeometry.get() );
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGpkgReader::getImageGeometry geometry:\n"
            << *(theGeometry.get()) << "\n";
      }
      
   }
   return theGeometry;
   
} // End: ossimGpkgReader::getImageGeometry()

ossimRefPtr<ossimImageGeometry> ossimGpkgReader::getInternalImageGeometry()
{
   static const char M[] = "ossimGpkgReader::getInternalImageGeometry";

   ossimRefPtr<ossimImageGeometry> geom = 0;
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " entered...\n";
   }
   
   if ( m_currentEntry < m_entries.size() )
   {
      ossimRefPtr<ossimMapProjection> mapProj = m_entries[m_currentEntry].getNewMapProjection();

      if ( mapProj.valid() )
      {
         ossimRefPtr<ossimProjection> proj = mapProj.get();
         if ( proj.valid() )
         {
            // Create and assign projection to our ossimImageGeometry object.
            geom = new ossimImageGeometry();
            geom->setProjection( proj.get() );
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG) << "Created geometry...\n";
            }
         }  
      }
   }

   if (traceDebug())
   {
      if ( geom.valid() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << M << " exited...\n";
      }
   }
   
   return geom;
   
} // End: ossimGpkgReader::getInternalImageGeometry()

bool ossimGpkgReader::initImageParams()
{
   static const char MODULE[] = "ossimGpkgReader::initImageParams";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   bool result = false;
   if ( m_db )
   {
      if ( m_currentEntry < m_entries.size() )
      {
         if ( m_entries[m_currentEntry].getTileMatrix().size() )
         {
            ossimIpt index(-1,-1);
            
            std::string tableName = m_entries[m_currentEntry].getTileMatrix()[0].m_table_name;
            ossim_int32 zoomLevel = m_entries[m_currentEntry].getTileMatrix()[0].m_zoom_level;

            // Get the first tile for the highest res zoom level.

            const char *zLeftover;   // Tail of unprocessed SQL
            sqlite3_stmt *pStmt = 0; // The current SQL statement

            std::ostringstream sql;
            sql << "SELECT min(id), tile_column, tile_row from "
                << tableName << " WHERE zoom_level=" << zoomLevel;

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "sql:\n" << sql.str() << "\n";
            }
            
            int rc = sqlite3_prepare_v2(m_db,        // Database handle
                                        sql.str().c_str(),// SQL statement, UTF-8 encoded
                                        -1,          // Maximum length of zSql in bytes.
                                        &pStmt,      // OUT: Statement handle
                                        &zLeftover); // OUT: Pointer to unused portion of zSql
            if ( rc == SQLITE_OK )
            {
               int nCol = sqlite3_column_count( pStmt );
               if ( nCol == 3 )
               {
                  // Read the row:
                  rc = sqlite3_step(pStmt);
                  if ( rc == SQLITE_ROW )
                  {
                     index.x = sqlite3_column_int(pStmt, 1);
                     index.y = sqlite3_column_int(pStmt, 2);
                  }
               }
            }
               
            sqlite3_finalize(pStmt);

            if ( (index.x > -1) && (index.y > -1) )
            {
               // Grab a tile:
               ossimRefPtr<ossimImageData> tile = getTile( 0, index );
               if ( tile.valid() )
               {
                  // Set the bands:
                  m_bands = tile->getNumberOfBands();

                  // Set the scalar type:
                  m_scalar = tile->getScalarType();
                  
                  // Set tile size:
                  m_tileWidth = tile->getWidth();
                  m_tileHeight = tile->getHeight();
                  result = true;
               }
            }

            if ( !result )
            {
               // Set to some default:
               m_bands = 3;
               m_scalar = OSSIM_UINT8;
               m_tileWidth = 256;
               m_tileHeight = 256;
               result = true;
            }
         }
      }
   }

   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false") << "\n";
   }

   return result;
}

ossimRefPtr<ossimImageData> ossimGpkgReader::getTile( ossim_uint32 resLevel,
                                                      ossimIpt index)
{
   static const char MODULE[] = "ossimGpkgReader::getTile(resLevel, index)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..."
         << "\nresLevel: " << resLevel << " index: " << index << "\n";
   }

   ossimRefPtr<ossimImageData> result = 0;
   
   if ( m_db )
   {
      if ( m_currentEntry < m_entries.size() )
      {
         if ( resLevel < m_entries[m_currentEntry].getTileMatrix().size() )
         {
            std::string tableName =
               m_entries[m_currentEntry].getTileMatrix()[resLevel].m_table_name;
            ossim_int32 zoomLevel =
               m_entries[m_currentEntry].getTileMatrix()[resLevel].m_zoom_level;
            const char *zLeftover;   // Tail of unprocessed SQL
            sqlite3_stmt *pStmt = 0; // The current SQL statement
            std::ostringstream sql;
            sql << "SELECT id, zoom_level, tile_column, tile_row, tile_data from "
                << tableName
                << " WHERE zoom_level=" << zoomLevel
                << " AND tile_column=" << index.x
                << " AND tile_row=" << index.y;

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << MODULE << " sql:\n" << sql.str() << "\n";
            }
            
            int rc = sqlite3_prepare_v2(
               m_db,             // Database handle
               sql.str().c_str(),// SQL statement, UTF-8 encoded
               -1,               // Maximum length of zSql in bytes.
               &pStmt,           // OUT: Statement handle
               &zLeftover);      // OUT: Pointer to unused portion of zSql
            if ( rc == SQLITE_OK )
            {
               int nCol = sqlite3_column_count( pStmt );
               if ( nCol )
               {
                  // Read the row:
                  rc = sqlite3_step(pStmt);
                  if (rc == SQLITE_ROW) //  || (rc == SQLITE_DONE) )
                  {
                     ossimGpkgTileRecord tile;
                     tile.setCopyTileFlag(true);
                     if (tile.init( pStmt ) )
                     {
                        ossimIpt tileSize;
                        m_entries[m_currentEntry].getTileMatrix()[resLevel].getTileSize(tileSize);
                        ossimRefPtr<ossimCodecBase> codec;
                        switch ( tile.getTileType() )
                        {
                           case ossimGpkgTileRecord::OSSIM_GPKG_JPEG:
                           {
                              // we need to cache this instead of allocating a new codec every tile
                              // for now just getting it to compile with registry implementation
                              //
                              if( !m_jpegCodec.valid() )
                              {
                                 m_jpegCodec = ossimCodecFactoryRegistry::instance()->
                                    createCodec(ossimString("jpeg"));
                              }
                              
                              codec = m_jpegCodec.get();
                              break;
                           }
                           case ossimGpkgTileRecord::OSSIM_GPKG_PNG:
                           {
                              if( !m_pngCodec.valid() )
                              {
                                 m_pngCodec = ossimCodecFactoryRegistry::instance()->
                                    createCodec(ossimString("png"));
                              }
                              codec = m_pngCodec.get();
                              break;
                           }
                           default:
                           {
                              if (traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_WARN)
                                    << "Unhandled type: " << tile.getTileType() << endl;;
                              }
                              result = 0;
                              break;
                           }
                        }
                        
                        if ( codec.valid() )
                        {
                           if ( codec->decode(tile.m_tile_data, m_cacheTile ) )
                           {
                              result = m_cacheTile;
                           }
                           else
                           {
                              ossimNotify(ossimNotifyLevel_WARN)
                                 << "WARNING: decode failed...\n";
                           }
                        }
                        
                        if ( result.valid() )
                        {
                           // Set the tile origin in image space.
                           ossimIpt origin( index.x*tileSize.x,
                                            index.y*tileSize.y );
                           
                           // Subtract the sub image offset if any:
                           ossimIpt subImageOffset(0,0);
                           m_entries[m_currentEntry].getSubImageOffset( resLevel, subImageOffset );
                           origin -= subImageOffset;
                           
                           result->setOrigin( origin );
                        }
                        else if (traceDebug())
                        {
                           ossimNotify(ossimNotifyLevel_WARN)
                              << MODULE << " WARNING: result is null!\n";
                        }
                     }
                  }
                  
               } // Matches: if ( nCol )
               
            } // Matches: if ( rc == SQLITE_OK )

            sqlite3_finalize(pStmt);
            
         } // Matches: if(resLevel<m_entries[m_currentEntry].getTileMatrix().size())
         
      } // Matches: if ( m_currentEntry < m_entries.size() )
      
   } // Matches: if ( m_db )

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit result is " << (result.valid()?"set.":"null.") << "\n";
   }

   return result;
   
} // End: ossimGpkgReader::getTile( resLevel, index )

ossimRefPtr<ossimImageData> ossimGpkgReader::uncompressPngTile( const ossimGpkgTileRecord& tile,
                                                                const ossimIpt& tileSize )
{
   ossimRefPtr<ossimImageData> result = 0;

   //---
   // This copies the data to a stringbuf.  TODO, use tile data
   // directly.
   //---
   if ( tile.m_tile_data.size() )
   {
      std::string data( (char*)&tile.m_tile_data.front(),
                        tile.m_tile_data.size() );

      // ossim::istringstream is(data);
      std::shared_ptr<ossim::istream> is;

      std::shared_ptr<ossim::istringstream> testIs =
         std::make_shared<ossim::istringstream>();
      testIs->str( data );          
      is = testIs;                     
      if ( m_ih.valid() )
      {
         // Have an image handler from previous open:
         ossimStreamReaderInterface* sri =
            dynamic_cast<ossimStreamReaderInterface*>( m_ih.get() );
         if ( sri )
         {
            // if ( sri->open( &is, 0, false ) == false )
            if ( sri->open( is, std::string("gpkg_tile")) == false )
            {
               // Per the spec tile mime types can be mixed.
               m_ih = 0;
            }
         }
         else
         {
            m_ih = 0;
         }
      }
                           
      if ( !m_ih )
      {
         // m_ih = ossimImageHandlerRegistry::instance()->open( &is, 0, false );
         m_ih = ossimImageHandlerRegistry::instance()->open( is, std::string("gpkg_tile"), false );
      }
      
      if ( m_ih.valid() )
      {
         // Get the raw rectangle:
         ossimIrect rect( 0, 0, tileSize.x - 1, tileSize.y - 1 );
         
         // Get the tile from the image handler:
         result = m_ih->getTile( rect, 0 );
      }
   }

   return result;
   
} // ossimGpkgReader::uncompressPngTile

void ossimGpkgReader::computeGsd( ossimDpt& gsd) const
{
   if ( m_currentEntry < m_entries.size() )
   {
      m_entries[m_currentEntry].getGsd( 0, gsd );
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgReader::computeGsd DEBUG:\ngsd: " << gsd << "\n";   
   }
   
} // End: ossimGpkgReader::computeGsd

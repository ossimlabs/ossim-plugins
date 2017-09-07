//---
//
// File: ossimGpkgReader.cpp
//
// Author:  David Burken
//
// License: MIT
//
// Description: OSSIM Geo Package writer.
//
//---
// $Id$

#include "ossimGpkgWriter.h"
#include "ossimGpkgContentsRecord.h"
#include "ossimGpkgNsgTileMatrixExtentRecord.h"
#include "ossimGpkgSpatialRefSysRecord.h"
#include "ossimGpkgTileEntry.h"
#include "ossimGpkgTileRecord.h"
#include "ossimGpkgTileMatrixRecord.h"
#include "ossimGpkgTileMatrixSetRecord.h"
#include "ossimGpkgUtil.h"
#include "ossimSqliteUtil.h"

#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimViewInterface.h>
#include <ossim/base/ossimVisitor.h>

#include <ossim/imaging/ossimCodecFactoryRegistry.h>
#include <ossim/imaging/ossimImageCombiner.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimJpegMemDest.h>
#include <ossim/imaging/ossimRectangleCutFilter.h>
#include <ossim/imaging/ossimScalarRemapper.h>

#include <ossim/matrix/myexcept.h>

#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimEpsgProjectionFactory.h>
#include <ossim/projection/ossimGoogleProjection.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimMercatorProjection.h>

#include <jpeglib.h>
#include <sqlite3.h>

#include <algorithm> /* std::sort */
#include <cmath>
#include <sstream>

RTTI_DEF1(ossimGpkgWriter, "ossimGpkgWriter", ossimImageFileWriter)

static const std::string ADD_ALPHA_CHANNEL_KW          = "add_alpha_channel";
static const std::string ADD_ENTRY_KW                  = "add_entry";
static const std::string ADD_LEVELS_KW                 = "add_levels";
static const std::string ALIGN_TO_GRID_KW              = "align_to_grid";
static const std::string APPEND_KW                     = "append";
static const std::string BATCH_SIZE_KW                 = "batch_size";
static const std::string CLIP_EXTENTS_KW               = "clip_extents";
static const std::string CLIP_EXTENTS_ALIGN_TO_GRID_KW = "clip_extents_align_to_grid";
static const std::string COMPRESSION_LEVEL_KW          = "compression_level";
static const std::string DEFAULT_FILE_NAME             = "output.gpkg";
static const std::string EPSG_KW                       = "epsg";
static const std::string INCLUDE_BLANK_TILES_KW        = "include_blank_tiles";
static const std::string TILE_SIZE_KW                  = "tile_size";
static const std::string TILE_TABLE_NAME_KW            = "tile_table_name";
static const std::string TRUE_KW                       = "true";
static const std::string USE_PROJECTION_EXTENTS_KW     = "use_projection_extents";
static const std::string WRITER_MODE_KW                = "writer_mode";
static const std::string ZOOM_LEVELS_KW                = "zoom_levels";

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimGpkgWriter:debug" your_app_args
//---
static ossimTrace traceDebug("ossimGpkgWriter:debug");

// For the "ident" program:
#if OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimGpkgWriter.cpp 22466 2013-10-24 18:23:51Z dburken $";
#endif

ossimGpkgWriter::ossimGpkgWriter()
   :
   ossimImageFileWriter(),
   m_db(0),
   m_batchCount(0),
   m_batchSize(32),
   m_projectionBoundingRect(0.0, 0.0, 0.0, 0.0, OSSIM_RIGHT_HANDED),
   m_sceneBoundingRect(0.0, 0.0, 0.0, 0.0, OSSIM_RIGHT_HANDED),
   m_clipRect(0.0, 0.0, 0.0, 0.0, OSSIM_RIGHT_HANDED),
   m_outputRect(0.0, 0.0, 0.0, 0.0, OSSIM_RIGHT_HANDED),
   m_tileSize(0,0),
   m_tileTableName(),
   m_srs_id(-1),
   m_kwl(new ossimKeywordlist()),
   m_fullTileCodec(0),
   m_partialTileCodec(0),
   m_fullTileCodecAlpha(false),
   m_partialTileCodecAlpha(false),
   m_zoomLevels(),
   m_zoomLevelMatrixSizes(),
   m_pStmt(0),
   m_writeBlanks(false)
{
   //---
   // Uncomment for debug mode:
   // traceDebug.setTraceFlag(true);
   //---
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgWriter::ossimGpkgWriter entered" << std::endl;
#if OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif
   }

   theOutputImageType = "ossim_gpkg"; // ossimImageFileWriter attribute.

   // Set default options:
   m_kwl->addPair( ALIGN_TO_GRID_KW, TRUE_KW );
   m_kwl->addPair( TILE_SIZE_KW, std::string("( 256, 256 )") );
   m_kwl->addPair( WRITER_MODE_KW, std::string("mixed") );

   // Note batch size dramatically effects speed.
   m_kwl->addPair( BATCH_SIZE_KW, "32" );
}

ossimGpkgWriter::~ossimGpkgWriter()
{
   close();

   // Not a leak, ref ptr.
   m_kwl = 0;
}

ossimString ossimGpkgWriter::getShortName() const
{
   return ossimString("ossim_gpkg_writer");
}

ossimString ossimGpkgWriter::getLongName() const
{
   return ossimString("ossim gpkg writer");
}

ossimString ossimGpkgWriter::getClassName() const
{
   return ossimString("ossimGpkgWriter");
}

bool ossimGpkgWriter::isOpen() const
{
   return (m_db?true:false);
}

bool ossimGpkgWriter::open()
{
   bool status = false;

   close();
   
   if ( theFilename.size() )
   {
      int flags = SQLITE_OPEN_READWRITE;

      if ( theFilename.exists() )
      {
         if ( !append() )
         {
            theFilename.remove();
            flags |= SQLITE_OPEN_CREATE;
         }
      }
      else
      {
         flags |= SQLITE_OPEN_CREATE;
         
         //---
         // Set the append flags to false for down stream code since there was
         // no file to append.
         //---
         m_kwl->addPair( ADD_ENTRY_KW, std::string("0"), true );
         m_kwl->addPair( ADD_LEVELS_KW, std::string("0"), true );         
      }

      int rc = sqlite3_open_v2( theFilename.c_str(), &m_db, flags, 0 );
      if ( rc == SQLITE_OK )
      {
         status = true;

         if ( !append() )
         {
            //---
            // Set the application_id:
            // Requirement 2: Every GeoPackage must contain 0x47503130 ("GP10" in ACII)
            // in the application id field of the SQLite database header to indicate a
            // GeoPackage version 1.0 file.
            //---
            const ossim_uint32 ID = 0x47503130;
            std::ostringstream sql;
            sql << "PRAGMA application_id = " << ID;
            if ( ossim_sqlite::exec( m_db, sql.str() ) != SQLITE_DONE )
            {
               status = false;
            }
         }            
      }
      else
      {
         close();
      }
   }
   
   return status;
}

bool ossimGpkgWriter::openFile( const ossimKeywordlist& options )
{
   static const char MODULE[] = "ossimGpkgWriter::openFile";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   
   bool status = false;

   if ( isOpen() )
   {
      close();
   }

   // Add the passed in options to the default options.
   m_kwl->add( 0, options, true );

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "\nwriter options/settings:\n"
         << *(m_kwl.get()) << "\n";
   }

   // Get the filename:
   if ( getFilename( theFilename ) )
   {
      // Open it:
      if ( open() )
      {
         if ( m_db )
         {
            if ( createTables( m_db ) )
            {
               if ( !append() )
               {
                  // New file:
                  status = initializeGpkg();
               }
               else // Existing gpkg....
               {
                  // Get the zoom level info needed for disconnected writeTile checks.
                  std::string tileTableName;
                  getTileTableName( tileTableName );
         
                  ossimGpkgTileEntry entry;
                  if ( ossim_gpkg::getTileEntry( m_db, tileTableName, entry ) )
                  {
                     entry.getZoomLevels( m_zoomLevels );
                     entry.getZoomLevelMatrixSizes( m_zoomLevelMatrixSizes );
                     status = true;
                  }
               }

               if ( status )
               {
                  initializeCodec(); // Throws exception on error.

                  m_writeBlanks = keyIsTrue( INCLUDE_BLANK_TILES_KW );
                  m_batchSize = getBatchSize();
                  m_batchCount = 0;
               }
            }
         }
      }
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status: " << (status?"true":"false") << std::endl;
   }

   // status = true;

   return status;
}

// Method assumes new gpkg with no input connection.
bool ossimGpkgWriter::initializeGpkg()
{
   static const char MODULE[] = "ossimGpkgWriter::initializeGpkg";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   bool status = false;
   
   if ( alignToGrid() ) // Currently only handle align to grid mode.
   {
      // Set tile size:
      getTileSize( m_tileSize );

      // Output projection:
      ossimRefPtr<ossimMapProjection> proj = getNewOutputProjection();
      if ( proj.valid() )
      {
         // Initialize m_projectionBoundingRect.
         initializeProjectionRect( proj.get() );
            
         m_srs_id = writeGpkgSpatialRefSysTable( m_db, proj.get() );
         if ( m_srs_id != -1 )
         {
            // Initialize m_sceneBoundingRect:
            if ( getWmsCutBox( m_sceneBoundingRect ) == false )
            {
               m_sceneBoundingRect = m_projectionBoundingRect;
            }

            bool needToExpandFlag = true;
            //---
            // Set the initial tie point. will make the AOI come out
            // (0,0) to (n,n). Needed for getExpandedAoi code. Requires
            // m_sceneBoundingRect or m_projectionBoundingRect(grid aligned).
            //---
            setProjectionTie( proj.get() );
            // Set the clip rect.
            if ( getClipExtents( m_clipRect, needToExpandFlag ) == false )
            {
               m_clipRect = m_sceneBoundingRect.clipToRect( m_projectionBoundingRect );
            }
            
            // Get the zoom levels:
            m_zoomLevels.clear();
            m_zoomLevelMatrixSizes.clear();
            getZoomLevels( m_zoomLevels );

            ossim_uint32 levels = m_zoomLevels.size();

            if ( levels )
            {
               // Get the gsd for the most expanded out level:
               ossimDpt largestGsd;
               getGsd( proj.get(), m_zoomLevels[0], largestGsd );

               // Get the gsd for the most expanded out level:
               ossimDpt smallestGsd;
               getGsd( proj.get(), m_zoomLevels[ levels-1 ], smallestGsd );

               //---
               // Set the scale of projection to the largest gsd before
               // expanding AOI. This will also recenter projection tie
               // for new gsd.
               //---
               applyScaleToProjection( proj.get(), largestGsd );

               // AOI at zoomed out level:
               ossimIrect aoi;
               getAoiFromRect( proj.get(), m_clipRect, aoi );
               
               // Expand the AOI to even tile boundary:
               ossimIrect expandedAoi = aoi;
               if(needToExpandFlag)
               {
                  getExpandedAoi( aoi, expandedAoi );                  
               }
                        
               initializeRect( proj.get(), expandedAoi, m_outputRect );
 
               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << "\n\nfirst level:  " << m_zoomLevels[0]
                     << "\nlast level:   " << m_zoomLevels[ levels-1 ]
                     << "\nlevel[" << m_zoomLevels[0] << "] gsd: " << largestGsd
                     << "\nlevel[" << m_zoomLevels[levels-1] << "] gsd: " << smallestGsd
                     << "\nexpanded aoi(first zoom level): " << expandedAoi
                     << "\ntile size:    " << m_tileSize
                     << "\nscene rect:   " << m_sceneBoundingRect
                     << "\nclip rect:    " << m_clipRect
                     << "\noutput rect:  " << m_outputRect
                     << "\n";
               }

               if ( writeGpkgContentsTable( m_db, m_outputRect ) )
               {
                  if ( writeGpkgTileMatrixSetTable( m_db, m_outputRect ) )
                  {
                     ossimDpt gsd;
                     getGsd( proj.get(), gsd );
                     
                     // Write the tile matrix record for each zoom level.
                     std::vector<ossim_int32>::const_iterator zoomLevel = m_zoomLevels.begin();
                     while ( zoomLevel != m_zoomLevels.end() )
                     {
                        // Get the area of interest.
                        ossimIrect aoi;
                        getAoiFromRect( proj.get(), m_outputRect, aoi );
                        
                        // Clipped aoi:
                        ossimIrect clippedAoi;
                        getAoiFromRect( proj.get(), m_clipRect, clippedAoi );
                        
                        // Expanded to tile boundaries:
                        ossimIrect expandedAoi;
                        getExpandedAoi( aoi, expandedAoi );
                        
                        // Get the number of tiles:
                        ossimIpt matrixSize;
                        getMatrixSize( expandedAoi, matrixSize);

                        // Capture for writeTile index check.
                        m_zoomLevelMatrixSizes.push_back( matrixSize );

                        if (traceDebug())
                        {
                           ossimNotify(ossimNotifyLevel_DEBUG)
                              << "\nlevel:       " << (*zoomLevel)
                              << "\ngsd:         " << gsd
                              << "\naoi:         " << aoi
                              << "\nclippedAoi:  " << clippedAoi
                              << "\nexpandedAoi: " << expandedAoi
                              << "\nmatrixSize:  " << matrixSize
                              << "\n";
                        }

                        if ( writeGpkgTileMatrixTable( m_db, (*zoomLevel), matrixSize, gsd ) )
                        {
                           
                           if ( writeGpkgNsgTileMatrixExtentTable( m_db, (*zoomLevel),
                                                                   expandedAoi, clippedAoi ) )
                           {
                              status = true;
                           }
                           else
                           {
                              ossimNotify(ossimNotifyLevel_WARN)
                                 << MODULE
                                 << " WARNING:\nwriteGpkgNsgTileMatrixExtentTable call failed!"
                                 << std::endl;
                           }
                        }
                        else
                        {
                           ossimNotify(ossimNotifyLevel_WARN)
                              << MODULE
                              << " WARNING:\nwriteGpkgTileMatrixTable call failed!" << std::endl;
                        }
         
                        ++zoomLevel;
                        
                        if ( zoomLevel != m_zoomLevels.end() )
                        {
                           gsd = gsd / 2.0;
                           
                           ossimDpt scale( 0.5, 0.5 );
                           proj->applyScale( scale, true );
                           proj->update();
                        }
                        
                     } // Matches: while ( zoomLevel != zoomLevels.end() )

                  } // Matches: if ( writeGpkgTileMatrixSetTable( m_db, m_outputRect ) )
                  
               } // Matches: if ( writeGpkgContentsTable( m_db, m_outputRect ) )
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "Must have at least one zoom level!"
                  << "Set zoom_levels key in option keyword list."
                  << "e.g. \"zoom_levels:()4,5,6,7,8,9,10,11\""
                  << endl;
            }
         }
      }
   }
   else if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_WARN) << "Non-grid-aligned mode not supported!\n";
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status: " << (status?"true":"false") << std::endl;
   }

   return status;
}

void ossimGpkgWriter::close()
{
   if ( m_db )
   {
      sqlite3_close( m_db );
      m_db = 0;
   }
   m_fullTileCodec    = 0;
   m_partialTileCodec = 0;

   // ??? : if ( m_pStmt ) finalizeTileProcessing()

   m_pStmt = 0;
   m_batchCount = 0;
}

bool ossimGpkgWriter::writeFile()
{
   static const char MODULE[] = "ossimGpkgWriter::writeFile";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..."
         << "\nwriter options/settings:\n"
         << *(m_kwl.get()) << "\n";
   }
   
   bool status = false;
   
   m_batchSize = getBatchSize();
   m_batchCount = 0;

   try // Exceptions can be thrown...
   {
      if( theInputConnection.valid() && (getErrorStatus() == ossimErrorCodes::OSSIM_OK) )
      {
         //---
         // To hold the original input to the image source sequencer. Only used
         // if we mess with the input chain, e.g. add a scalar remapper.
         //---
         ossimRefPtr<ossimConnectableObject> originalSequencerInput = 0;

         //---
         // Set up input connection for eight bit if needed.
         // Some writers, e.g. jpeg are only 8 bit.
         //---
         if ( (theInputConnection->getOutputScalarType() != OSSIM_UINT8) &&
              requiresEightBit() )
         {
            originalSequencerInput = theInputConnection->getInput(0);

            ossimRefPtr<ossimImageSource> sr = new ossimScalarRemapper();

            // Connect scalar remapper to sequencer input.
            sr->connectMyInputTo(0, theInputConnection->getInput(0));

            // Connect sequencer to the scalar remapper.
            theInputConnection->connectMyInputTo(0, sr.get());
            theInputConnection->initialize();
         }
         
         // Note only the master process used for writing...
         if( theInputConnection->isMaster() )
         {
            if (!isOpen())
            {
               open();
            }

            if ( m_db )
            {
               status = createTables( m_db );
            }

            if ( status )
            {
               if ( keyIsTrue( ADD_LEVELS_KW ) )
               {
                  status = addLevels();
               }
               else
               {
                  status = writeEntry();
               }
            }
            
            close();
         }
         else // Matches: if( theInputConnection->isMaster() )
         {
            // Slave process only used to get tiles from the input.
            theInputConnection->slaveProcessTiles();
         }

         // Reset the connection if needed.
         if( originalSequencerInput.valid() )
         {
            theInputConnection->connectMyInputTo(0, originalSequencerInput.get());
            originalSequencerInput = 0;   
         }
         
      } // Matches: if ( theInputConnection.isValid() ... )
   }
   catch ( const ossimException& e )
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " Caught exception!\n"
         << e.what()
         << std::endl;
      status = false;
   }
   catch ( const RBD_COMMON::BaseException& me ) // Matrix exeption...
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " Caught Matrix exception!\n"
         << me.what()
         << std::endl;
      status = false;
   }
   catch ( ... )
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " Caught unknown exception!\n"
         << std::endl;
      status = false; 
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status: " << (status?"true":"false") << std::endl;
   }
   
   return status;
   
} // End: ossimGpkgWriter::writeFile

bool ossimGpkgWriter::writeEntry()
{
   static const char MODULE[] = "ossimGpkgWriter::writeEntry";

   bool status = false;

   // Get the image geometry from the input.   
   ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
   
   // Input projection:
   ossimRefPtr<ossimMapProjection> sourceProj = geom->getAsMapProjection();
   
   // Raw area of interest:
   ossimIrect sourceAoi = getAreaOfInterest();
   
   if ( geom.valid() && sourceProj.valid() && (sourceAoi.hasNans() == false) )
   {
      ossimRefPtr<ossimMapProjection> productProjection =
         getNewOutputProjection( geom.get() );
      
      if ( productProjection.valid() )
      {
         ossimDpt sourceGsd;
         getGsd( productProjection.get(), sourceGsd );
         
         bool gridAligned = alignToGrid();
         
         if ( gridAligned &&
              ( productProjection->getClassName() == "ossimUtmProjection" ) )
         {
            // Turn off grid alignment for utm.
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << MODULE << " WARNING!"
                  << "\"align_to_grid\" option is invalid a UTM projection and is "
                  << "being disabled..."
                  << std::endl;
            }
            
            m_kwl->addPair( ALIGN_TO_GRID_KW, std::string("0"), true );
            gridAligned = false;
         }
         
         // Initialize m_sceneBoundingRect:
         initializeRect( sourceProj.get(), sourceAoi, m_sceneBoundingRect );

         if ( gridAligned )
         {
            // Initialize m_projectionBoundingRect.  Only used if grid aligned.
            initializeProjectionRect( productProjection.get() );
         }

         //---
         // Set the initial tie point. will make the AOI come out
         // (0,0) to (n,n). Needed for getExpandedAoi code. Requires
         // m_sceneBoundingRect or m_projectionBoundingRect(grid aligned).
         //---
         setProjectionTie( productProjection.get() );
         
         if ( gridAligned )
         {
            // Set the clip rect.
            m_clipRect = m_sceneBoundingRect.clipToRect( m_projectionBoundingRect );
         }
         else
         {
            m_clipRect = m_sceneBoundingRect;
         }
         
         // Set tile size:
         getTileSize( m_tileSize );
         
         // Set the sequence tile size:
         theInputConnection->setTileSize( m_tileSize );
         
         // Get the first aoi from the clip rect.
         ossimIrect aoi;
         getAoiFromRect( productProjection.get(), m_clipRect, aoi );
         
         // Get the zoom levels to write:
         ossimDpt fullResGsd;
         std::vector<ossim_int32> zoomLevels;
         getZoomLevels( productProjection.get(),
                        aoi,
                        sourceGsd,
                        zoomLevels,
                        fullResGsd );
         
         if ( zoomLevels.size() )
         {
            //---
            // Start zoom level is full res.
            // Stop zoom level is overview.
            //---
            
            //---
            // Set the initial full res scale:
            // Must do this before the below call to
            // "ossimMapProjection::applyScale"
            //---
            bool isGeographic = productProjection->isGeographic();
            if ( isGeographic )
            {
               productProjection->setDecimalDegreesPerPixel( fullResGsd );
            }
            else
            {
               productProjection->setMetersPerPixel( fullResGsd );
            }

            // Recenter tie point after resolution change.
            setProjectionTie( productProjection.get() );
            
            // Stop gsd:
            ossimDpt stopGsd;
            getGsd( fullResGsd,
                    zoomLevels[zoomLevels.size()-1],
                    zoomLevels[0],
                    stopGsd );
            
            //---
            // Set the scale of projection to the largest gsd before
            // expanding AOI. This will also recenter projection tie
            // for new gsd.
            //---
            applyScaleToProjection( productProjection.get(), stopGsd );

            // propagate to chains.
            setView( productProjection.get() );
            
            // New aoi at zoomed out level:
            getAoiFromRect( productProjection.get(), m_clipRect, aoi );
            
            // Expand the AOI to even tile boundary:
            ossimIrect expandedAoi;
            getExpandedAoi( aoi, expandedAoi );
            
            initializeRect( productProjection.get(), expandedAoi, m_outputRect );
            
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "source aoi:     " << sourceAoi
                  << "\nproduct aoi:  " << aoi
                  << "\nexpanded aoi(last zoom level): " << expandedAoi
                  << "\ngsd:          " << fullResGsd
                  << "\nstop gsd:     " << stopGsd
                  << "\ntile size:    " << m_tileSize
                  << "\nscene rect:   " << m_sceneBoundingRect
                  << "\nclip rect:    " << m_clipRect
                  << "\noutput rect:  " << m_outputRect
                  << "\n";
               
               if ( gridAligned )
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << "\nproj rect:    " << m_projectionBoundingRect << "\n";
               }
            }
            
            m_srs_id = writeGpkgSpatialRefSysTable( m_db, productProjection.get() );
            if ( m_srs_id != -1 )
            {
               if ( writeGpkgContentsTable( m_db, m_outputRect ) )
               {
                  if ( writeGpkgTileMatrixSetTable( m_db, m_outputRect ) )
                  {
                     //---
                     // Note:
                     // Writer starts at "stop" zoom level(low res) and goes
                     // to "start"(high res).  I know naming is confusing!
                     //---
                     writeZoomLevels( m_db,
                                      productProjection.get(),
                                      zoomLevels );
                     status = true;
                  }
               }
            }
            
         } // Matches: if ( zoomLevels.size() )
         
      } // Matches: if ( productProjection.valid() )
      
   } // Matches: if ( geom.valid() && ... )

   return status;
               
} // End: ossimGpkgWriter::writeEntry()

bool ossimGpkgWriter::addLevels()
{
   static const char MODULE[] = "ossimGpkgWriter::addLevels";

   bool status = false;
   
   // Get the image geometry from the input.   
   ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
   
   // Input projection:
   ossimRefPtr<ossimMapProjection> sourceProj = geom->getAsMapProjection();
   
   // Raw area of interest:
   ossimIrect sourceAoi = getAreaOfInterest();
   
   if ( geom.valid() && sourceProj.valid() && (sourceAoi.hasNans() == false) )
   {
      ossimRefPtr<ossimMapProjection> productProjection =
         getNewOutputProjection( geom.get() );
      
      if ( productProjection.valid() )
      {
         std::string tileTableName;
         getTileTableName( tileTableName );
         
         ossimGpkgTileEntry entry;
         if ( ossim_gpkg::getTileEntry( m_db, tileTableName, entry ) )
         {
            // productProjection must match what's already in there.
            if ( entry.getSrs().m_organization_coordsys_id ==
                 (ossim_int32)productProjection->getPcsCode() )
            {
               ossimDpt sourceGsd;
               getGsd( productProjection.get(), sourceGsd );
            
               bool gridAligned = alignToGrid();
               
               if ( gridAligned &&
                    ( productProjection->getClassName() == "ossimUtmProjection" ) )
               {
                  // Turn off grid alignment for utm.
                  if (traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << MODULE << " WARNING!"
                        << "\"align_to_grid\" option is invalid a UTM projection and is "
                        << "being disabled..."
                        << std::endl;
                  }
                  
                  m_kwl->addPair( ALIGN_TO_GRID_KW, std::string("0"), true );
                  gridAligned = false;
               }
            
               // Initialize m_sceneBoundingRect:
               initializeRect( sourceProj.get(), sourceAoi, m_sceneBoundingRect );

               if ( gridAligned )
               {
                  // Initialize m_projectionBoundingRect.  Only used if grid aligned.
                  initializeProjectionRect( productProjection.get() );
               }

               //---
               // Set the initial tie point. Requires m_sceneBoundingRect or
               // m_projectionBoundingRect(grid aligned).
               //---
               setProjectionTie( productProjection.get() );

               // Pull the output rect from the existing entry.
               entry.getTileMatrixSet().getRect( m_outputRect );
               
               if ( gridAligned )
               {
                  // Set the clip rect.
                  m_clipRect = m_sceneBoundingRect.clipToRect( m_projectionBoundingRect );
               }
               else
               {
                  m_clipRect = m_sceneBoundingRect;
               }

               // Final clip rect to existing entry.
               m_clipRect = m_clipRect.clipToRect( m_outputRect );
               
               // Set tile size:
               getTileSize( m_tileSize );
               
               // Set the sequence tile size:
               theInputConnection->setTileSize( m_tileSize );
               
               // Get the first aoi from the clip rect.
               ossimIrect aoi;
               getAoiFromRect( productProjection.get(), m_clipRect, aoi );

               // Get the current(existing) zoom levels from the entry.
               std::vector<ossim_int32> currentZoomLevels;
               entry.getZoomLevels( currentZoomLevels );
               std::sort( currentZoomLevels.begin(), currentZoomLevels.end() );
               
               // Get the zoom levels to write:
               ossimDpt fullResGsd;
               std::vector<ossim_int32> zoomLevels;
               getZoomLevels( productProjection.get(),
                              aoi,
                              sourceGsd,
                              zoomLevels,
                              fullResGsd );

               // Sanity check.  Throws exception:
               checkLevels( currentZoomLevels, zoomLevels );
               
               if ( zoomLevels.size() )
               {
                  //---
                  // Start zoom level is full res.
                  // Stop zoom level is overview.
                  //---
                  
                  //---
                  // Set the initial full res scale:
                  // Must do this before the below call to
                  // "ossimMapProjection::applyScale"
                  //---
                  bool isGeographic = productProjection->isGeographic();
                  if ( isGeographic )
                  {
                     productProjection->setDecimalDegreesPerPixel( fullResGsd );
                  }
                  else
                  {
                     productProjection->setMetersPerPixel( fullResGsd );
                  }

                  // Recenter tie point after resolution change.
                  setProjectionTie( productProjection.get() );
                  
                  // Stop gsd:
                  ossimDpt stopGsd;
                  getGsd( fullResGsd,
                          zoomLevels[zoomLevels.size()-1],
                          zoomLevels[0],
                          stopGsd );

                  //---
                  // Set the scale of projection to the largest gsd before
                  // expanding AOI. This will also recenter projection tie
                  // for new gsd.
                  //---
                  applyScaleToProjection( productProjection.get(), stopGsd );
                  
                  // propagate to chains.
                  setView( productProjection.get() );
                  
                  if (traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << "source aoi:     " << sourceAoi
                        << "\nproduct aoi:  " << aoi
                        // << "\nexpanded aoi(last zoom level): " << expandedAoi
                        << "\ngsd:          " << fullResGsd
                        << "\nstop gsd:     " << stopGsd
                        << "\ntile size:    " << m_tileSize
                        << "\nscene rect:   " << m_sceneBoundingRect
                        << "\nclip rect:    " << m_clipRect
                        << "\noutput rect:  " << m_outputRect
                        << "\n";
                     
                     if ( gridAligned )
                     {
                        ossimNotify(ossimNotifyLevel_DEBUG)
                           << "\nproj rect:    " << m_projectionBoundingRect << "\n";
                     }
                  }
                  
                  //---
                  // Note:
                  // Writer starts at "stop" zoom level(low res) and goes
                  // to "start"(high res).  I know naming is confusing!
                  //---
                  writeZoomLevels( m_db,
                                   productProjection.get(),
                                   zoomLevels );
                  status = true;
               
               } // Matches: if ( zoomLevels.size() )

            } // Matches: if ( entry.getSrs() ...
            
         } // Matches: if ( productProjection.valid() )
         
      } // Matches: if ( ossim_gpkg::getTileEntry( m_db, ... )
      
   } // Matches: if ( geom.valid() && ... )

   return status;
               
} // End: ossimGpkgWriter::addLevels()

void ossimGpkgWriter::writeZoomLevels( sqlite3* db,
                                       ossimMapProjection* proj,
                                       const std::vector<ossim_int32>& zoomLevels )
// ossim_int32 fullResZoomLevel,
//                                     ossim_int32 stopZoomLevel )

{
   static const char MODULE[] = "ossimGpkgWriter::writeZoomLevels";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered..." << std::endl;
   }
   
   if ( db && proj )
   {
      initializeCodec(); // Throws exception on error.
      
      ossimDpt gsd;
      getGsd( proj, gsd );

      // To communicate the percent complete.
      ossim_float64 tilesWritten = 0.0;
      ossim_float64 totalTiles   = 0.0;

      std::vector<ossim_int32>::const_iterator zoomLevel = zoomLevels.begin();
      while ( zoomLevel != zoomLevels.end() )
      {
         // Get the area of interest.
         ossimIrect aoi;
         getAoiFromRect( proj, m_outputRect, aoi );

         // Clipped aoi:
         ossimIrect clippedAoi;
         getAoiFromRect( proj, m_clipRect, clippedAoi );

         // Expanded to tile boundaries:
         ossimIrect expandedAoi;
         getExpandedAoi( aoi, expandedAoi );
         
         // Get the number of tiles:
         ossimIpt matrixSize;
         getMatrixSize( expandedAoi, matrixSize);
         
         if ( totalTiles < 1 )
         {
            //---
            // First time through, compute total tiles for percent complete output.
            // This will be inaccurate if user skips zoom levels for some reason.
            //
            // NOTE: Numbers get very large, i.e. billions of tiles.
            // Was busting the int 32 boundary multiplying
            // (matrixSize.x * matrixSize.y).
            //---
            ossim_float64 x = matrixSize.x;
            ossim_float64 y = matrixSize.y;
            totalTiles = x * y;
            
            ossim_int32 levels = zoomLevels.size();
            if ( levels > 1 )
            {
               // Tile count doubles in each direction with each additional level.
               ossim_int32 z = 1;
               do
               {
                  x = x * 2.0;
                  y = y * 2.0;
                  totalTiles += (x * y);
                  ++z;
               } while ( z < levels );
            }

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "total tiles: " << totalTiles << "\n";
            }
         }
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimGpkgWriter::writeZoomLevels DEBUG:"
               << "\nlevel:       " << (*zoomLevel)
               << "\ngsd:         " << gsd
               << "\naoi:         " << aoi
               << "\nclippedAoi:  " << clippedAoi
               << "\nexpandedAoi: " << expandedAoi
               << "\nmatrixSize:  " << matrixSize
               << "\n";
         }

         if ( writeGpkgTileMatrixTable( db, (*zoomLevel), matrixSize, gsd ) )
         {
            
            if ( writeGpkgNsgTileMatrixExtentTable( db, (*zoomLevel),
                                                    expandedAoi, clippedAoi ) )
            {
               writeTiles( db, expandedAoi, (*zoomLevel), totalTiles, tilesWritten );
               // writeZoomLevel( db, expandedAoi, (*zoomLevel), totalTiles, tilesWritten );
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << MODULE
                  << " WARNING:\nwriteGpkgNsgTileMatrixExtentTable call failed!" << std::endl;
            }
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE
               << " WARNING:\nwriteGpkgTileMatrixTable call failed!" << std::endl;
         }
         
         if ( needsAborting() ) break;
         
         ++zoomLevel;
         
         if ( zoomLevel != zoomLevels.end() )
         {
            gsd = gsd / 2.0;
            
            ossimDpt scale( 0.5, 0.5 );
            proj->applyScale( scale, true );
            proj->update();
            
            // Propagate projection to chains and update aoi's of cutters.
            setView( proj );
         }
         
      } // Matches: while ( zoomLevel != zoomLevels.end() )
      
   } // Matches: if ( db && proj )
   
   if((m_batchCount < m_batchSize)&&(m_batchCount>0))
   {
      char * sErrMsg = 0;
      sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &sErrMsg);
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exited...\n";
   }
   
} // ossimGpkgWriter::writeZoomLevels( ... )

void ossimGpkgWriter::writeTiles( sqlite3* db,
                                  const ossimIrect& aoi,
                                  ossim_int32 zoomLevel,
                                  const ossim_float64& totalTiles,
                                  ossim_float64& tilesWritten )
{
   if ( db )
   {
      // Initialize the sequencer:
      theInputConnection->setAreaOfInterest( aoi );
      theInputConnection->setToStartOfSequence();

      const ossim_int64 ROWS = (ossim_int64)(theInputConnection->getNumberOfTilesVertical());
      const ossim_int64 COLS = (ossim_int64)(theInputConnection->getNumberOfTilesHorizontal());

      char * sErrMsg = 0;
      sqlite3_stmt* pStmt = 0; // The current SQL statement
      std::ostringstream sql;
      sql << "INSERT INTO " << m_tileTableName << "( zoom_level, tile_column, tile_row, tile_data ) VALUES ( "
          << "?, " // 1: zoom level
          << "?, " // 2: col
          << "?, " // 3: row
          << "?"   // 4: blob
          << " )";
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "sql:\n" << sql.str() << "\n";
      }
      
      int rc = sqlite3_prepare_v2(db,          // Database handle
                                  sql.str().c_str(), // SQL statement, UTF-8 encoded
                                  -1,          // Maximum length of zSql in bytes.
                                  &pStmt,      // OUT: Statement handle
                                  NULL);

      bool writeBlanks = keyIsTrue( INCLUDE_BLANK_TILES_KW );

      if(rc == SQLITE_OK)
      {
         for ( ossim_int64 row = 0; row < ROWS; ++row )
         {
            for ( ossim_int64 col = 0; col < COLS; ++col )
            {
               // Grab the tile.
               ossimRefPtr<ossimImageData> tile = theInputConnection->getNextTile();
               if ( tile.valid() )
               {
                  // Only write tiles that have data in them:
                  if (tile->getDataObjectStatus() != OSSIM_NULL )
                  {
                     if( (tile->getDataObjectStatus() != OSSIM_EMPTY) || writeBlanks )
                     {
                        if(m_batchCount == 0)
                        {
                           sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);
                        }

                        writeTile( pStmt, db, tile, zoomLevel, row, col);//, quality );
                        ++m_batchCount;

                        if(m_batchCount == m_batchSize)
                        {
                           sqlite3_exec(db, "END TRANSACTION", NULL, NULL, &sErrMsg);
                           m_batchCount = 0;
                        }
                     }
                  }
               }
               else
               {
                  std::ostringstream errMsg;
                  errMsg << "ossimGpkgWriter::writeTiles ERROR: "
                         << "Sequencer returned null tile pointer for ("
                         << col << ", " << row << ")";
                  
                  throw ossimException( errMsg.str() );
               }

               // Always increment the tiles written thing.
               ++tilesWritten;

               if ( needsAborting() ) break;
               
            } // End: col loop

            setPercentComplete( (tilesWritten / totalTiles) * 100.0 );

            if ( needsAborting() )
            {
               setPercentComplete( 100 );
               break;
            }
             
         } // End: row loop

         sqlite3_finalize(pStmt);
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
              << "sqlite3_prepare_v2 error: " << sqlite3_errmsg(db) << std::endl;
      }
     
   } // if ( db )

} // End: ossimGpkgWriter::writeTiles( ... )

void ossimGpkgWriter::writeTile( sqlite3_stmt* pStmt,
                                 sqlite3* db,
                                 ossimRefPtr<ossimImageData>& tile,
                                 ossim_int32 zoomLevel,
                                 ossim_int64 row,
                                 ossim_int64 col )
{
   if ( db && tile.valid() )
   {
      std::vector<ossim_uint8> codecTile; // To hold the jpeg encoded tile.
      bool encodeStatus;
      std::string ext;
      int mode = getWriterMode();

      if ( tile->getDataObjectStatus() == OSSIM_FULL )
      {
         if ( m_fullTileCodecAlpha )
         {
            tile->computeAlphaChannel();
         }
         encodeStatus = m_fullTileCodec->encode(tile, codecTile);
         if((mode == OSSIM_GPGK_WRITER_MODE_JPEG)||(mode == OSSIM_GPGK_WRITER_MODE_MIXED)) ext = ".jpg";
         else ext = ".png";
      }
      else
      {
         if ( m_partialTileCodecAlpha )
         {
            tile->computeAlphaChannel();
         }
         encodeStatus = m_partialTileCodec->encode(tile, codecTile);
         if(mode == OSSIM_GPGK_WRITER_MODE_JPEG) ext = ".jpg";
         else ext = ".png";
     }
      
      if ( encodeStatus )
      {
         // Insert into the database file(gpkg):
         int rc = sqlite3_bind_int (pStmt, 1, zoomLevel);
         rc |= sqlite3_bind_int (pStmt, 2, col);
         rc |= sqlite3_bind_int (pStmt, 3, row);
         rc |= sqlite3_bind_blob (pStmt,
                                  4,
                                  (void*)&codecTile.front(),
                                  codecTile.size(),
                                  SQLITE_TRANSIENT);
         if (  rc == SQLITE_OK )
         {
            rc = sqlite3_step(pStmt);
            if (  rc == SQLITE_OK )
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "sqlite3_step error: " << sqlite3_errmsg(db) << std::endl;
            }
            
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "sqlite3_bind_blob error: " << sqlite3_errmsg(db) << std::endl;
         }
         
         sqlite3_clear_bindings(pStmt);
         sqlite3_reset(pStmt);
      }
      
   } // Matches:  if ( db && tile.valid() )
}

void ossimGpkgWriter::writeCodecTile( sqlite3_stmt* pStmt,
                                 sqlite3* db,
                                 ossim_uint8* codecTile,
                                 ossim_int32 codecTileSize,
                                 ossim_int32 zoomLevel,
                                 ossim_int64 row,
                                 ossim_int64 col )
{
   if ( db && codecTile )
   {
      // Insert into the database file(gpkg):
      int rc = sqlite3_bind_int (pStmt, 1, zoomLevel);
      rc |= sqlite3_bind_int (pStmt, 2, col);
      rc |= sqlite3_bind_int (pStmt, 3, row);
      rc |= sqlite3_bind_blob (pStmt,
                               4,
                               (void*)codecTile,
                               codecTileSize,
                               SQLITE_TRANSIENT);
      if (  rc == SQLITE_OK )
      {
         rc = sqlite3_step(pStmt);
         if (  rc == SQLITE_OK )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "sqlite3_step error: " << sqlite3_errmsg(db) << std::endl;
         }
         
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "sqlite3_bind_blob error: " << sqlite3_errmsg(db) << std::endl;
      }
      
      sqlite3_clear_bindings(pStmt);
      sqlite3_reset(pStmt);
      
   } // Matches:  if ( db && tile.valid() )
}

// For connectionless write tiles:
ossim_int32 ossimGpkgWriter::beginTileProcessing()
{
   std::ostringstream sql;
   sql << "INSERT INTO " << m_tileTableName << "( zoom_level, tile_column, tile_row, tile_data ) VALUES ( "
       << "?, " // 1: zoom level
       << "?, " // 2: col
       << "?, " // 3: row
       << "?"   // 4: blob
       << " )";
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "sql:\n" << sql.str() << "\n";
   }
   
   return sqlite3_prepare_v2(m_db,              // Database handle
                             sql.str().c_str(), // SQL statement, UTF-8 encoded
                             -1,                // Maximum length of zSql in bytes.
                             &m_pStmt,          // OUT: Statement handle
                             NULL);
}
   
// For connectionless write tiles:
bool ossimGpkgWriter::writeTile( ossimRefPtr<ossimImageData>& tile,
                                 ossim_int32 zoomLevel,
                                 ossim_int64 row,
                                 ossim_int64 col )
{
   bool status = false;

   if ( tile.valid() )
   {
      // Check for valid zoom level:
      if ( isValidZoomLevelRowCol( zoomLevel, row, col ) )
      {
 
         // Only write tiles that have data in them:
         if (tile->getDataObjectStatus() != OSSIM_NULL )
         {
            if( (tile->getDataObjectStatus() != OSSIM_EMPTY) || m_writeBlanks )
            {
               char* sErrMsg = 0;
               
               if(m_batchCount == 0)
               {
                  sqlite3_exec(m_db, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);
               }
               
               writeTile( m_pStmt, m_db, tile, zoomLevel, row, col);//, quality );
               ++m_batchCount;
               
               if(m_batchCount == m_batchSize)
               {
                  sqlite3_exec(m_db, "END TRANSACTION", NULL, NULL, &sErrMsg);
                  m_batchCount = 0;
               }
            }
         }
         
         status = true;
      }
   }

   return status;
}

bool ossimGpkgWriter::writeCodecTile(ossim_uint8* codecTile,
                                     ossim_int32 codecTileSize,
                                     ossim_int32 zoomLevel,
                                     ossim_int64 row,
                                     ossim_int64 col)
{
   bool status = true;

   char* sErrMsg = 0;
   
   if(m_batchCount == 0)
   {
      sqlite3_exec(m_db, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);
   }
   
   writeCodecTile( m_pStmt, m_db, codecTile, codecTileSize, zoomLevel, row, col);//, quality );
   ++m_batchCount;
   
   if(m_batchCount == m_batchSize)
   {
      sqlite3_exec(m_db, "END TRANSACTION", NULL, NULL, &sErrMsg);
      m_batchCount = 0;
   }

   return status;
}
void ossimGpkgWriter::finalizeTileProcessing()
{
   if ( m_batchCount )
   {
      char* sErrMsg = 0;
      sqlite3_exec(m_db, "END TRANSACTION", NULL, NULL, &sErrMsg);
      m_batchCount = 0;
   }
   
   sqlite3_finalize(m_pStmt);
   m_pStmt = 0;
}
   
bool ossimGpkgWriter::createTables( sqlite3* db )
{
   bool status = false;
   if ( ossimGpkgSpatialRefSysRecord::createTable( db ) )
   {
      if ( ossimGpkgContentsRecord::createTable( db ) )
      {
         if ( ossimGpkgTileMatrixSetRecord::createTable( db ) )
         {
            if ( ossimGpkgNsgTileMatrixExtentRecord::createTable( db ) )
            {
               if ( ossimGpkgTileMatrixRecord::createTable( db ) )
               {
                  getTileTableName(m_tileTableName);
                  status = ossimGpkgTileRecord::createTable( db, m_tileTableName );
               }
            }
         }
      }
   }
   return status;
}

ossim_int32 ossimGpkgWriter::writeGpkgSpatialRefSysTable(
   sqlite3* db, const ossimMapProjection* proj )
{
   //---
   // NOTE: the "srs_id" is NOT synonomous with the m_organization_coordsys_id, e.g
   // 4326.  We need this so that other records can tie themselves to the correct
   // gpkg_spatial_ref_sys record.
   //---
   ossim_int32 srs_id = -1;
   if ( db && proj )
   {
      ossimGpkgSpatialRefSysRecord record;
      ossimGpkgSpatialRefSysRecord::InitCode returnCode = record.init( db, proj );
      if ( returnCode == ossimGpkgSpatialRefSysRecord::OK )
      {
         if ( record.insert( db ) )
         {
            srs_id = record.m_srs_id;
         }
      }
      else if ( returnCode == ossimGpkgSpatialRefSysRecord::OK_EXISTS )
      {
         // Record exists in database already(epsg:4326):
         srs_id = record.m_srs_id;
      }
      else if ( returnCode == ossimGpkgSpatialRefSysRecord::ERROR )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimGpkgWriter::writeGpkgSpatialRefSysTable ERROR initialized record!\n";
      }
   }
   return srs_id;
   
} // End: ossimGpkgWriter::writeGpkgSpatialRefSysTable

bool ossimGpkgWriter::writeGpkgContentsTable( sqlite3* db, const ossimDrect& rect )
{
   bool status = false;
   if ( db )
   {
      ossimGpkgContentsRecord record;
      ossimDpt minPt( rect.ul().x, rect.lr().y);
      ossimDpt maxPt( rect.lr().x, rect.ul().y);

      if ( record.init( m_tileTableName, m_srs_id, minPt, maxPt ) )
      {
         status = record.insert( db );
      }  
   }
   return status;
}

bool ossimGpkgWriter::writeGpkgTileMatrixSetTable( sqlite3* db, const ossimDrect& rect )
{
   bool status = false;
   if ( db )
   {
      ossimGpkgTileMatrixSetRecord record;
      ossimDpt minPt( rect.ul().x, rect.lr().y);
      ossimDpt maxPt( rect.lr().x, rect.ul().y);
      
      if ( record.init( m_tileTableName, m_srs_id, minPt, maxPt ) )
      {
         status = record.insert( db );
      }  
   }
   return status;
   
} // End: ossimGpkgWriter::writeGpkgTileMatrixSetTable

bool ossimGpkgWriter::writeGpkgTileMatrixTable( sqlite3* db,
                                                ossim_int32 zoom_level,
                                                const ossimIpt& matrixSize,
                                                const ossimDpt& gsd )
{
   bool status = false;
   if ( db )
   {
      ossimGpkgTileMatrixRecord record;
      if ( record.init( m_tileTableName, zoom_level, matrixSize, m_tileSize, gsd ) )
      {
         status = record.insert( db );
      }  
   }
   return status;
   
} // End: ossimGpkgWriter::writeGpkgTileMatrixTable

bool ossimGpkgWriter::writeGpkgNsgTileMatrixExtentTable( sqlite3* db,
                                                         ossim_int32 zoom_level,
                                                         const ossimIrect& expandedAoi,
                                                         const ossimIrect& clippedAoi )
{
   bool status = false;
   if ( db )
   {
      // Compute the image rect:
      ossimIrect imageRect( clippedAoi.ul().x - expandedAoi.ul().x,
                            clippedAoi.ul().y - expandedAoi.ul().y,
                            clippedAoi.lr().x - expandedAoi.ul().x,
                            clippedAoi.lr().y - expandedAoi.ul().y );
      
      ossimGpkgNsgTileMatrixExtentRecord record;
      if ( record.init( m_tileTableName, zoom_level, imageRect, m_clipRect ) )
      {
         status = record.insert( db );
      } 
   }
   return status;
   
} // ossimGpkgWriter::writeGpkgNsgTileMatrixExtentTable( ... )

bool ossimGpkgWriter::saveState(ossimKeywordlist& kwl, const char* prefix) const
{
   if ( m_kwl.valid() )
   {
      // Lazy man save state...
      kwl.add( prefix, *(m_kwl.get()), true );
   }
   return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimGpkgWriter::loadState(const ossimKeywordlist& kwl, const char* prefix) 
{
   if ( m_kwl.valid() )
   {
      ossimString regularExpression;
      if ( prefix )
      {
         regularExpression = prefix;
      }
      
      regularExpression += "*";
      kwl.extractKeysThatMatch( *(m_kwl.get()), regularExpression );
      
      if ( prefix )
      {
         regularExpression = prefix;
         m_kwl->stripPrefixFromAll( regularExpression );
      }
   }
   
   return ossimImageFileWriter::loadState(kwl, prefix);
}

void ossimGpkgWriter::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back(ossimString("ossim_gpkg"));
}

ossimString ossimGpkgWriter::getExtension() const
{
   return ossimString("gpkg");
}

bool ossimGpkgWriter::hasImageType(const ossimString& imageType) const
{
   if ( (imageType == "ossim_gpkg") || (imageType == "image/gpkg") )
   {
      return true;
   }

   return false;
}

void ossimGpkgWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if ( property.valid() )
   {
      // See if it's one of our properties:
      std::string key = property->getName().string();
      if ( ( key == ADD_ALPHA_CHANNEL_KW ) ||
           ( key == ADD_ENTRY_KW ) ||
           ( key == ADD_LEVELS_KW ) ||
           ( key == ALIGN_TO_GRID_KW ) ||
           ( key == APPEND_KW ) ||           
           ( key == BATCH_SIZE_KW ) ||
           ( key == COMPRESSION_LEVEL_KW ) ||
           ( key == ossimKeywordNames::COMPRESSION_QUALITY_KW ) ||
           ( key == EPSG_KW ) ||
           ( key == INCLUDE_BLANK_TILES_KW ) ||
           ( key == TILE_SIZE_KW ) ||
           ( key == TILE_TABLE_NAME_KW ) ||           
           ( key == WRITER_MODE_KW ) ||
           ( key == ZOOM_LEVELS_KW ) )
      {
         ossimString value;
         property->valueToString(value);
         m_kwl->addPair( key, value.string(), true );
      }
      else
      {
         ossimImageFileWriter::setProperty(property);
      }
   }
}

ossimRefPtr<ossimProperty> ossimGpkgWriter::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> prop = 0;
   prop = ossimImageFileWriter::getProperty(name);
   return prop;
}

void ossimGpkgWriter::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(ossimString(ADD_ALPHA_CHANNEL_KW));
   propertyNames.push_back(ossimString(ADD_ENTRY_KW));
   propertyNames.push_back(ossimString(ADD_LEVELS_KW));      
   propertyNames.push_back(ossimString(ALIGN_TO_GRID_KW));
   propertyNames.push_back(ossimString(APPEND_KW));   
   propertyNames.push_back(ossimString(BATCH_SIZE_KW));
   propertyNames.push_back(ossimString(COMPRESSION_LEVEL_KW));
   propertyNames.push_back(ossimString(ossimKeywordNames::COMPRESSION_QUALITY_KW));
   propertyNames.push_back(ossimString(EPSG_KW));
   propertyNames.push_back(ossimString(INCLUDE_BLANK_TILES_KW));
   propertyNames.push_back(ossimString(TILE_SIZE_KW));
   propertyNames.push_back(ossimString(TILE_TABLE_NAME_KW));
   propertyNames.push_back(ossimString(WRITER_MODE_KW));
   propertyNames.push_back(ossimString(ZOOM_LEVELS_KW));

   ossimImageFileWriter::getPropertyNames(propertyNames);
}

void ossimGpkgWriter::setCompressionQuality(  const std::string& quality )
{
   m_kwl->addPair( std::string(ossimKeywordNames::COMPRESSION_QUALITY_KW),
                   quality );
}

ossim_uint32 ossimGpkgWriter::getCompressionQuality() const
{
   ossim_uint32 quality = 0;
   std::string value = m_kwl->findKey( std::string(ossimKeywordNames::COMPRESSION_QUALITY_KW) );
   if ( value.size() )
   {
      quality = ossimString(value).toUInt32();
   }
   return quality;
}

ossimString ossimGpkgWriter::getCompressionLevel() const
{
   ossimString result = ossimString("z_default_compression");
#if 0
   switch (theCompressionLevel)
   {
      case Z_NO_COMPRESSION:
         result = ossimString("z_no_compression");
         break;
         
      case Z_BEST_SPEED:
         result = ossimString("z_best_speed");
         break;
         
      case Z_BEST_COMPRESSION:
         result = ossimString("z_best_compression");
         break;

      default:
         break;
   }
#endif
   return result;
}

bool ossimGpkgWriter::setCompressionLevel(const ossimString& /* level */ )
{
   bool status = true;
#if 0
   ossimString s = level;
   s.downcase();

   if(s == "z_no_compression")
   {
      theCompressionLevel = Z_NO_COMPRESSION;
   }
   else if(s == "z_best_speed")
   {
      theCompressionLevel = Z_BEST_SPEED;
   }
   else if(s == "z_best_compression")
   {
      theCompressionLevel = Z_BEST_COMPRESSION;
   }
   else if(s == "z_default_compression")
   {
      theCompressionLevel = Z_DEFAULT_COMPRESSION;
   }
   else
   {
      status = false;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "DEBUG:"
         << "\nossimGpkgWriter::setCompressionLevel DEBUG"
         << "passed in level:  " << level.c_str()
         << "writer level: " << getCompressionLevel().c_str()
         << std::endl;
   }
#endif
   return status;
}

void ossimGpkgWriter::getGsd( const ossimDpt& fullResGsd,
                              ossim_int32 fullResZoomLevel,
                              ossim_int32 currentZoomLevel,
                              ossimDpt& gsd )
{
   if ( fullResGsd.hasNans() == false )
   {
      double delta = fullResZoomLevel - currentZoomLevel;
      if ( delta > 0 )
      {
         gsd = fullResGsd * ( std::pow( 2.0, delta ) );
      }
      else if ( delta < 0 )
      {
         gsd = fullResGsd / ( std::pow( 2, std::fabs(delta) ) );
      }
      else
      {
         gsd = fullResGsd;
      }
   }
}

void ossimGpkgWriter::getGsd( const ossimMapProjection* proj,
                              ossimDpt& gsd ) const
{
   if ( proj )
   {
      if ( proj->isGeographic() )
      {
         gsd = proj->getDecimalDegreesPerPixel();
      }
      else
      {
         gsd = proj->getMetersPerPixel();
      }  
   }
}

void ossimGpkgWriter::getGsd( const ossimImageGeometry* geom,
                              ossimDpt& gsd ) const
{
   if ( geom )
   {
      const ossimMapProjection* proj = geom->getAsMapProjection();
      if ( proj )
      {
         if ( proj->isGeographic() )
         {
            geom->getDegreesPerPixel( gsd );
         }
         else
         {
            geom->getMetersPerPixel( gsd );
         }
      }
   }
}

void ossimGpkgWriter::getGsd(
   const ossimMapProjection* proj, ossim_int32 zoomLevel, ossimDpt& gsd ) const
{
   if ( proj )
   {
      if ( alignToGrid() )
      {
         ossimDpt dims;
         if ( proj->isGeographic() )
         {
            dims.x = 360.0;
            dims.y = 180.0;

            gsd.x = 360.0/(m_tileSize.x*2);
            gsd.y = 180.0/m_tileSize.y;
               
         }
         else
         {
            getProjectionDimensionsInMeters( proj, dims );

            // Gsd that puts Earth in one tile:
            gsd.x = dims.x/m_tileSize.x;
            gsd.y = dims.y/m_tileSize.y;
         }

         if ( zoomLevel )
         {
            gsd = gsd / ( std::pow( 2.0, zoomLevel ) );
         }
      }
      else
      {
         gsd.makeNan();
         
         // Error message??? (drb)
      }
   }
}

bool ossimGpkgWriter::alignToGrid() const
{
   return keyIsTrue( ALIGN_TO_GRID_KW );
}

bool ossimGpkgWriter::append() const
{
   return ( keyIsTrue( ADD_ENTRY_KW ) || keyIsTrue( ADD_LEVELS_KW ) || keyIsTrue( APPEND_KW ) );
}

bool ossimGpkgWriter::keyIsTrue( const std::string& key ) const
{ 
   bool result = false;
   std::string value = m_kwl->findKey( key );
   if ( value.size() )
   {
      result = ossimString(value).toBool();
   }
   return result;
}

void ossimGpkgWriter::setView( ossimMapProjection* proj )
{
   if ( theInputConnection.valid() && proj )
   {
      ossimTypeNameVisitor visitor( ossimString("ossimViewInterface"),
                                    false, // firstofTypeFlag
                                    (ossimVisitor::VISIT_INPUTS|
                                     ossimVisitor::VISIT_CHILDREN) );
      theInputConnection->accept( visitor );
      if ( visitor.getObjects().size() )
      {
         for( ossim_uint32 i = 0; i < visitor.getObjects().size(); ++i )
         {
            ossimViewInterface* viewClient = visitor.getObjectAs<ossimViewInterface>( i );
            if (viewClient)
            {
               viewClient->setView( proj );
            }
         }

         //---
         // After a view change the combiners must reset their input rectangles
         // for each image.
         //---
         reInitializeCombiners();

         //---
         // Cutter, if present, must be updated since the view has been
         // changed and the cutter's AOI is no longer relative.  Note 
         // the original AOI was already saved for our writer.
         //---
         reInitializeCutters( proj );
         
         theInputConnection->initialize();
      }
   }
}

void ossimGpkgWriter::reInitializeCombiners()
{
   if ( theInputConnection.valid() )
   {
      ossimTypeNameVisitor visitor( ossimString("ossimImageCombiner"),
                                    false, // firstofTypeFlag
                                    (ossimVisitor::VISIT_INPUTS|
                                     ossimVisitor::VISIT_CHILDREN) );

      theInputConnection->accept( visitor );
      if ( visitor.getObjects().size() )
      {
         for( ossim_uint32 i = 0; i < visitor.getObjects().size(); ++i )
         {
            ossimImageCombiner* combiner = visitor.getObjectAs<ossimImageCombiner>( i );
            if (combiner)
            {
               combiner->initialize();
            }
         }
      }
   }
}

void ossimGpkgWriter::reInitializeCutters( const ossimMapProjection* proj )
{
   if ( theInputConnection.valid() && proj )
   {
      ossimTypeNameVisitor visitor( ossimString("ossimRectangleCutFilter"),
                                    false, // firstofTypeFlag
                                    (ossimVisitor::VISIT_INPUTS|
                                     ossimVisitor::VISIT_CHILDREN) );

      theInputConnection->accept( visitor );
      if ( visitor.getObjects().size() )
      {
         ossimIrect rect;
         getAoiFromRect( proj, m_clipRect, rect );
         
         for( ossim_uint32 i = 0; i < visitor.getObjects().size(); ++i )
         {
            ossimRectangleCutFilter* cutter = visitor.getObjectAs<ossimRectangleCutFilter>( i );
            if (cutter)
            {
               // Set the clip rect of the cutter.
               cutter->setRectangle(rect);

               // Enable the getTile...
               cutter->setEnableFlag( true );
            }
         }
      }
   }
}

ossimRefPtr<ossimMapProjection> ossimGpkgWriter::getNewOutputProjection(
   ossimImageGeometry* geom ) const
{
   ossimRefPtr<ossimMapProjection> proj = 0;

   if ( geom )
   {
      // "epsg" is a writer prop so check for it.  This overrides the input projection.
      proj = getNewOutputProjection();

      if ( proj.valid() == false )
      {
         ossimRefPtr<ossimMapProjection> sourceProj = geom->getAsMapProjection();
         if ( sourceProj.valid() )
         {
            // Check the input projection.  This could have been set by the caller.
            if ( sourceProj->getClassName() == "ossimEquDistCylProjection" )
            {
               // This will be an equdist with origin at 0, 0.
               proj = getNewGeographicProjection();
            }
            else if ( sourceProj->getClassName() == "ossimMercatorProjection" )
            {
               // WGS 84 / World Mercator:
               proj = getNewWorldMercatorProjection();
            }
            else if ( sourceProj->getClassName() == "ossimGoogleProjection" )
            {
               proj = new ossimGoogleProjection(); // sourceProj;
            }
            else if ( sourceProj->getClassName() == "ossimUtmProjection" )
            {
               proj = dynamic_cast<ossimMapProjection*>(sourceProj->dup());
            }
         }

         // Final default:
         if ( proj.valid() == false )
         {
            //---
            // DEFAULT: Geographic, WGS 84
            // Note: May need to switch default to ossimMercatorProjection:
            //---
            proj = getNewGeographicProjection();
         }
      }

      if ( proj.valid() )
      {
         bool isGeographic = proj->isGeographic();
         bool gridAligned = alignToGrid();

         // Set the gsd:
         ossimDpt fullResGsd;
         getGsd( geom, fullResGsd );
         
         if ( isGeographic )
         {
            if ( gridAligned )
            {
               // Make pixels square if not already.
               if ( fullResGsd.y < fullResGsd.x )
               {
                  fullResGsd.x = fullResGsd.y;
               }
               else if ( fullResGsd.x < fullResGsd.y )
               {
                  fullResGsd.y = fullResGsd.x; 
               }
            }
            proj->setDecimalDegreesPerPixel( fullResGsd );
         }
         else
         {
            if ( gridAligned && (proj->getClassName() == "ossimUtmProjection" ) )
            {
               // Turn off grid alignment for utm.
               gridAligned = false;
            }
            
            if ( gridAligned )
            {
               // Make pixels square if not already.
               if ( fullResGsd.y < fullResGsd.x )
               {
                  fullResGsd.x = fullResGsd.y;
               }
               else if ( fullResGsd.x < fullResGsd.y )
               {
                  fullResGsd.y = fullResGsd.x; 
               }
            }
            proj->setMetersPerPixel( fullResGsd );
         }
      }
      
   } // Matches: if ( geom )
   
   return proj;
   
} // End: ossimGpkgWriter::getNewOutputProjection( geom )

ossimRefPtr<ossimMapProjection> ossimGpkgWriter::getNewOutputProjection() const
{
   ossimRefPtr<ossimMapProjection> proj = 0;

   // "epsg" is a writer prop so check for it.  This overrides the input projection.
   ossim_uint32 epsgCode = getEpsgCode();
   if ( epsgCode )
   {
      if ( epsgCode ==  4326 )
      {
         // Geographic, WGS 84
         proj = getNewGeographicProjection();
      }
      else if ( epsgCode == 3395 )
      {
         // WGS 84 / World Mercator:
         proj = getNewWorldMercatorProjection();
      }
      else if ( ( epsgCode == 3857 ) || ( epsgCode == 900913) )
      {
         proj = new ossimGoogleProjection();
      }
      else
      {
         // Go to the factory:
         ossimString name = "EPSG:";
         name += ossimString::toString(epsgCode);
         ossimRefPtr<ossimProjection> proj =
            ossimEpsgProjectionFactory::instance()->createProjection(name);
         if ( proj.valid() )
         {
            proj = dynamic_cast<ossimMapProjection*>( proj.get() );
         }
      }
   }

   return proj;
   
} // End: ossimGpkgWriter::getNewOutputProjection()

ossimRefPtr<ossimMapProjection> ossimGpkgWriter::getNewGeographicProjection() const
{
   // Geographic, WGS 84, with origin at 0,0 for square pixels in decimal degrees.
   ossimRefPtr<ossimMapProjection> result =
      new ossimEquDistCylProjection(
         ossimEllipsoid(),
         ossimGpt(0.0, 0.0, 0.0, ossimDatumFactory::instance()->wgs84()) );
   return result;
}

ossimRefPtr<ossimMapProjection> ossimGpkgWriter::getNewWorldMercatorProjection() const
{
   // EPSG: 3395, "WGS 84 / World Mercator", with origin at 0,0.
   ossimRefPtr<ossimMapProjection> result =
      new ossimMercatorProjection(
         ossimEllipsoid(),
         ossimGpt(0.0, 0.0, 0.0, ossimDatumFactory::instance()->wgs84()) );

   // Set the pcs(epsg) code:
   result->setPcsCode( 3395 );
   
   return result;
}

void ossimGpkgWriter::getTileSize( ossimIpt& tileSize ) const
{
   std::string value = m_kwl->findKey( TILE_SIZE_KW );
   if ( value.size() )
   {
      tileSize.toPoint( value );
   }
   else
   {
      ossim::defaultTileSize( tileSize );
   }
}

ossim_uint64 ossimGpkgWriter::getBatchSize() const
{
   ossim_uint64 size = 32; // ???
   std::string value = m_kwl->findKey( BATCH_SIZE_KW );
   if ( value.size() )
   {
      size = ossimString(value).toUInt64();
   }
   return size;
}

void ossimGpkgWriter::getZoomLevels( std::vector<ossim_int32>& zoomLevels ) const
{
   std::string value = m_kwl->findKey( ZOOM_LEVELS_KW );
   if ( value.size() )
   {
      ossimString stringOfPoints(value);
      if ( ossim::toSimpleVector(zoomLevels, stringOfPoints) )
      {
         std::sort( zoomLevels.begin(), zoomLevels.end() );

         // Check for negative and disallow.
         if ( zoomLevels[0] < 0 )
         {
            zoomLevels.clear();

            // Warning message???
         }
      }
      else
      {
         zoomLevels.clear();
      }
   }
   else
   {
      zoomLevels.clear();
   }
}


void ossimGpkgWriter::getZoomLevels( const ossimMapProjection* proj,
                                     const ossimIrect& aoi,
                                     const ossimDpt& sourceGsd,
                                     std::vector<ossim_int32>& zoomLevels,
                                     ossimDpt& fullResGsd ) const
{
   if ( proj && (aoi.hasNans() == false) )
   {
      // Initial assignment of full res gsd. Will change if aligned to grid is on.
      fullResGsd = sourceGsd;
      
      // Check for user specified levels.
      getZoomLevels( zoomLevels );
      
      if ( zoomLevels.size() )
      {
         if ( alignToGrid() )
         {
            ossim_int32 zoomLevel = zoomLevels[zoomLevels.size()-1];
            if ( proj->isGeographic() )
            {
               fullResGsd.x = 360.0/(m_tileSize.x*2);
               fullResGsd.y = 180.0/m_tileSize.y;
            }
            else
            {
               ossimDpt dims;
               getProjectionDimensionsInMeters( proj, dims );
               
               // Gsd that puts Earth in one tile:
               fullResGsd.x = dims.x/m_tileSize.x;
               fullResGsd.y = dims.y/m_tileSize.y;
            }

            if ( zoomLevel )
            {
               fullResGsd = fullResGsd / ( std::pow( 2.0, zoomLevel ) );
            }
         }
      }
      else
      {
         ossim_int32 levels = getNumberOfZoomLevels( aoi );
         if ( levels )
         {
            if ( alignToGrid() )
            {
               ossimDpt zoomGsd;
               
               if ( proj->isGeographic() )
               {
                  zoomGsd.x = 360.0/(m_tileSize.x*2);
                  zoomGsd.y = 180.0/m_tileSize.y;
               }
               else
               {
                  ossimDpt dims;
                  getProjectionDimensionsInMeters( proj, dims );
                  
                  // Gsd that puts Earth in one tile:
                  zoomGsd.x = dims.x/m_tileSize.x;
                  zoomGsd.y = dims.y/m_tileSize.y;
               }
               
               if ( fullResGsd.hasNans() == false )
               {
                  // If zoom level gsd is below this threshold we stop there.
                  ossimDpt gsdThreshold = fullResGsd * 1.5;
                  
                  // Start full Earth in 2x1 tiles:
                  ossim_int32 startZoomLevel = 0;
                  while ( ( zoomGsd.x > gsdThreshold.x ) &&
                          ( zoomGsd.y > gsdThreshold.y ) )
                  {
                     // Go to next level.
                     zoomGsd = zoomGsd/2.0;
                     fullResGsd = zoomGsd;
                     ++startZoomLevel;
                  }
                  
                  ossim_int32 stopZoomLevel = startZoomLevel-(levels-1);
                  
                  if ( stopZoomLevel < 0 ) stopZoomLevel = 0;
                  
                  for ( ossim_int32 i = stopZoomLevel; i <= startZoomLevel; ++i )
                  {
                     zoomLevels.push_back(i);
                  }
               }
            }
            else // Not grid aligned.
            {
               //---
               // If not grid aligned the full res gsd is the inputs which is
               // already set.
               //---
               for ( ossim_int32 i = 0; i < levels; ++i )
               {
                  zoomLevels.push_back(i);
               }
            }          
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgWriter::getZoomLevels DEBUG"
         << "aoi: " << aoi << "\n"
         << "sourceGsd: " << sourceGsd << "\n"
         << "\nfullResGsd:     " << fullResGsd << "\n"
         << "levels: (";
      std::vector<ossim_int32>::const_iterator i = zoomLevels.begin();
      while ( i != zoomLevels.end() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << (*i);
         ++i;
         if ( i != zoomLevels.end() )
         {
            ossimNotify(ossimNotifyLevel_DEBUG) << ",";
         }
         else
         {
            ossimNotify(ossimNotifyLevel_DEBUG) << ")\n";
         }
      }
   }
   
} // End: ossimGpkgWriter::getZoomLevels( proj, aoi, ... )

ossim_int32 ossimGpkgWriter::getNumberOfZoomLevels( const ossimIrect& aoi ) const
{
   ossim_int32 result = 0;

   if ( aoi.hasNans() == false )
   {
      ossim_float64 w = aoi.width();
      ossim_float64 h = aoi.height();

      // Take it down to at least a quarter of a tile.
      const ossim_float64 TW = m_tileSize.x/4;
      const ossim_float64 TH = m_tileSize.y/4;
      if ( w && h )
      {
         ++result; // At least one level.
         while ( ( TW < w ) && ( TH < h ) )
         {
            w /= 2.0;
            h /= 2.0;
            ++result;
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgWriter::getNumberOfZoomLevels DEBUG"
         << "\nlevels: " << result << "\n";
   }
   
   return result;
}

void ossimGpkgWriter::getAoiFromRect( const ossimMapProjection* proj,
                                      const ossimDrect& rect,
                                      ossimIrect& aoi )
{
   static const char MODULE[] = "ossimGpkgWriter::getAoi";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   
   // Take the aoi edges(minPt, maxPt), shift to center pixel and return the aoi.
   if ( proj )
   {
      ossimDpt gsd;
      getGsd( proj, gsd );
      ossimDpt halfGsd = gsd/2.0;
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << "gsd: " << gsd << "\n";
      }

      ossimDpt ulDpt;
      ossimDpt lrDpt;
      
      if ( proj->isGeographic() )
      {
         // Convert the ground points to view space.
         ossimGpt ulGpt( rect.ul().y-halfGsd.y, rect.ul().x+halfGsd.x, 0.0 );
         ossimGpt lrGpt( rect.lr().y+halfGsd.y, rect.lr().x-halfGsd.x, 0.0 );

         // Get the view coords of the aoi.
         proj->worldToLineSample(ulGpt, ulDpt);
         proj->worldToLineSample(lrGpt, lrDpt);

         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "\nulGpt: " << ulGpt
               << "\nlrGpt: " << lrGpt
               << "\nulDpt: " << ulDpt
               << "\nlrDpt: " << lrDpt
               << "\n";
         }
      }
      else
      {
         ossimDpt ulEnPt( rect.ul().x+halfGsd.x, rect.ul().y-halfGsd.y );
         ossimDpt lrEnPt( rect.lr().x-halfGsd.x, rect.lr().y+halfGsd.y );

         // Get the view coords of the aoi.
         proj->eastingNorthingToLineSample( ulEnPt, ulDpt );
         proj->eastingNorthingToLineSample( lrEnPt, lrDpt );
      }

      // Area of interest in view space on point boundaries.
      aoi = ossimIrect( ossimIpt(ulDpt), ossimIpt(lrDpt) );

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "aoi: " << aoi << "\n"
            << MODULE << " exited...\n";
      }
   }
   
} // End: ossimGpkgWriter::getAoiFromRect( ... )

void ossimGpkgWriter::getExpandedAoi( const ossimIrect& aoi,
                                      ossimIrect& expandedAoi ) const
{
   expandedAoi = aoi;
   expandedAoi.stretchToTileBoundary( m_tileSize );
}

void ossimGpkgWriter::getMatrixSize(
   const ossimIrect& rect, ossimIpt& matrixSize ) const
{
   matrixSize.x = rect.width()/m_tileSize.x;
   if ( rect.width() % m_tileSize.x )
   {
      ++matrixSize.x;
   }
   matrixSize.y = rect.height()/m_tileSize.y;
   if ( rect.height() % m_tileSize.y )
   {
      ++matrixSize.y;
   }
}

void ossimGpkgWriter::setProjectionTie( ossimMapProjection* proj ) const
{
   if ( proj )
   {
      ossimDpt gsd;
      getGsd( proj, gsd );
      ossimDpt halfGsd = gsd/2.0;
      
      bool gridAligned = alignToGrid();
      bool isGeographic = proj->isGeographic();
      if ( isGeographic )
      {
         ossimGpt tie(0.0, 0.0, 0.0);
         if ( gridAligned )
         {
            tie.lon = m_projectionBoundingRect.ul().x + halfGsd.x;
            tie.lat = m_projectionBoundingRect.ul().y - halfGsd.y;
         }
         else
         {
            tie.lon = m_sceneBoundingRect.ul().x + halfGsd.x;
            tie.lat = m_sceneBoundingRect.ul().y - halfGsd.y;
         }
         proj->setUlTiePoints(tie);
         
         if ( traceDebug() )
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimGpkgWriter::setProjectionTie DEBUG:\n"
               << "tie: " << tie << std::endl;
         }
      }
      else
      {
         ossimDpt tie( 0.0, 0.0 );
         if ( gridAligned )
         {
            tie.x = m_projectionBoundingRect.ul().x + halfGsd.x;
            tie.y = m_projectionBoundingRect.ul().y - halfGsd.y;
         }
         else
         {
            tie.x = m_sceneBoundingRect.ul().x + halfGsd.x;
            tie.y = m_sceneBoundingRect.ul().y - halfGsd.y;
         }
         
         proj->setUlTiePoints(tie);

         if ( traceDebug() )
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimGpkgWriter::setProjectionTie DEBUG:\n"
               << "tie: " << tie << std::endl;
         }
      }
   }
   
} // End: ossimGpkgWriter::setProjectionTie

ossimGpkgWriter::ossimGpkgWriterMode ossimGpkgWriter::getWriterMode() const
{
   // Default to mixed.
   ossimGpkgWriterMode mode = OSSIM_GPGK_WRITER_MODE_MIXED;
   
   std::string value = m_kwl->findKey( WRITER_MODE_KW );
   if ( value.size() )
   {
      ossimString os(value);
      os.downcase();
      
      if ( os == "jpeg" )
      {
         mode = OSSIM_GPGK_WRITER_MODE_JPEG;
      }
      else if ( os == "png" )
      {
         mode = OSSIM_GPGK_WRITER_MODE_PNG;
      }
      else if ( os == "pnga" )
      {
         mode = OSSIM_GPGK_WRITER_MODE_PNGA;
      }
   }
   return mode;
}

std::string ossimGpkgWriter::getWriterModeString( ossimGpkgWriterMode mode ) const
{
   std::string result;
   switch ( mode )
   {
      case OSSIM_GPGK_WRITER_MODE_JPEG:
      {
         result = "jpeg";
         break;
      }
      case OSSIM_GPGK_WRITER_MODE_MIXED:
      {
         result = "mixed";
         break;
      }
      case OSSIM_GPGK_WRITER_MODE_PNG:
      {
         result = "png";
         break;
      }
      case OSSIM_GPGK_WRITER_MODE_PNGA:
      {
         result = "pnga";
         break;
      }
      case OSSIM_GPGK_WRITER_MODE_UNKNOWN:
      default:
      {
         result = "unknown";
         break;
      }
   }
   return result;
}

bool ossimGpkgWriter::requiresEightBit() const
{
   bool result = false;
   ossimGpkgWriterMode mode = getWriterMode();
   if ( mode == OSSIM_GPGK_WRITER_MODE_JPEG )
   {
      result = true;
   }
   return result;
}

ossim_uint32 ossimGpkgWriter::getEpsgCode() const
{
   ossim_uint32 result = 0;
   std::string value = m_kwl->findKey( EPSG_KW );
   if ( value.size() )
   {
      result = ossimString(value).toUInt32();
   }
   return result;
}

void ossimGpkgWriter::getProjectionDimensionsInMeters(
   const ossimMapProjection* proj, ossimDpt& dims ) const
{
   if ( proj )
   {
      ossim_uint32 epsgCode = proj->getPcsCode();

      switch( epsgCode )
      {
         case 4326:
         {
            if ( proj->getOrigin().lat == 0.0 )
            { 
               // 20015110.0 * 2 = 40030220.0;
               dims.x = 40030220.0;

               // 10007555.0 * 2 = 20015110;
               dims.y = 20015110;  
            }
            else
            {
               std::ostringstream errMsg;
               errMsg << "ossimGpkgWriter::getProjectionDimensionsInMeters ERROR:\n"
                      << "EPSG 4326 Origin latitude is not at 0.\n";
               throw ossimException( errMsg.str() );
            }
            break;
         }
         case 3395:
         {
            //---
            // http://spatialreference.org/ref/epsg/3395/
            // WGS84 Bounds: -180.0000, -80.0000, 180.0000, 84.0000
            // Projected Bounds: -20037508.3428, -15496570.7397, 20037508.3428, 18764656.2314
            //---
            dims.x = 40075016.6856;
            dims.y = 34261226.9711;
            
            break;
         }
         case 3857:
         {
            // Bounds: 
            //  -20037508.342789244,-20037508.342789244,20037508.342789244,20037508.342789244
            //
            dims.x = 40075016.685578488;//40075016.6856;
            dims.y = 40075016.685578488;//40075016.6856;
            break;
         }
         default:
         {
            std::ostringstream errMsg;
            errMsg << "ossimGpkgWriter::getProjectionDimensionsInMeters ERROR:\n"
                   << "Unhandled espg code: " << epsgCode << "\n";
            throw ossimException( errMsg.str() );
         }
      }
   }
   
} // End: ossimGpkgWriter::getProjectionDimensionsInMeters

#if 0
void ossimGpkgWriter::clipToProjectionBounds(
   const ossimMapProjection* proj, const ossimGpt& inUlGpt, const ossimGpt& inLrGpt,
   ossimGpt& outUlGpt, ossimGpt& outLrGpt ) const
{
   if ( proj )
   {
      outUlGpt = inUlGpt;
      outLrGpt = inLrGpt;

      ossim_uint32 code = proj->getPcsCode();

      if ( code == 3395 )
      {
         //---
         // http://spatialreference.org/ref/epsg/3395/
         // WGS84 Bounds: -180.0000, -80.0000, 180.0000, 84.0000
         //---
         const ossim_float64 MAX_LAT = 84.0;
         const ossim_float64 MIN_LAT = -80.0;
         
         if ( outUlGpt.lat > MAX_LAT ) outUlGpt.lat = MAX_LAT;
         if ( outLrGpt.lat < MIN_LAT ) outLrGpt.lat = MIN_LAT;
      }
      else if ( code == 3857 )
      {
         //---
         // http://epsg.io/3857
         // WGS84 bounds: -180.0 -85.06, 180.0 85.06 
         //---
         ossim_float64 MAX_LAT = 85.05112878;
         ossim_float64 MIN_LAT = -85.05112878;
         
         if ( outUlGpt.lat > MAX_LAT ) outUlGpt.lat = MAX_LAT;
         if ( outLrGpt.lat < MIN_LAT ) outLrGpt.lat = MIN_LAT;
      }
   }
   
} // End: ossimGpkgWriter::clipToProjectionBounds
#endif

void ossimGpkgWriter::initializeProjectionRect( const ossimMapProjection* proj )
{
   if ( proj )
   {
      ossim_uint32 epsgCode = proj->getPcsCode();

      switch( epsgCode )
      {
         case 4326:
         {
            if ( proj->getOrigin().lat == 0.0 )
            {
               m_projectionBoundingRect =
                  ossimDrect( -180.0, 90.0, 180.0, -90.0, OSSIM_RIGHT_HANDED);
            }
            else
            {
               std::ostringstream errMsg;
               errMsg << "ossimGpkgWriter::initializeProjectionRect ERROR:\n"
                      << "EPSG 4326 Origin latitude is not at 0.\n";
               throw ossimException( errMsg.str() );
            }
            break;
         }
         case 3395:
         {
            //---
            // http://spatialreference.org/ref/epsg/3395/
            // WGS84 Bounds: -180.0000, -80.0000, 180.0000, 84.0000
            // Projected Bounds: -20037508.3428, -15496570.7397, 20037508.3428, 18764656.2314
            //---
             m_projectionBoundingRect =
                ossimDrect( -20037508.3428, 18764656.2314,
                            20037508.3428, -15496570.7397, OSSIM_RIGHT_HANDED );
             break;
         }
         case 3857:
         {
            m_projectionBoundingRect =
               ossimDrect( -20037508.342789244, 20037508.342789244,
                           20037508.342789244, -20037508.342789244, OSSIM_RIGHT_HANDED);
            break;       
         }
         default:
         {
            std::ostringstream errMsg;
            errMsg << "ossimGpkgWriter::initializeProjectionRect ERROR:\n"
                   << "Unhandled espg code: " << epsgCode << "\n";
            throw ossimException( errMsg.str() );
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgWriter::initializeProjectionRect:\n"
         << "projection bounding rect: " << m_projectionBoundingRect
         << std::endl;
   }
   
} // End: ossimGpkgWriter::initializeProjectionRect

void ossimGpkgWriter::initializeRect( const ossimMapProjection* proj,
                                      const ossimIrect& aoi,
                                      ossimDrect& rect )
{
   if ( proj )
   {
      ossimDpt gsd;
      getGsd( proj, gsd );
      ossimDpt halfGsd = gsd/2.0;
      
      ossimDpt ulLineSample = aoi.ul();
      ossimDpt lrLineSample = aoi.lr();

      if ( proj->isGeographic() )
      {
         // Convert line, sample to ground points:
         ossimGpt ulGpt;
         ossimGpt lrGpt;
         proj->lineSampleToWorld( ulLineSample, ulGpt );
         proj->lineSampleToWorld( lrLineSample, lrGpt );
         
         ossim_float64 ulx = ossim::max<ossim_float64>( ulGpt.lon - halfGsd.x, -180.0 );
         ossim_float64 uly = ossim::min<ossim_float64>( ulGpt.lat + halfGsd.y,  90.0 );
         ossim_float64 lrx = ossim::min<ossim_float64>( lrGpt.lon + halfGsd.x,  180.0 );
         ossim_float64 lry = ossim::max<ossim_float64>( lrGpt.lat - halfGsd.y, -90.0 );
         
         rect = ossimDrect( ulx, uly, lrx, lry, OSSIM_RIGHT_HANDED );
      }
      else
      {
         ossimDpt ulEastingNorthingPt;
         ossimDpt lrEastingNorthingPt;
         proj->lineSampleToEastingNorthing(
            aoi.ul(), ulEastingNorthingPt );
         proj->lineSampleToEastingNorthing(
            aoi.lr(), lrEastingNorthingPt );
         
         // Edge to edge scene bounding rect.
         rect = ossimDrect( ulEastingNorthingPt.x - halfGsd.x,
                            ulEastingNorthingPt.y + halfGsd.y,
                            lrEastingNorthingPt.x + halfGsd.x,
                            lrEastingNorthingPt.y - halfGsd.y,
                            OSSIM_RIGHT_HANDED);
      }

   } // Matches: if ( proj.valid() )

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGpkgWriter::initializeRect:"
         << "\naoi: " << aoi
         << "\nrect: " << rect
         << std::endl;
   }
   
} // End: ossimGpkgWriter::initializeRect

void ossimGpkgWriter::getTileTableName( std::string& tileTableName ) const
{
   std::string key = "tile_table_name";
   tileTableName = m_kwl->findKey( key );
   if ( tileTableName.empty() )
   {
      tileTableName = "tiles";
   }
}

bool ossimGpkgWriter::getFilename( ossimFilename& file ) const
{
   bool status = false;
   file.string() = m_kwl->findKey( std::string(ossimKeywordNames::FILENAME_KW) );
   if ( file.size() > 0 )
   {
      status = true;
   }
   return status;
}

void ossimGpkgWriter::initializeCodec()
{
   ossimGpkgWriterMode mode = getWriterMode();
   if ( mode == OSSIM_GPGK_WRITER_MODE_JPEG )
   {
      m_fullTileCodec = ossimCodecFactoryRegistry::instance()->createCodec(ossimString("jpeg"));
      m_partialTileCodec = m_fullTileCodec.get();
      m_fullTileCodecAlpha = false;
      m_partialTileCodecAlpha = false;
   }
   else if(mode == OSSIM_GPGK_WRITER_MODE_PNG)
   {
      m_fullTileCodec = ossimCodecFactoryRegistry::instance()->createCodec(ossimString("png"));
      m_partialTileCodec = m_fullTileCodec.get();
      m_fullTileCodecAlpha = false;
      m_partialTileCodecAlpha = false;
   }
   else if( mode == OSSIM_GPGK_WRITER_MODE_PNGA )
   {
      m_fullTileCodec = ossimCodecFactoryRegistry::instance()->createCodec(ossimString("pnga"));
      m_partialTileCodec = m_fullTileCodec.get();
      m_fullTileCodecAlpha = true;
      m_partialTileCodecAlpha = true;
   }
   else if( mode == OSSIM_GPGK_WRITER_MODE_MIXED )
   {
      m_fullTileCodec = ossimCodecFactoryRegistry::instance()->createCodec(ossimString("jpeg"));
      m_partialTileCodec = ossimCodecFactoryRegistry::instance()->createCodec(ossimString("pnga"));
      m_fullTileCodecAlpha = false;
      m_partialTileCodecAlpha = true;
   }
   else
   {
      m_fullTileCodec = 0;
      m_partialTileCodec = 0;
   }

   if ( m_fullTileCodec.valid() &&  m_partialTileCodec.valid() )
   {
      // Note: This will only take for jpeg.  Png uses compression_level and need to add.      
      ossim_uint32 quality = getCompressionQuality();
      if ( !quality )
      {
         quality = (ossim_uint32)ossimGpkgWriter::DEFAULT_JPEG_QUALITY;
      }
      m_fullTileCodec->setProperty("quality", ossimString::toString(quality));
      m_partialTileCodec->setProperty("quality", ossimString::toString(quality));
   }
   else
   {
      std::ostringstream errMsg;
      errMsg << "ossimGpkgWriter::initializeCodec ERROR:\n"
             << "Unsupported writer mode: " << getWriterModeString( mode )
             << "\nCheck for ossim png plugin..."
             << "\n";
      throw ossimException( errMsg.str() );
   }
}

bool ossimGpkgWriter::getWmsCutBox( ossimDrect& rect ) const
{
   const std::string KEY = "cut_wms_bbox";
   return getRect( KEY, rect );
}

bool ossimGpkgWriter::getClipExtents( ossimDrect& rect, bool& alignToGridFlag ) const
{
   bool result = getRect( CLIP_EXTENTS_KW, rect );
   alignToGridFlag = true;
   ossimString value = m_kwl->findKey( CLIP_EXTENTS_ALIGN_TO_GRID_KW );
   if(!value.empty()) alignToGridFlag = value.toBool();

   return result;
}

bool ossimGpkgWriter::getRect( const std::string& key, ossimDrect& rect ) const
{
   bool status = false;
   ossimString value;
   value.string() = m_kwl->findKey( key );
   if ( value.size() )
   {
      std::string replacementPattern = value.string() + std::string(":");
      ossimString bbox = value.downcase().replaceAllThatMatch(replacementPattern.c_str(),"");
      std::vector<ossimString> cutBox;
      bbox.split( cutBox, "," );
      if( cutBox.size() == 4 )
      {
         ossim_float64 minx = cutBox[0].toFloat64();
         ossim_float64 miny = cutBox[1].toFloat64();
         ossim_float64 maxx = cutBox[2].toFloat64();
         ossim_float64 maxy = cutBox[3].toFloat64();
         rect = ossimDrect( minx, maxy, maxx, miny );
         status = true;
      }
   }
   return status;
}

void ossimGpkgWriter::checkLevels(
   const std::vector<ossim_int32>& currentZoomLevels,
   const std::vector<ossim_int32>& newZoomLevels ) const
{
   static const char MODULE[] = "ossimGpkgWriter::checkLevels";

   // Assuming sorted, low to high arrays.
   if ( currentZoomLevels.size() )
   {
      if ( newZoomLevels.size() )
      {
         // Check for new level lower then existing.
         if ( newZoomLevels[0] <  currentZoomLevels[0] )
         {
            std::ostringstream errMsg;
            errMsg << MODULE
                   << " ERROR:\n"
                   << "New level[" << newZoomLevels[0]
                   << "] will not fit in existing extents of level["
                   << currentZoomLevels[0] << "].\n";
            throw ossimException( errMsg.str() );
         }   

         // 2) level already present
         std::vector<ossim_int32>::const_iterator newIdx = newZoomLevels.begin();
         while ( newIdx < newZoomLevels.end() )
         {
            std::vector<ossim_int32>::const_iterator currentIdx = currentZoomLevels.begin();
            while ( currentIdx != currentZoomLevels.end() )
            {
               if ( (*newIdx) == (*currentIdx) )
               {
                  std::ostringstream errMsg;
                  errMsg << MODULE
                         << " ERROR:\n"
                         << "New level[" << (*newIdx)
                         << "] already exists in current matrix set.\n";
                  throw ossimException( errMsg.str() );
               }
               ++currentIdx;
            }
            ++newIdx;
         }
      }
   }
   
} // End: ossimGpkgWriter::checkLevels

bool ossimGpkgWriter::isValidZoomLevelRowCol(
   ossim_int32 level, ossim_int32 row, ossim_int32 col  ) const
{
   bool status = false;

   if ( m_zoomLevels.size() && (m_zoomLevels.size() == m_zoomLevelMatrixSizes.size()) )
   {
      std::vector<ossim_int32>::const_iterator zIdx = m_zoomLevels.begin();
      std::vector<ossimIpt>::const_iterator sIdx = m_zoomLevelMatrixSizes.begin();
      while ( zIdx != m_zoomLevels.end() )
      {
         if ( (*zIdx) == level )
         {
            if ( ( row < (*sIdx).y ) && ( col < (*sIdx).x ) &&
                 ( row > -1 ) && ( col > -1 ) )
            {
               status = true;
               break;
            }
         }
         ++zIdx;
         ++sIdx;
      }
   }
   
   return status;
   
} // End: ossimGpkgWriter::isValidZoomLevel( ossim_int32 level ) const



void ossimGpkgWriter::applyScaleToProjection( ossimMapProjection* proj,
                                              const ossimDpt& desiredGsd ) const
{
   if ( proj )
   {
      if ( desiredGsd.hasNans() == false )
      {
         // Current projection gsd:
         ossimDpt currentGsd;
         getGsd( proj, currentGsd );
         
         //---
         // Set the scale of projection to the stop gsd:
         // True on applyScale is to recenter tie.
         //---
         ossimDpt scale;
         scale.x = desiredGsd.x / currentGsd.x;
         scale.y = desiredGsd.y / currentGsd.y;
         proj->applyScale( scale, true );

#if 0 /** Please leave for debug. (drb) */
         cout << "ossimGpkgWriter::applyScaleToProjection DEBUG:"
              << "\nproj gsd:    " << currentGsd
              << "\ndesired gsd: " << desiredGsd
              << "\nscale:       " << scale
              << "\n";
#endif
      }
   }
}


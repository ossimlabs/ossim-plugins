//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Open JPEG JP2 reader (tile source).
//
//----------------------------------------------------------------------------
// $Id$

#include <ossimOpjJp2Reader.h>
#include <ossimOpjCommon.h>

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotifyContext.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimUnitConversionTool.h>

#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimImageGeometryRegistry.h>
#include <ossim/imaging/ossimTiffTileSource.h>
#include <ossim/imaging/ossimU8ImageData.h>

#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>

#include <ossim/support_data/ossimFgdcXmlDoc.h>
#include <ossim/support_data/ossimGmlSupportData.h>
#include <ossim/support_data/ossimJ2kCodRecord.h>
#include <ossim/support_data/ossimJp2Info.h>
#include <ossim/support_data/ossimTiffInfo.h>
#include <ossim/support_data/ossimTiffWorld.h>

#include <openjpeg.h>
#include <fstream>

RTTI_DEF1(ossimOpjJp2Reader, "ossimOpjJp2Reader", ossimImageHandler)

#ifdef OSSIM_ID_ENABLED
   static const char OSSIM_ID[] = "$Id: ossimOpjJp2Reader.cpp 11439 2007-07-29 17:43:24Z dburken $";
#endif
   
static ossimTrace traceDebug(ossimString("ossimOpjJp2Reader:degug"));

static const ossim_uint16 SOC_MARKER = 0xff4f; // start of codestream marker
static const ossim_uint16 SIZ_MARKER = 0xff51; // size maker


ossimOpjJp2Reader::ossimOpjJp2Reader()
   :
   ossimImageHandler(),
   m_sizRecord(),
   m_tile(0),
   m_str(0),
   m_minDwtLevels(0)
{
   // Uncomment to enable trace for debug:
   // traceDebug.setTraceFlag(true); 
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimOpjJp2Reader::ossimOpjJp2Reader entered..." << std::endl;
#ifdef OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  " << OSSIM_ID << endl;
#endif
   }
}

ossimOpjJp2Reader::~ossimOpjJp2Reader()
{
   destroy();
}

void ossimOpjJp2Reader::destroy()
{
   m_tile      = 0;  // ossimRefPtr
   m_cacheTile = 0;  // ossimRefPtr   
   
   if ( m_str )
   {
      m_str->close();
      delete m_str;
      m_str = 0;
   }
}

bool ossimOpjJp2Reader::open()
{
   static const char MODULE[] = "ossimOpjJp2Reader::open";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..."
         << "\nFile:  " << theImageFile.c_str()
         << std::endl;
   }

   bool status = false;

   close();

   if ( theImageFile.size() )
   {
      m_str = new std::ifstream();
      m_str->open( theImageFile.string().c_str(), std::ios_base::in | std::ios_base::binary);
      
      if ( m_str->good() )
      {
         m_format = ossim::getCodecFormat( m_str );

         if ( m_format != OPJ_CODEC_UNKNOWN )
         {
            m_str->seekg(0, ios_base::beg);
            if ( initSizRecord( m_str, m_sizRecord ) )
            {
               ossimJ2kCodRecord codRecord;
               status = initCodRecord( m_str, codRecord );

               if ( status )
               {
                  // Number of built in reduced res sets.
                  m_minDwtLevels = codRecord.m_numberOfDecompositionLevels;

                  // Put the stream back:
                  m_str->seekg(0, ios_base::beg);

                  if ( traceDebug() )
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << " DEBUG: J2K COD RECORD:\n"
                        << codRecord
                        << "\n";
                  }
               }
            }
         }
         
         if ( status && traceDebug() )
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << " DEBUG: J2K SIZ RECORD:\n"
               << m_sizRecord
               << "\n";
         }
      }
      else if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "Cannot open:  " << theImageFile.c_str() << "\n";
      }
      
   }

   if ( !status )
   {
      m_str->close();
      delete m_str;
      m_str = 0;
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status " << (status?"true\n":"false\n");
   }
   return status;
}

void ossimOpjJp2Reader::close()
{
   destroy();
   ossimImageHandler::close();   
}

ossimRefPtr<ossimImageData> ossimOpjJp2Reader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   ossimRefPtr<ossimImageData> result = 0;
   
   if( isSourceEnabled() && isOpen() && isValidRLevel(resLevel) )
   {
      if ( m_tile.valid() == false )
      {
         allocate(); // First time though.
      }

      // Rectangle must be set prior to getOverviewTile call.
      m_tile->setImageRectangle(rect);

      // tmp drb...
      m_tile->makeBlank();
      
      if ( getOverviewTile( resLevel, m_tile.get() ) )
      {
         result = m_tile.get();
      }
   }

   return result;
}

bool ossimOpjJp2Reader::getOverviewTile(ossim_uint32 resLevel,
                                        ossimImageData* result)
{
   bool status = false;

   if ( result )
   {
      if (resLevel <= m_minDwtLevels)
      {
         // Using internal overviews.
         
         //---
         // NOTE:
         //
         // The geojp2 doqq's that I have all have an offset in them.  In
         // other words the "pos" from "get_dims" is not always 0,0.  I
         // think this was intended for mosaicing without any projection. I
         // do NOT think it was intended to be used as a sub image offset
         // into the projection picked up by the geotiff_box. If this were
         // so the current code here would not mosaic correctly.
         //
         // This may not be the case though with all data sets...  In which
         // case some kind of logic would have to be added to this code.
         
         ossimIrect tileRect  = result->getImageRectangle();
         ossimIrect imageRect = getImageRectangle( resLevel );
         ossimIrect clipRect  = tileRect.clipToRect(imageRect);

         ossimIpt offset((ossim_int32)m_sizRecord.m_XOsiz,
                         (ossim_int32)m_sizRecord.m_YOsiz);

         // tmp drb...
         // ossimIrect rect(0,0,255,255);

         ossimIrect shiftedRect = clipRect + offset;
         if ( traceDebug() )
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimOpjJp2Reader::getOverviewTile DEBUG:"
               << "\ntile rect:   " << tileRect
               << "\nimageRect:   " << imageRect
               << "\nclipRect:    " << clipRect
               << "\nshiftedRect: " << shiftedRect
               << "\noffset:      " << offset
               << "\nresLevel:      " << resLevel
               << std::endl;
         }

         m_cacheTile->setImageRectangle( clipRect );
         
         try
         {
            status = ossim::opj_decode( m_str,
                                        shiftedRect,
                                        resLevel,
                                        m_format,
                                        0,
                                        m_cacheTile.get() );

            if ( status )
            {
               
               result->loadTile(m_cacheTile->getBuf(), clipRect,  OSSIM_BSQ);
               result->validate();
            }
         }
         catch( const ossimException& e )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << __FILE__ << " " << __LINE__ << " caught exception\n"
               << e.what() << "\n File:" << this->theImageFile << "\n";
            status = false;
         }
         catch( ... )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << __FILE__ << " " << __LINE__ << " caught unknown exception\n";
         }
            
         // result->setImageRectangle(originalTileRect);
         
         try
         {
#if 0
            if ( theChannels )
            {
               status = ossim::copyRegionToTile(theChannels,
                                                theCodestream,
                                                static_cast<int>(resLevel),
                                                theThreadEnv,
                                                theOpenTileThreadQueue,
                                                result);
            }
            else
            {
               status = ossim::copyRegionToTile(theCodestream,
                                                static_cast<int>(resLevel),
                                                theThreadEnv,
                                                theOpenTileThreadQueue,
                                                result);
            }
#endif
         }
         catch(const ossimException& e)
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << __FILE__ << " " << __LINE__ << " caught exception\n"
               << e.what();
            status = false;
         }
         
         // Set the rect back.
         // result->setImageRectangle(originalTileRect);
         
      }  // matches:  if (resLevel <= theMinDwtLevels)
      else
      {
         // Using external overview.
         status = theOverview->getTile(result, resLevel);
      }
   }

   return status;
}

ossimIrect
ossimOpjJp2Reader::getImageRectangle(ossim_uint32 reduced_res_level) const
{
   return ossimIrect(0,
                     0,
                     getNumberOfSamples(reduced_res_level) - 1,
                     getNumberOfLines(reduced_res_level)   - 1);
}

bool ossimOpjJp2Reader::saveState(ossimKeywordlist& kwl,
                                  const char* prefix) const
{
   return ossimImageHandler::saveState(kwl, prefix);
}

bool ossimOpjJp2Reader::loadState(const ossimKeywordlist& kwl,
                                  const char* prefix)
{
   if (ossimImageHandler::loadState(kwl, prefix))
   {
      return open();
   }

   return false;
}

ossim_uint32 ossimOpjJp2Reader::getNumberOfDecimationLevels()const
{
   ossim_uint32 result = 1; // Add r0

   if (m_minDwtLevels)
   {
      //---
      // Add internal overviews.
      //---
      result += m_minDwtLevels;
   }

   if (theOverview.valid())
   {
      //---
      // Add external overviews.
      //---
      result += theOverview->getNumberOfDecimationLevels();
   }

   return result;
}

ossim_uint32 ossimOpjJp2Reader::getNumberOfLines( ossim_uint32 resLevel ) const
{
   ossim_uint32 result = 0;
   if ( isValidRLevel(resLevel) )
   {
      if (resLevel <= m_minDwtLevels)
      {
         result = m_sizRecord.m_Ysiz - m_sizRecord.m_YOsiz;
         if ( resLevel )
         {
            ossim_float32 x = 2.0;
            ossim_float32 y = resLevel;
            result = (ossim_uint32) result / std::pow(x, y);
         }  
      }
      else if (theOverview.valid())
      {
         result = theOverview->getNumberOfLines(resLevel);
      }
   }
   return result;
}

ossim_uint32 ossimOpjJp2Reader::getNumberOfSamples( ossim_uint32 resLevel ) const
{
   ossim_uint32 result = 0;
   if ( isValidRLevel(resLevel) )
   {
      if (resLevel <= m_minDwtLevels)
      {
         result = m_sizRecord.m_Xsiz - m_sizRecord.m_XOsiz;
         if ( resLevel )
         {
            ossim_float32 x = 2.0;
            ossim_float32 y = resLevel;
            result = (ossim_uint32) result / std::pow(x, y);
         }  
      }
      else if (theOverview.valid())
      {
         result = theOverview->getNumberOfLines(resLevel);
      }
   }
   return result;
}

//---
// Use the internal j2k tile size if it's not the same as the image(one BIG tile).
//---
ossim_uint32 ossimOpjJp2Reader::getImageTileWidth() const
{
   ossim_uint32 result = 0;
   if ( (m_sizRecord.m_XTsiz - m_sizRecord.m_XTOsiz) < getNumberOfSamples(0) )
   {
      result = m_sizRecord.m_XTsiz - m_sizRecord.m_XTOsiz;
   }
   return result;
}

ossim_uint32 ossimOpjJp2Reader::getImageTileHeight() const
{
   ossim_uint32 result = 0;
   if ( (m_sizRecord.m_YTsiz - m_sizRecord.m_YTOsiz) < getNumberOfLines(0) )
   {
      result = m_sizRecord.m_YTsiz - m_sizRecord.m_YTOsiz;
   }
   return result; 
}

ossimString ossimOpjJp2Reader::getShortName()const
{
   return ossimString("ossim_openjpeg_reader");
}
   
ossimString ossimOpjJp2Reader::getLongName()const
{
   return ossimString("ossim open jpeg reader");
}

ossimString  ossimOpjJp2Reader::getClassName()const
{
   return ossimString("ossimOpjJp2Reader");
}

ossim_uint32 ossimOpjJp2Reader::getNumberOfInputBands() const
{
   return m_sizRecord.m_Csiz;
}

ossim_uint32 ossimOpjJp2Reader::getNumberOfOutputBands()const
{
   return m_sizRecord.m_Csiz;
}

ossimScalarType ossimOpjJp2Reader::getOutputScalarType() const
{
   return m_sizRecord.getScalarType();
}

bool ossimOpjJp2Reader::isOpen()const
{
   return m_str ? m_str->is_open() : false;
}

double ossimOpjJp2Reader::getMaxPixelValue(ossim_uint32 band)const
{
   
   if (m_tile.valid())
   {
      return m_tile->getMaxPix(band);
   }
   return 255.0;
}

bool ossimOpjJp2Reader::initSizRecord( std::istream* str,
                                       ossimJ2kSizRecord& sizRecord ) const
{
   bool result = false;
   
   if ( str )
   {
      if ( str->good() )
      {
         // Looking for SOC, SIZ markers: 0xff, 0x4f, 0xff, 0x51
         const ossim_uint8 AFF = 0xff;
         const ossim_uint8 A4F = 0x4f;
         const ossim_uint8 A51 = 0x51;

         union
         {
            char c;
            ossim_uint8 uc;
         } ct;
      
         // Read in the box.
         while ( str->get( ct.c ) )
         {
            if ( ct.uc == AFF ) // Found FF byte.
            { 
               if ( str->get( ct.c ) )
               {
                  if ( ct.uc ==  A4F ) // Found 4F byte.
                  {
                     if ( str->get( ct.c ) )
                     {
                        if ( ct.uc ==  AFF ) // Found FF byte.
                        {
                           if ( str->get( ct.c ) )
                           {
                              if ( ct.uc ==  A51 ) // Found 51 byte.
                              {
                                 sizRecord.parseStream( *str );
                                 result = true;
                                 break;
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   
   return result;
}

bool ossimOpjJp2Reader::initCodRecord( std::istream* str,
                                       ossimJ2kCodRecord& codRecord ) const
{
   bool result = false;

   if ( str )
   {
      if ( str->good() )
      {
         // Looking for COD marker: 0xff, 0x51
         const ossim_uint8 AFF = 0xff;
         const ossim_uint8 A52 = 0x52;

         union
         {
            char c;
            ossim_uint8 uc;
         } ct;
      
         // Read in the box.
         while ( str->get( ct.c ) )
         {
            if ( ct.uc == AFF ) // Found FF byte.
            { 
               if ( str->get( ct.c ) )
               {
                  if ( ct.uc ==  A52 ) // Found 52 byte.
                  {
                     codRecord.parseStream( *str );
                     result = true;
                     break;
                  }
               }
            }
         }
      }
   }
   
   return result;
}

ossimRefPtr<ossimImageGeometry> ossimOpjJp2Reader::getImageGeometry()
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
         //---
         // Check for external files other than .geom, i.e. file.xml & j2w:
         //---
         theGeometry = getMetadataImageGeometry();

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
      }

      // Set image things the geometry object should know about.
      initImageParameters( theGeometry.get() );
   }
   return theGeometry;
}

ossimRefPtr<ossimImageGeometry> ossimOpjJp2Reader::getInternalImageGeometry()
{
   static const char MODULE[] = "ossimOpjJp2Reader::getInternalImageGeometry";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageGeometry> geom = 0;

   if ( isOpen() )
   {
      std::streamoff pos = m_str->tellg();

      m_str->seekg(0, std::ios_base::beg );

      // Straight up J2k has no boxes.
      if ( ossim::getCodecFormat( m_str ) == OPJ_CODEC_JP2 )
      {
         m_str->seekg(4, std::ios_base::beg );

         // Try to get geom from GML box:
         geom = getImageGeometryFromGmlBox();

         if ( geom.valid() == false )
         {
            // Try to get geom from geotiff box:
            geom = getImageGeometryFromGeotiffBox();
         }
      }

      // Seek back to original position.
      m_str->seekg(pos, std::ios_base::beg );
   }

   return geom;

} // End: ossimOpjJp2Reader::getInternalImageGeometry()
 
ossimRefPtr<ossimImageGeometry> ossimOpjJp2Reader::getImageGeometryFromGeotiffBox()
{
   static const char MODULE[] = "ossimOpjJp2Reader::getImageGeometryFromGeotiffBox";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   ossimRefPtr<ossimImageGeometry> geom = 0;
   
   if ( isOpen() )
   {
      std::streamoff pos = m_str->tellg();
      
      m_str->seekg(0, std::ios_base::beg );

      std::vector<ossim_uint8> box;
      ossimJp2Info jp2Info;
      
      std::streamoff boxPos = jp2Info.getGeotiffBox( *m_str, box );

      // Seek back to original position.
      m_str->seekg(pos, std::ios_base::beg );
      
      if ( boxPos && box.size() )
      {
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "Found geotiff uuid at: " << boxPos+8 << "\n";
         }

         //---
         // Create a string stream and set the vector buffer as its source.
         // Note: The box has the 16 GEOTIFF_UUID bytes in there so offset
         // address and size.
         //---
#if 0
         // This doesn't work with VS2010...
         // Create a string stream and set the vector buffer as its source.
         std::istringstream boxStream;
         boxStream.rdbuf()->pubsetbuf( (char*)&box.front()+GEOTIFF_UUID_SIZE,
                                 box.size()-GEOTIFF_UUID_SIZE );
#else
         // convert the vector into a string
         std::string boxString( box.begin()+GEOTIFF_UUID_SIZE, box.end() );
         std::istringstream boxStream;
         boxStream.str( boxString );
#endif


         // Give the stream to tiff info to create a geometry.
         ossimTiffInfo info;
         ossim_uint32 entry = 0;
         ossimKeywordlist kwl; // Used to capture geometry data. 
         
         if ( info.getImageGeometry(boxStream, kwl, entry) )
         {
            //---
            // The tiff embedded in the geojp2 only has one line
            // and one sample by design so overwrite the lines and
            // samples with the real value.
            //---
            ossimString pfx = "image";
            pfx += ossimString::toString(entry);
            pfx += ".";
            
            // Add the lines.
            kwl.add(pfx.chars(), ossimKeywordNames::NUMBER_LINES_KW,
                    getNumberOfLines(0), true);
            
            // Add the samples.
            kwl.add(pfx.chars(), ossimKeywordNames::NUMBER_SAMPLES_KW,
                    getNumberOfSamples(0), true);
            
            // Create the projection.
            ossimRefPtr<ossimProjection> proj =
               ossimProjectionFactoryRegistry::instance()->createProjection(kwl, pfx);
            if ( proj.valid() )
            {
               // Create and assign projection to our ossimImageGeometry object.
               geom = new ossimImageGeometry();
               geom->setProjection( proj.get() );
               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG) << "Found GeoTIFF box." << std::endl;
               }
               
               // Get the internal raster pixel alignment type and set the base class.
               const char* lookup = kwl.find(pfx.chars(), ossimKeywordNames::PIXEL_TYPE_KW);
               if ( lookup )
               {
                  ossimString type = lookup;
                  type.downcase();
                  if ( type == "pixel_is_area" )
                  {
                     thePixelType = OSSIM_PIXEL_IS_AREA;
                  }
                  else if ( type == "pixel_is_point" )
                  {
                     thePixelType = OSSIM_PIXEL_IS_POINT;
                  }
               }
            }
         }
      }
      else // Did not find box in file.
      {
         m_str->clear();
      }
   }
   return geom;
   
} // End: ossimOpjJp2Reader::getImageGeometryFromGeotiffBox


ossimRefPtr<ossimImageGeometry> ossimOpjJp2Reader::getImageGeometryFromGmlBox()
{
   static const char MODULE[] = "ossimOpjJp2Reader::getImageGeometryFromGmlBox";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   ossimRefPtr<ossimImageGeometry> geom = 0;
   
   if ( isOpen() )
   {
      std::streamoff pos = m_str->tellg();
      
      m_str->seekg(0, std::ios_base::beg );

      std::vector<ossim_uint8> box;
      ossimJp2Info jp2Info;
      
      std::streamoff boxPos = jp2Info.getGmlBox( *m_str, box );

      // Seek back to original position.
      m_str->seekg(pos, std::ios_base::beg );
      
      if ( boxPos && box.size() )
      {
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "Found gml box at: " << boxPos+8
               << "\nbox size: " << box.size() << "\n";
         }

#if 0
         // This doesn't work with VS2010...
         // Create a string stream and set the vector buffer as its source.
         std::istringstream boxStream;
         boxStream.rdbuf()->pubsetbuf( (char*)&box.front(), box.size() );
#else
         // convert the vector into a string
         std::string boxString( box.begin(), box.end() );
         std::istringstream boxStream;
         boxStream.str( boxString );
#endif

         ossimGmlSupportData* gml = new ossimGmlSupportData();

         if ( gml->initialize( boxStream ) )
         {
            // Tmp drb
            //cout << *(gml->getXmlDoc().get()) << endl;

            ossimKeywordlist geomKwl;
            if ( gml->getImageGeometry( geomKwl ) )
            {
               // Make projection:
               // Create the projection.
               ossimRefPtr<ossimProjection> proj =
                  ossimProjectionFactoryRegistry::instance()->createProjection( geomKwl );
               if ( proj.valid() )
               {
                  // Create and assign projection to our ossimImageGeometry object.
                  geom = new ossimImageGeometry();
                  geom->setProjection( proj.get() );
                  if (traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG) << "Found GMLJP2 box." << std::endl;
                  }
               }
            }
         }

         // Cleanup:
         delete gml;
         gml = 0;
      }
      else // Did not find box in file.
      {
         m_str->clear();
      }
   }
   return geom;
   
} // End: ossimOpjJp2Reader::getImageGeometryFromGmlBox

ossimRefPtr<ossimImageGeometry> ossimOpjJp2Reader::getMetadataImageGeometry() const
{
   static const char M[] = "ossimOpjJp2Reader::getMetadataImageGeometry";
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " entered...\n";
   }
   
   ossimRefPtr<ossimImageGeometry> geom = 0;
   ossimRefPtr<ossimProjection>    proj = 0;

   // See if we can pick up the projection from the FGDC file:
   ossimFilename fdgcFile = theImageFile;
   
   fdgcFile += ".xml"; // file.jp2.xml
   if ( fdgcFile.exists() == false ) 
   {
      fdgcFile = theImageFile;
      fdgcFile.setExtension(ossimString("xml")); // file.xml
   }
   
   if ( fdgcFile.exists() )
   {
      ossimFgdcXmlDoc fgdcDoc;
      if ( fgdcDoc.open(fdgcFile) )
      {
         try
         {
            proj = fgdcDoc.getGridCoordSysProjection();
         }
         catch (const ossimException& e)
         {
            ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
         }

         if ( proj.valid() )
         {
            geom = new ossimImageGeometry();

            ossimMapProjection* mapProj = dynamic_cast<ossimMapProjection*>(proj.get());
            if ( mapProj )
            {
               // See if we have a world file.  Seems they have a more accurate tie point.
               ossimFilename worldFile = theImageFile;
               worldFile.setExtension(ossimString("j2w")); // file.j2w
               if ( worldFile.exists() )
               {
                  //---
                  // Note need a way to determine pixel type from fgdc doc.
                  // This can result in a half pixel shift.
                  //---
                  ossimPixelType pixelType = OSSIM_PIXEL_IS_POINT;
                  ossimUnitType  unitType  = fgdcDoc.getUnitType();
                  
                  ossimTiffWorld tfw;
                  if ( tfw.open(worldFile, pixelType, unitType) )
                  {
                     ossimDpt gsd = tfw.getScale();
                     gsd.y = std::fabs(gsd.y); // y positive up so negate.
                     ossimDpt tie = tfw.getTranslation();
                     
                     if ( unitType != OSSIM_METERS )
                     {
                        ossimUnitConversionTool uct;
                        
                        // GSD (scale):
                        uct.setValue(gsd.x, unitType);
                        gsd.x = uct.getValue(OSSIM_METERS);
                        uct.setValue(gsd.y, unitType);
                        gsd.y = uct.getValue(OSSIM_METERS);
                        
                        // Tie point:
                        uct.setValue(tie.x, unitType);
                        tie.x = uct.getValue(OSSIM_METERS);
                        uct.setValue(tie.y, unitType);
                        tie.y = uct.getValue(OSSIM_METERS);
                     }
                     
                     mapProj->setMetersPerPixel(gsd);
                     mapProj->setUlTiePoints(tie);
                  }
                  
                  if ( tfw.getRotation() != 0.0 )
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << M << " Unhandled rotation in tfw file." << std::endl;
                  }
               }
               
            } // if ( worldFile.exists() )
               
            geom->setProjection( proj.get() );
            
         }  // if ( proj.valid() )
         
      } // if ( fgdcDoc.open(fdgcFile) )
      
   } // if ( fdgcFile.exists() )

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " exit status = " << (geom.valid()?"true":"false\n")
         << std::endl;
   }   
   
   return geom;
}

void ossimOpjJp2Reader::allocate()
{
   m_tile = ossimImageDataFactory::instance()->create(this, this);
   m_tile->initialize();
   m_cacheTile = ossimImageDataFactory::instance()->create(this, this);
   m_cacheTile->initialize();

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimOpjJp2Reader::allocate DEBUG:"
         << "\nm_tile:\n"      << *(m_tile.get())
         << endl;
   }
}

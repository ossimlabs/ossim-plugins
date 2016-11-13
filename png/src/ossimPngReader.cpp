//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: OSSIM Portable Network Graphics (PNG) reader (tile source).
//
//----------------------------------------------------------------------------
// $Id: ossimPngReader.cpp 23355 2015-06-01 23:55:15Z dburken $


#include "ossimPngReader.h"
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimIoStream.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotifyContext.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageDataFactory.h>

#include <zlib.h>

#include <cstddef> /* for NULL */
#include <cmath>   /* for pow */
#include <fstream>

// If true alpha channel is passed as a layer.
static const std::string USE_ALPHA_KW = "use_alpha"; // boolean

RTTI_DEF1(ossimPngReader, "ossimPngReader", ossimImageHandler)

#ifdef OSSIM_ID_ENABLED
   static const char OSSIM_ID[] = "$Id: ossimPngReader.cpp 23355 2015-06-01 23:55:15Z dburken $";
#endif
   
static ossimTrace traceDebug("ossimPngReader:degug");  

ossimPngReader::ossimPngReader()
   :
   ossimImageHandler(),
   m_tile(0),
   m_cacheTile(0),
   m_lineBuffer(0),
   m_lineBufferSizeInBytes(0),
   m_str(0),
   m_bufferRect(0, 0, 0, 0),
   m_imageRect(0, 0, 0, 0),
   m_numberOfInputBands(0),
   m_numberOfOutputBands(0),
   m_bytePerPixelPerBand(1),
   m_cacheSize(0),
   m_cacheId(-1),
   m_pngReadPtr(0),
   m_pngReadInfoPtr(0),
   m_pngColorType(PNG_COLOR_TYPE_GRAY),
   m_currentRow(0),
   m_outputScalarType(OSSIM_UINT8),
   m_interlacePasses(1),
   m_bitDepth(8),
   m_readMode(ossimPngReadUnknown),
   m_maxPixelValue(),
   m_swapFlag(false),
   m_useAlphaChannelFlag(false)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReader::ossimPngReader entered..." << std::endl;
      readPngVersionInfo();
#ifdef OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  " << OSSIM_ID << endl;
#endif
   }
}

ossimPngReader::~ossimPngReader()
{
   if (isOpen())
   {
      close();
   }
}

void ossimPngReader::close()
{
   destroy();
   ossimImageHandler::close();
}

void ossimPngReader::destroy()
{
   ossimAppFixedTileCache::instance()->deleteCache(m_cacheId);

   // ossimRefPtrs so assign to 0(unreferencing) will handle memory.
   m_tile      = 0;
   m_cacheTile = 0;

   if (m_lineBuffer)
   {
      delete [] m_lineBuffer;
      m_lineBuffer = 0;
   }

   if (m_pngReadPtr)
   {
      png_destroy_read_struct(&m_pngReadPtr, &m_pngReadInfoPtr, NULL);
      m_pngReadPtr = 0;
      m_pngReadInfoPtr = 0;
   }

   // Stream data member m_str is share pointer now:
   m_str = 0;
}

void ossimPngReader::allocate()
{
   // Make the cache tile the height of one tile by the image width.
   ossim::defaultTileSize(m_cacheSize);
   m_cacheSize.x = m_imageRect.width();
   
   ossimAppFixedTileCache::instance()->deleteCache(m_cacheId);
   m_cacheId = ossimAppFixedTileCache::instance()->
      newTileCache(m_imageRect, m_cacheSize);

   m_tile = ossimImageDataFactory::instance()->create(this, this);
   m_cacheTile = (ossimImageData*)m_tile->dup();
   m_tile->initialize();
   
   ossimIrect cache_rect(m_imageRect.ul().x,
                         m_imageRect.ul().y,
                         m_imageRect.ul().x + (m_cacheSize.x-1),
                         m_imageRect.ul().y + (m_cacheSize.y-1));
   
   m_cacheTile->setImageRectangle(cache_rect);
   m_cacheTile->initialize();

   if(m_lineBuffer)
   {
      delete [] m_lineBuffer;
   }
   m_lineBuffer = new ossim_uint8[m_lineBufferSizeInBytes];

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReader::allocate DEBUG:"
         << "\nm_cacheTile:\n" << *(m_cacheTile.get())
         << "\nm_tile:\n"      << *(m_tile.get())
         << "\ncache tile size:           " << m_cacheSize
         << "\nimage width:               " << m_imageRect.width()
         << "\nimage height:              " << m_imageRect.height()
         << "\nnumber of bands:           " << m_numberOfOutputBands
         << "\nline buffer size:          " << m_lineBufferSizeInBytes
         << endl;
   }
}

ossimRefPtr<ossimImageData> ossimPngReader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   if ( !m_tile )
   {
      // First time through. Allocate memory...
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

bool ossimPngReader::getTile(ossimImageData* result,
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
      
      //---
      // Check for overview tile.  Some overviews can contain r0 so always
      // call even if resLevel is 0.  Method returns true on success, false
      // on error.
      //---
      status = getOverviewTile(resLevel, result);

      if (status)
      {
         if(m_outputScalarType == OSSIM_UINT16)
         {
            //---
            // Temp fix:
            // The overview handler could return a tile of OSSIM_USHORT11 if
            // the max sample value was not set to 2047.
            //
            // To prevent a scalar mismatch set 
            //---
            result->setScalarType(m_outputScalarType);
         }
      }
      
      if (!status) // Did not get an overview tile.
      {
         status = true;
         
         ossimIrect tile_rect = result->getImageRectangle();

         if ( ! tile_rect.completely_within(getImageRectangle(0)) )
         {
            // We won't fill totally so make blank first.
            m_tile->makeBlank();
         }
         
         if (getImageRectangle(0).intersects(tile_rect))
         {
            // Make a clip rect.
            ossimIrect clip_rect = tile_rect.clipToRect(getImageRectangle(0));
            
            // This will validate the tile at the end.
            fillTile(clip_rect, result);
         }
      }

      result->unref();  // Decrement ref count.
   }

   return status;
}

void ossimPngReader::fillTile(const ossimIrect& clip_rect,
                              ossimImageData* tile)
{
   if (!tile || !m_str) return;

   ossimIrect buffer_rect = clip_rect;
   buffer_rect.stretchToTileBoundary(m_cacheSize);
   buffer_rect.set_ulx(0);
   buffer_rect.set_lrx(getImageRectangle(0).lr().x);

   ossim_int32 number_of_cache_tiles = buffer_rect.height()/m_cacheSize.y;

#if 0
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "tiles high:  " << number_of_cache_tiles
         << endl;
   }
#endif

   ossimIpt origin = buffer_rect.ul();
   
   for (int tileIdx = 0; tileIdx < number_of_cache_tiles; ++tileIdx)
   {
      // See if it's in the cache already.
      ossimRefPtr<ossimImageData> tempTile;
      tempTile = ossimAppFixedTileCache::instance()->
         getTile(m_cacheId, origin);
      if (tempTile.valid())
      {
         tile->loadTile(tempTile.get());
      }
      else
      {
         // Have to read from the png file.
         ossim_uint32 startLine = static_cast<ossim_uint32>(origin.y);
         ossim_uint32 stopLine  = 
            static_cast<ossim_uint32>( min(origin.y+m_cacheSize.y-1,
                                           getImageRectangle().lr().y) );
         ossimIrect cache_rect(origin.x,
                               origin.y,
                               origin.x+m_cacheSize.x-1,
                               origin.y+m_cacheSize.y-1);
         
         m_cacheTile->setImageRectangle(cache_rect);

         if ( !m_cacheTile->getImageRectangle().
              completely_within(getImageRectangle()) )
         {
            m_cacheTile->makeBlank();
         }

         if (startLine < m_currentRow)
         {
            // Must restart the compression process again.
            restart();
         }

         // Gobble any not needed lines.
         while(m_currentRow < startLine)
         {
            png_read_row(m_pngReadPtr, m_lineBuffer, NULL);
            ++m_currentRow;
         }
            
         switch (m_readMode)
         {
            case ossimPngRead8:
            {
               copyLines(ossim_uint8(0), stopLine);
               break;
            }
            case ossimPngRead16:
            {
               copyLines(ossim_uint16(0), stopLine);
               break;
            }
            case ossimPngRead8a:
            {
               if (m_useAlphaChannelFlag)
               {
                  copyLines(ossim_uint8(0), stopLine);
               }
               else
               {
                  // Will burn alpha value into the other bands.
                  copyLinesWithAlpha(ossim_uint8(0), stopLine);
               }
               break;
            }
            case ossimPngRead16a:
            {
               if (m_useAlphaChannelFlag)
               {
                  copyLines(ossim_uint16(0), stopLine); 
               }
               else
               {
                  // Will burn alpha value into the other bands.
                  copyLinesWithAlpha(ossim_uint16(0), stopLine);
               }
               break;
            }
            case ossimPngReadUnknown:
            default:
            {
               break; // should never happen.
            }
         }

         m_cacheTile->validate();
         
         tile->loadTile(m_cacheTile.get());
         
         // Add it to the cache for the next time.
         ossimAppFixedTileCache::instance()->addTile(m_cacheId,
                                                     m_cacheTile);
         
      } // End of reading for png file.
      
      origin.y += m_cacheSize.y;
      
   } // for (int tile = 0; tile < number_of_cache_tiles; ++tile)
   
   tile->validate();
}

ossimIrect
ossimPngReader::getImageRectangle(ossim_uint32 reduced_res_level) const
{
   return ossimIrect(0,
                     0,
                     getNumberOfSamples(reduced_res_level) - 1,
                     getNumberOfLines(reduced_res_level)   - 1);
}

bool ossimPngReader::saveState(ossimKeywordlist& kwl,
                               const char* prefix) const
{
   std::string p = ( prefix ? prefix : "" );
   std::string v = ossimString::toString(m_useAlphaChannelFlag).string();
   kwl.addPair( p, USE_ALPHA_KW, v, true );
   return ossimImageHandler::saveState(kwl, prefix);
}

bool ossimPngReader::loadState(const ossimKeywordlist& kwl,
                               const char* prefix)
{
   bool result = false;
   if (ossimImageHandler::loadState(kwl, prefix))
   {
      // this was causing core dumps.  Mainly because if prefix is null then
      // standard string core dumps.  So wrapped with OSSIM string that checks
      // for this and inits with empty string if null
      //
      ossimString value = kwl.findKey( ossimString(prefix).c_str(), USE_ALPHA_KW );
      if ( value.size() )
      {
         ossimString s = value;
         m_useAlphaChannelFlag = s.toBool();
      }
      result = open();
  }
   return result;
}

void ossimPngReader::setProperty(ossimRefPtr<ossimProperty> property)
{
   if ( property.valid() )
   {
      if ( property->getName().string() == USE_ALPHA_KW )
      {
         ossimString s;
         property->valueToString(s);
         m_useAlphaChannelFlag = s.toBool();
      }
      else
      {
         ossimImageHandler::setProperty(property);
      }
   }
}

ossimRefPtr<ossimProperty> ossimPngReader::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> prop = 0;
   if ( name.string() == USE_ALPHA_KW )
   {
      prop = new ossimBooleanProperty(name, m_useAlphaChannelFlag);
   }
   else
   {
      prop = ossimImageHandler::getProperty(name);
   }
   return prop;
}

void ossimPngReader::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back( ossimString(USE_ALPHA_KW) );
   ossimImageHandler::getPropertyNames(propertyNames);
}

bool ossimPngReader::open()
{
   static const char MODULE[] = "ossimPngReader::open()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n" << "File:  " << theImageFile.c_str()
         << "\n";
   }

   bool result = false;

   // Start with a clean slate.
   if (isOpen())
   {
      close();
   }

   // Check for empty filename.
   if ( theImageFile.size() )
   {
      // Open up a stream to the file.
      std::shared_ptr<ossim::istream> str = ossim::StreamFactoryRegistry::instance()->
         createIstream( theImageFile.string(), ios::in | ios::binary);
      if ( str )
      {
         // Pass to our open that takes a stream:
         result = open( str );
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReader::open() exit status: " << (result?"true":"false") << "\n";
   }
   
   return result;
}

bool ossimPngReader::open( std::shared_ptr<ossim::istream>& str,
                           const std::string& connectionString )
{
   bool result = open( str );
   if ( result )
   {
      theImageFile = connectionString;
   }
   return result;
}

bool ossimPngReader::open( std::shared_ptr<ossim::istream>& str )
{
   bool result = false;

   if (isOpen())
   {
      close();
   }
   
   if ( str )
   {
      str->seekg( 0, std::ios_base::beg );
      
      if ( checkSignature( *str ) )
      {
         // Store the pointer:
         m_str = str;
         
         result = readPngInit();
         if ( result )
         {
            result = initReader();
            if ( result )
            {
               completeOpen();
            }
         }
         else
         {
            destroy();
         }
      }
      else
      {
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimPngReader::open NOTICE:\n"
               << "Could not open stream!\n";
         }
      }
   }
   
   return result;
}

ossim_uint32 ossimPngReader::getTileWidth() const
{
   return ( m_tile.valid() ? m_tile->getWidth() : 0 );
}

ossim_uint32 ossimPngReader::getTileHeight() const
{
   return ( m_tile.valid() ? m_tile->getHeight() : 0 );
}

ossim_uint32 ossimPngReader::getNumberOfLines(ossim_uint32 reduced_res_level) const
{
   if (reduced_res_level == 0)
   {
      return m_imageRect.height();
   }
   else if ( theOverview.valid() )
   {
      return theOverview->getNumberOfLines(reduced_res_level);
   }

   return 0;
}

ossim_uint32 ossimPngReader::getNumberOfSamples(ossim_uint32 reduced_res_level) const
{
   if (reduced_res_level == 0)
   {
      return m_imageRect.width();
   }
   else if ( theOverview.valid() )
   {
      return theOverview->getNumberOfSamples(reduced_res_level);
   }

   return 0;
}

ossim_uint32 ossimPngReader::getImageTileWidth() const
{
   return 0;
}

ossim_uint32 ossimPngReader::getImageTileHeight() const
{
   return 0;
}

ossimString ossimPngReader::getShortName()const
{
   return ossimString("ossim_png_reader");
}
   
ossimString ossimPngReader::getLongName()const
{
   return ossimString("ossim png reader");
}

ossimString  ossimPngReader::getClassName()const
{
   return ossimString("ossimPngReader");
}

ossim_uint32 ossimPngReader::getNumberOfInputBands() const
{
   //---
   // NOTE:  If there is an alpha channel the input band will be one more than
   // the output bands.  For library purposes the output bands and input bands
   // are the same.
   //---
   return m_numberOfOutputBands;
}

ossim_uint32 ossimPngReader::getNumberOfOutputBands()const
{
   return m_numberOfOutputBands;
}

ossimScalarType ossimPngReader::getOutputScalarType() const
{
   return m_outputScalarType;
}

bool ossimPngReader::isOpen()const
{
   if ( m_str ) return m_str->good();

   return false;
}

double ossimPngReader::getMaxPixelValue(ossim_uint32 band)const
{
   //---
   // Note the size of m_maxPixelValue can be one greater than output bands
   // if there is an alpa channel.
   //---
   if (band < m_numberOfOutputBands)
   {
      return m_maxPixelValue[band];
   }
   return 255.0;
}

void ossimPngReader::restart()
{
   if ( m_str )
   {
      // Destroy the existing memory associated with png structs.
      if (m_pngReadPtr && m_pngReadInfoPtr)
      {
         png_destroy_read_struct(&m_pngReadPtr, &m_pngReadInfoPtr, NULL);
      }

      m_pngReadPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                            NULL,
                                            NULL,
                                            NULL);
      m_pngReadInfoPtr = png_create_info_struct(m_pngReadPtr);

      if ( setjmp( png_jmpbuf(m_pngReadPtr) ) )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "Error while reading.  File corrupted?  "
            << theImageFile
            << std::endl;
      
         return;
      }

      // Reset the file pointer.
      m_str->seekg( 0, std::ios_base::beg );
   
      //---
      // Pass the static read method to libpng to allow us to use our
      // c++ stream instead of doing "png_init_io (pp, ...);" with
      // c stream.
      //---
      png_set_read_fn( m_pngReadPtr,
                       (png_voidp)m_str.get(),
                       (png_rw_ptr)&ossimPngReader::pngReadData );

      //---
      // Note we won't do png_set_sig_bytes(png_ptr, 8) here because we are not
      // rechecking for png signature.
      //---
      png_read_info(m_pngReadPtr, m_pngReadInfoPtr);

      //---
      // If png_set_expand used:
      // Expand data to 24-bit RGB, or 8-bit grayscale,
      // with alpha if available.
      //---
      bool expandFlag = false;

      if ( m_pngColorType == PNG_COLOR_TYPE_PALETTE )
      {
         expandFlag = true;
      }
      if ( (m_pngColorType == PNG_COLOR_TYPE_GRAY) && (m_bitDepth < 8) )
      {
         expandFlag = true;
      }
      if ( png_get_valid(m_pngReadPtr, m_pngReadInfoPtr, PNG_INFO_tRNS) )
      {
         expandFlag = true;
      }

      //---
      // If png_set_packing used:
      // Use 1 byte per pixel in 1, 2, or 4-bit depth files. */
      //---
      bool packingFlag = false;

      if ( (m_bitDepth < 8) && (m_pngColorType == PNG_COLOR_TYPE_GRAY) )
      {
         packingFlag = true;
      }

      if (expandFlag)
      {
         png_set_expand(m_pngReadPtr);
      }
      if (packingFlag)
      {
         png_set_packing(m_pngReadPtr);
      }

      // Gamma correction.
      //    ossim_float64 gamma;
      //    if (png_get_gAMA(m_pngReadPtr, m_pngReadInfoPtr, &gamma))
      //    {
      //       png_set_gamma(m_pngReadPtr, display_exponent, gamma);
      //    }

      //---
      // Turn on interlace handling... libpng returns just 1 (ie single pass)
      //  if the image is not interlaced
      //---
      png_set_interlace_handling (m_pngReadPtr);

      //---
      // Update the info structures after the transformations take effect
      //---
      png_read_update_info (m_pngReadPtr, m_pngReadInfoPtr);
   
      // We're back on row 0 or first line.
      m_currentRow = 0;
   }
}

bool ossimPngReader::checkSignature( std::istream& str )
{
   bool result = false;

   //---
   // Verify the file is a png by checking the first eight bytes:
   // 0x 89 50 4e 47 0d 0a 1a 0a
   //---
   ossim_uint8 sig[8];
   str.read( (char*)sig, 8);
   if ( str.good() )
   {
      if ( png_sig_cmp(sig, 0, 8) == 0 )
      {
         result = true;
      }
   }

   return result;
}

ossimRefPtr<ossimImageGeometry> ossimPngReader::getImageGeometry()
{
   if ( !theGeometry )
   {
      //---
      // Check factory for external geom:
      //---
      theGeometry = getExternalImageGeometry();

      if ( !theGeometry )
      {
         // Fist time through:
         theGeometry = new ossimImageGeometry();

         //---
         // This code does not call ossimImageGeometryRegistry::extendGeometry
         // by design to avoid wasted factory calls.
         //---
      }

      // Set image things the geometry object should know about.
      initImageParameters( theGeometry.get() );
   }
   return theGeometry;
}

bool ossimPngReader::readPngInit()
{
   bool result = false;
   if ( m_str )
   {
      if ( m_str->good() )
      {
         m_pngReadPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                               NULL,
                                               NULL,
                                               NULL);
         if ( m_pngReadPtr )
         {
            m_pngReadInfoPtr = png_create_info_struct(m_pngReadPtr);
            if ( m_pngReadInfoPtr )
            {
               if ( setjmp( png_jmpbuf(m_pngReadPtr) ) == 0 )
               {
                  //---
                  // Pass the static read method to libpng to allow us to use our
                  // c++ stream instead of doing "png_init_io (pp, ...);" with
                  // c stream.
                  //---
                  png_set_read_fn( m_pngReadPtr,
                                   (png_voidp)m_str.get(),
                                   (png_rw_ptr)&ossimPngReader::pngReadData );
                  png_set_sig_bytes(m_pngReadPtr, 8);
                  png_read_info(m_pngReadPtr, m_pngReadInfoPtr);
                  result = true;
               }
            }
            else
            {
               // Out of memory?
               png_destroy_read_struct(&m_pngReadPtr, NULL, NULL);
            }
         }
      }
   }
   return result;
   
} // End: ossimPngReader::readPngInit()

bool ossimPngReader::initReader()
{
   bool result = true;
   
   ossim_uint32 height    = png_get_image_height(m_pngReadPtr, m_pngReadInfoPtr);
   ossim_uint32 width     = png_get_image_width(m_pngReadPtr, m_pngReadInfoPtr);
   m_bitDepth            = png_get_bit_depth(m_pngReadPtr, m_pngReadInfoPtr);
   m_pngColorType        = png_get_color_type(m_pngReadPtr, m_pngReadInfoPtr);
   
   m_imageRect = ossimIrect(0, 0, width  - 1, height - 1);
   
   if (m_bitDepth == 16)
   {
      // png_set_strip_16 (m_pngReadPtr);
      m_bytePerPixelPerBand = 2;
      m_outputScalarType = OSSIM_UINT16;
   }
   else
   {
      m_bytePerPixelPerBand = 1;
   }

   // Set the read mode from scalar and color type.
   if (m_outputScalarType == OSSIM_UINT8)
   {
      if ( (m_pngColorType == PNG_COLOR_TYPE_RGB_ALPHA) ||
           (m_pngColorType == PNG_COLOR_TYPE_GRAY_ALPHA) )
      {
         m_readMode = ossimPngRead8a;
      }
      else
      {
         m_readMode = ossimPngRead8;
      }
   }
   else
   {
      if ( (m_pngColorType == PNG_COLOR_TYPE_RGB_ALPHA) ||
           (m_pngColorType == PNG_COLOR_TYPE_GRAY_ALPHA) )
      {
         m_readMode = ossimPngRead16a;
      }
      else
      {
         m_readMode = ossimPngRead16;
      }

      // Set the swap flag.  PNG stores data in network byte order(big endian).
      if(ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
      {
         m_swapFlag = true;
      }
   }
   
   //---
   // If png_set_expand used:
   // Expand data to 24-bit RGB, or 8-bit grayscale,
   // with alpha if available.
   //---
   bool expandFlag = false;

   if ( m_pngColorType == PNG_COLOR_TYPE_PALETTE )
   {
      expandFlag = true;
   }
   if ( (m_pngColorType == PNG_COLOR_TYPE_GRAY) && (m_bitDepth < 8) )
   {
      expandFlag = true;
   }
   if ( png_get_valid(m_pngReadPtr, m_pngReadInfoPtr, PNG_INFO_tRNS) )
   {
      expandFlag = true;
   }

   //---
   // If png_set_packing used:
   // Use 1 byte per pixel in 1, 2, or 4-bit depth files. */
   //---
   bool packingFlag = false;

   if ( (m_bitDepth < 8) && (m_pngColorType == PNG_COLOR_TYPE_GRAY) )
   {
      packingFlag = true;
   }

   if (expandFlag)
   {
       png_set_expand(m_pngReadPtr);
   }
   if (packingFlag)
   {
      png_set_packing(m_pngReadPtr);
   }

   // Gamma correction.
   // ossim_float64 gamma;
   // if (png_get_gAMA(m_pngReadPtr, m_pngReadInfoPtr, &gamma))
   // {
   //    png_set_gamma(png_ptr, display_exponent, gamma);
   // }

   //---
   // Turn on interlace handling... libpng returns just 1 (ie single pass)
   //  if the image is not interlaced
   //---
   m_interlacePasses = png_set_interlace_handling (m_pngReadPtr);

   //---
   // Update the info structures after the transformations take effect
   //---
   png_read_update_info (m_pngReadPtr, m_pngReadInfoPtr);

   // TODO:
   // Add check for image offsets.
   // Add check for resolution.
   // Add check for colormap.

   switch (m_pngColorType)
   {
      case PNG_COLOR_TYPE_RGB:           /* RGB */
      {
         m_numberOfInputBands  = 3;
         m_numberOfOutputBands = 3;
         break;
      }  
      case PNG_COLOR_TYPE_RGB_ALPHA:     /* RGBA */
      {
         m_numberOfInputBands  = 4;
         if (m_useAlphaChannelFlag)
         {
            m_numberOfOutputBands = 4;     
         }
         else
         {
            m_numberOfOutputBands = 3;    
         }         
         break;
      }
      case PNG_COLOR_TYPE_GRAY:          /* Grayscale */
      {
         m_numberOfInputBands = 1;
         m_numberOfOutputBands = 1;
         break;
      }  
      case PNG_COLOR_TYPE_GRAY_ALPHA:    /* Grayscale + alpha */
      {
         m_numberOfInputBands = 2;
         if (m_useAlphaChannelFlag)
         {
            m_numberOfOutputBands = 2;     
         }
         else
         {
            m_numberOfOutputBands = 1;
         }
        break;
      }
     case PNG_COLOR_TYPE_PALETTE:       /* Indexed */
     {
        m_numberOfInputBands  = 3;
        m_numberOfOutputBands = 3;
        break;
     }  
     default:                   /* Unknown type */
     {
        result = false;
     }
   }

   if ( result )
   {
      m_lineBufferSizeInBytes = png_get_rowbytes(m_pngReadPtr, m_pngReadInfoPtr);
      
      // Set the max pixel value.
      setMaxPixelValue();
      
      // Set to OSSIM_USHORT11 for use of specialized tile.
      if (m_maxPixelValue[0] == 2047.0)
      {
         m_outputScalarType = OSSIM_USHORT11;
      }

      // We're on row 0 or first line.
      m_currentRow = 0;
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimPngReader::initReader DEBUG:"
            << "\nm_imageRect:                     " << m_imageRect
            << "\nm_bitDepth:                      " << int(m_bitDepth)
            << "\nm_pngColorType:                  "
            <<  getPngColorTypeString().c_str()
            << "\nm_numberOfInputBands:            " << m_numberOfInputBands
            << "\nm_numberOfOutputBands:           " << m_numberOfOutputBands
            << "\nm_bytePerPixelPerBand:           " << m_bytePerPixelPerBand
            << "\nm_lineBufferSizeInBytes:         " << m_lineBufferSizeInBytes
            << "\nm_interlacePasses:               " << m_interlacePasses
            << "\npalette expansion:                "
            << (expandFlag?"on":"off")
            << "\npacking (1,2,4 bit to one byte):  "
            << (packingFlag?"on":"off")
            << "\nm_readMode:                      " << m_readMode
            << "\nm_swapFlag:                      " << m_swapFlag
            << std::endl;
         
         for (ossim_uint32 band = 0; band < m_numberOfInputBands; ++band)
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "max[" << band << "]:  " << m_maxPixelValue[band]
               << std::endl;
         }
      }
   }

   return result;
   
} // End: ossimPngReader::initReader()

void ossimPngReader::readPngVersionInfo()
{
   ossimNotify(ossimNotifyLevel_WARN)
      << "ossimPngReader::readPngVersionInfo\nCompiled with:"
      << "\nlibpng " << PNG_LIBPNG_VER_STRING
      << " using libpng " << PNG_LIBPNG_VER
      << "\nzlib " << ZLIB_VERSION " using zlib "
      << zlib_version << std::endl;
}

ossimString ossimPngReader::getPngColorTypeString() const
{
   ossimString result = "unknown";
   if (m_pngColorType == PNG_COLOR_TYPE_GRAY)
   {
      return ossimString("PNG_COLOR_TYPE_GRAY");
   }
   else if (m_pngColorType == PNG_COLOR_TYPE_PALETTE)
   {
      return ossimString("PNG_COLOR_TYPE_PALETTE");
   }
   else if (m_pngColorType == PNG_COLOR_TYPE_RGB)
   {
      return ossimString("PNG_COLOR_TYPE_RGB");
   }
   else if (m_pngColorType == PNG_COLOR_TYPE_RGB_ALPHA)
   {
      return ossimString("PNG_COLOR_TYPE_RGB_ALPHA");
   }
   else if (m_pngColorType == PNG_COLOR_TYPE_GRAY_ALPHA)
   {
      return ossimString("PNG_COLOR_TYPE_GRAY_ALPHA");
   }

   return ossimString("unknown");
}

void ossimPngReader::setMaxPixelValue()
{
   ossim_uint32 band;
   m_maxPixelValue.resize(m_numberOfInputBands);
   for (band = 0; band < m_numberOfInputBands; ++band)
   {
      m_maxPixelValue[band] = 0.0;
   }
   
   if (png_get_valid(m_pngReadPtr, m_pngReadInfoPtr, PNG_INFO_sBIT ))
   {
      png_color_8p sig_bit;
      png_get_sBIT(m_pngReadPtr, m_pngReadInfoPtr, &sig_bit);
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimPngReader::setMaxPixelValue DEBUG:"
            << "\nsig_bit->red:   " << int(sig_bit->red)
            << "\nsig_bit->green: " << int(sig_bit->green)
            << "\nsig_bit->blue:  " << int(sig_bit->blue)
            << "\nsig_bit->gray:  " << int(sig_bit->gray)            
            << "\nsig_bit->alpa:  " << int(sig_bit->alpha)
            << endl;
      }
      switch (m_pngColorType)
      {
         case PNG_COLOR_TYPE_RGB:           /* RGB */
            m_maxPixelValue[0] = pow(2.0, double(sig_bit->red))-1.0;
            m_maxPixelValue[1] = pow(2.0, double(sig_bit->green))-1.0;
            m_maxPixelValue[2] = pow(2.0, double(sig_bit->blue))-1.0;
            break;
         case PNG_COLOR_TYPE_RGB_ALPHA:     /* RGBA */
            m_maxPixelValue[0] = pow(2.0, double(sig_bit->red))-1.0;
            m_maxPixelValue[1] = pow(2.0, double(sig_bit->green))-1.0;
            m_maxPixelValue[2] = pow(2.0, double(sig_bit->blue))-1.0;
            m_maxPixelValue[3] = pow(2.0, double(sig_bit->alpha))-1.0;
            break;
         case PNG_COLOR_TYPE_GRAY:          /* Grayscale */
            m_maxPixelValue[0] = pow(2.0, double(sig_bit->gray))-1.0;
            break;
         case PNG_COLOR_TYPE_GRAY_ALPHA:    /* Grayscale + alpha */            
            m_maxPixelValue[0] = pow(2.0, double(sig_bit->gray))-1.0;
            m_maxPixelValue[1] = pow(2.0, double(sig_bit->alpha))-1.0;
            break;
         case PNG_COLOR_TYPE_PALETTE:       /* Indexed */
            m_maxPixelValue[0] = 255.0;
            m_maxPixelValue[1] = 255.0;
            m_maxPixelValue[2] = 255.0;
            break;
         default:                   /* Aie! Unknown type */
            break;
      }
   }

   // Sanity check.
   for (ossim_uint32 band = 0; band < m_numberOfInputBands; ++band)
   {
      if (m_maxPixelValue[band] == 0.0)
      {
         if (m_bitDepth <= 8)
         {
            m_maxPixelValue[band] = 255.0;
         }
         else
         {
            m_maxPixelValue[band] = 65535.0;
         }
      }
   }
}

template <class T>  void ossimPngReader::copyLines(
   T /*dummy*/,  ossim_uint32 stopLine)
{
   const ossim_uint32 SAMPLES = m_imageRect.width();

   T* src = (T*)m_lineBuffer;
   std::vector<T*> dst(m_numberOfOutputBands);

   ossim_uint32 band = 0;
   for (band = 0; band < m_numberOfOutputBands; ++band)
   {
      dst[band] = (T*) m_cacheTile->getBuf(band);
   }
   
   ossim_int32 bufIdx = 0;
   
   while (m_currentRow <= stopLine)
   {
      // Read a line from the jpeg file.
      png_read_row(m_pngReadPtr, m_lineBuffer, NULL);
      ++m_currentRow;

      if(m_swapFlag)
      {
         ossimEndian endian;
         endian.swap(src, SAMPLES*m_numberOfInputBands);
      }

      //---
      // Copy the line which is band interleaved by pixel the the band
      // separate buffers.
      //---
      ossim_uint32 index = 0;
      for (ossim_uint32 sample = 0; sample < SAMPLES; ++sample)
      {
         for (band = 0; band < m_numberOfOutputBands; ++band)
         {
            dst[band][bufIdx] = src[index];
            ++index;
         }
         ++bufIdx;
      }
   }
}

template <class T> void ossimPngReader::copyLinesWithAlpha(
   T, ossim_uint32 stopLine)
{
   ossim_float64 denominator;
   if (m_outputScalarType == OSSIM_UINT8)
   {
      denominator = m_maxPixelValue[m_numberOfInputBands-1];
   }
   else
   {
      denominator = m_maxPixelValue[m_numberOfInputBands-1];
   }

   const ossim_uint32 SAMPLES = m_imageRect.width();
   
   T* src = (T*) m_lineBuffer;

   std::vector<T*> dst(m_numberOfOutputBands);
   std::vector<T> p(m_numberOfOutputBands);
   
   ossim_float64 alpha;

   const ossim_float64 MIN_PIX  = m_cacheTile->getMinPix(0);
   const ossim_float64 MAX_PIX  = m_cacheTile->getMaxPix(0);
   const ossim_float64 NULL_PIX = m_cacheTile->getNullPix(0);
 
   ossim_uint32 band = 0;
   for (band = 0; band < m_numberOfOutputBands; ++band)
   {
      dst[band] = (T*)m_cacheTile->getBuf(band);
   }

   ossim_int32 dstIdx = 0;
   
   while (m_currentRow <= stopLine)
   {
      // Read a line from the jpeg file.
      png_read_row(m_pngReadPtr, m_lineBuffer, NULL);
      ++m_currentRow;

      if(m_swapFlag)
      {
         ossimEndian endian;
         endian.swap(src, SAMPLES*m_numberOfInputBands);
      }
      
      //---
      // Copy the line which is band interleaved by pixel the the band
      // separate buffers.
      //---
      ossim_uint32 srcIdx = 0;
      for (ossim_uint32 sample = 0; sample < SAMPLES; ++sample)
      {
         // Copy the pixels.
         for (band = 0; band < m_numberOfOutputBands; ++band)
         {
            p[band] = src[srcIdx++];
         }
         
         // Get the alpha channel.
         alpha = src[srcIdx++];
         alpha = alpha / denominator;
         
         if (alpha == 1.0)
         {
            for (band = 0; band < m_numberOfOutputBands; ++band)
            {
               dst[band][dstIdx] = p[band];
            }
         }
         else if (alpha == 0.0)
         {
            for (band = 0; band < m_numberOfOutputBands; ++band)
            {
               dst[band][dstIdx] = static_cast<T>(NULL_PIX);
            }
         }
         else
         {
            for (band = 0; band < m_numberOfOutputBands; ++band)
            {
               ossim_float64 f = p[band];
               f = f * alpha;
               if (f != NULL_PIX)
               {
                  dst[band][dstIdx] =
                     static_cast<T>( (f>=MIN_PIX) ?
                                     ( (f<=MAX_PIX) ? f : MAX_PIX ) :
                                     MIN_PIX );
               }
               else
               {
                  dst[band][dstIdx] = static_cast<T>(NULL_PIX);
               }
            }
         }
         ++dstIdx; // next sample...
            
      } // End of sample loop.
      
   } // End of line loop.
}

// Static function to read from c++ stream.
void ossimPngReader::pngReadData( png_structp png_ptr, png_bytep data, png_size_t length )
{
   ossim::istream* str = (ossim::istream*)png_get_io_ptr(png_ptr);
   if ( str )
   {
      str->read( (char*)data, length );
   }
}

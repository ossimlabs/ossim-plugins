//----------------------------------------------------------------------------
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: Wrapper class to compress whole tiles using kdu_analysis
// object.
//
//----------------------------------------------------------------------------
// $Id: ossimOpjCompressor.cpp 22513 2013-12-11 21:45:51Z dburken $

#include "ossimOpjCompressor.h"
#include "ossimOpjCommon.h"
#include "ossimOpjKeywords.h"

#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageGeometry.h>

#include <ossim/support_data/ossimGeoTiff.h>
#include <ossim/support_data/ossimGmlSupportData.h>

#include <sstream>

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimOpjCompressor:debug" your_app_args
//---
static ossimTrace traceDebug("ossimOpjCompressor:debug");

static const ossim_int32 DEFAULT_LEVELS = 5;

// Callback stream function prototypes:
static OPJ_SIZE_T ossim_opj_ostream_write( void * p_buffer,
                                           OPJ_SIZE_T p_nb_bytes,
                                           void * p_user_data )
{
   OPJ_SIZE_T count = 0;
   if ( p_user_data )
   {
      ostream* str = (ostream*)p_user_data;
      std::streamsize bytesToRead = (std::streamsize)p_nb_bytes;
      str->write( (char*) p_buffer, bytesToRead );
      count = p_nb_bytes;
   }
   return count;
}

// Callback function prototype for skip function:
static OPJ_OFF_T ossim_opj_ostream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
   OPJ_OFF_T pos = 0;
   if ( p_user_data )
   {
      ostream* str = (ostream*)p_user_data;
      str->seekp( p_nb_bytes, std::ios_base::cur );
      pos = p_nb_bytes;
   }
   return pos;
}

/// Callback function prototype for seek function:
static OPJ_BOOL ossim_opj_ostream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
   if ( p_user_data )
   {
      ostream* str = (ostream*)p_user_data;
      // str->seekp( p_nb_bytes, std::ios_base::cur );
      str->seekp( p_nb_bytes, std::ios_base::beg );         
   }
   return OPJ_TRUE;
}

static void ossim_opj_free_user_ostream_data( void * p_user_data )
{
   if ( p_user_data )
   {
      ostream* str = (ostream*)p_user_data;
      str->flush();
   }
}

// Matches ossimOpjCompressionQuality enumeration:
static const ossimString COMPRESSION_QUALITY[] = { "unknown",
                                                   "user_defined",
                                                   "numerically_lossless",
                                                   "visually_lossless",
                                                   "lossy" };


ossimOpjCompressor::ossimOpjCompressor()
   :
   m_params(0),
   m_codec(0),
   m_stream(0),
   m_image(0),
   m_imageRect(),
   m_reversible(true),
   m_alpha(false),
   m_levels(0),
   m_threads(1),
   m_options(),
   m_qualityType(ossimOpjCompressor::OPJ_NUMERICALLY_LOSSLESS)
{
   //---
   // Uncomment for debug mode:
   // traceDebug.setTraceFlag(true);
   //---
}

ossimOpjCompressor::~ossimOpjCompressor()
{
   finish();
}

void ossimOpjCompressor::create(std::ostream* os,
                                ossimScalarType scalar,
                                ossim_uint32 bands,
                                const ossimIrect& imageRect,
                                const ossimIpt& tileSize,
                                bool jp2)
{
   static const char MODULE[] = "ossimOpjCompressor::create";
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_WARN) << MODULE << " entered...\n";
   }
   
#if 0 /* Please leave for debug. (drb) */
   cout << "levels:      " << m_levels
        << "\nreversible:  " << m_reversible
        << "\nthreads:     " << m_threads
        << "\nscalar:    " << scalar
        << "\nbands:     " << bands
        << "\nimageRect: " << imageRect
        << "\ntileSize:  " << tileSize
        << "\njp2:       " << jp2
        << endl;
#endif

   // In case we were reused.
   finish();

   if ( !os )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null stream passed to method!";
      throw ossimException(errMsg);
   }
   
   if ( !os->good() )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Stream state has error!";
      throw ossimException(errMsg);
   }
   
   if ( ossim::getActualBitsPerPixel(scalar) > 31 )
   {
      // Data is not reversible.
      if ( m_reversible )
      {
         std::string errMsg = MODULE;
         errMsg += " ERROR: Reversible processing not possible with 32 bit data!";
         throw ossimException(errMsg);
      }
   }
   
   // Store for tile clip.
   m_imageRect = imageRect;

   m_stream = createOpjStream( os );
   if ( !m_stream )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: OPJ stream creation failed!";
      throw ossimException(errMsg); 
   }

   // Requests the insertion of TLM (tile-part-length) marker.
   // setTlmTileCount(tilesToWrite);
   
   // Set up stream:
   initOpjCodingParams( jp2, tileSize, imageRect );
   if ( !m_params )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: coding parameters creation failed!";
      throw ossimException(errMsg);
   }

   m_codec = createOpjCodec( jp2 );
   if ( !m_codec )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: code creation failed!";
      throw ossimException(errMsg);
   }   

   // Set rates rates/distorsion
   // parameters->cp_disto_alloc = 1;

   // ossim::print( cout, *m_params );
   
   if (m_alpha)
   {
      if ( (bands != 1) && (bands != 3) )
      {
         m_alpha = false;
         if ( traceDebug() )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "Alpha channel being unset! Can only be used with "
               << "one or three band data.\n"
               << "Source image bands: " << bands << "\n";
         }
      }
   }

   // Create an image without allocating memory(for tile based).
   m_image = createOpjImage( scalar, bands, imageRect );
   if ( !m_image )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Unsupported input image type!";
      throw ossimException(errMsg);
   }

   if ( !opj_setup_encoder( m_codec, m_params, m_image) )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: opj_setup_encoder failed!";
      throw ossimException(errMsg); 
   }

   // openJp2Codestream();
   
   if ( traceDebug() )
   {
      ossim::print( ossimNotify(ossimNotifyLevel_DEBUG), *m_params );
      
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exiting...\n";
   }
}

void ossimOpjCompressor::openJp2Codestream()
{
   //---
   // Start compression...
   // Matching "opj_end_compress" is in ossimOpjCompressor::finish().
   //---
   if ( !opj_start_compress(m_codec, m_image, m_stream) )
   {
      std::string errMsg = "ossimOpjCompressor::openJp2Codestream ERROR: opj_start_compress failed!";
      throw ossimException(errMsg);
   }
}

bool ossimOpjCompressor::writeTile(
   ossimImageData* srcTile, ossim_uint32 tileIndex )
{
   bool result = false;

   if ( srcTile )
   {
      if (srcTile->getDataObjectStatus() != OSSIM_NULL)
      {
         ossimIrect tileRect = srcTile->getImageRectangle();

         // Write the tile out:
         if ( opj_write_tile( m_codec,
                              tileIndex,
                              srcTile->getUcharBuf(),
                              srcTile->getDataSizeInBytes(),
                              m_stream ) ) 
         {
            result = true;
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossimOpjCompressor::writeTile ERROR on tile index: "
               << tileIndex << std::endl;
         }
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimOpjCompressor::writeTile ERROR null tile passed in!" << endl;
      }
   }
   
   return result;
   
} // End: ossimOpjCompressor::writeTile

void ossimOpjCompressor::finish()
{
   if ( m_stream )
   {
      // Every opj_start_compress must have an end.
      opj_end_compress( m_codec, m_stream );

      opj_stream_destroy( m_stream );
      m_stream = 0;
   }
   if ( m_codec )
   {
      opj_destroy_codec( m_codec );
      m_codec = 0;
   }
   if ( m_image )
   {
      opj_image_destroy( m_image );
      m_image = 0;
   }
}

void ossimOpjCompressor::setQualityType(ossimOpjCompressionQuality type)
{
   m_qualityType = type;

   //---
   // Set the reversible flag for appropriate type.
   // Not sure what to set for unknown and user defined but visually lossless
   // and lossy need to set the reversible flag to false.
   //---
   if ( (type == ossimOpjCompressor::OPJ_VISUALLY_LOSSLESS) ||
        (type == ossimOpjCompressor::OPJ_LOSSY) )
   {
      setReversibleFlag(false);
   }
   else
   {
      setReversibleFlag(true);
   }
}

ossimOpjCompressor::ossimOpjCompressionQuality ossimOpjCompressor::getQualityType() const
{
   return m_qualityType;
   
}

void ossimOpjCompressor::setReversibleFlag(bool reversible)
{
   m_reversible = reversible;
}

bool ossimOpjCompressor::getReversibleFlag() const
{
   return m_reversible;
}

void ossimOpjCompressor::setAlphaChannelFlag(bool flag)
{
   m_alpha = flag;
}

bool ossimOpjCompressor::getAlphaChannelFlag() const
{
   return m_alpha;
}

void ossimOpjCompressor::setLevels(ossim_int32 levels)
{
   if (levels)
   {
      m_levels = levels;
   }
}

ossim_int32 ossimOpjCompressor::getLevels() const
{
   return m_levels;
}

void ossimOpjCompressor::setThreads(ossim_int32 threads)
{
   if (threads)
   {
      m_threads = threads;
   }
}

ossim_int32 ossimOpjCompressor::getThreads() const
{
   return m_threads;
}

void ossimOpjCompressor::setOptions(const std::vector<ossimString>& options)
{
   std::vector<ossimString>::const_iterator i = options.begin();
   while ( i != options.end() )
   {
      m_options.push_back( (*i) );
      ++i;
   }
}

bool ossimOpjCompressor::setProperty(ossimRefPtr<ossimProperty> property)
{
   bool consumed = false;
   
   if ( property.valid() )
   {
      ossimString key = property->getName();

      if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimOpjCompressor::setProperty DEBUG:"
            << "\nkey: " << key
            << " values: " << property->valueToString() << std::endl;
      }
      
      if (key == ossimKeywordNames::COMPRESSION_QUALITY_KW)
      {
         setQualityTypeString(property->valueToString());
         consumed = true;
      }
      else if ( (key == LEVELS_KW) || (key ==  "Clevels") )
      {
         m_levels = property->valueToString().toInt32();
         consumed = true;
      }
      else if( (key == REVERSIBLE_KW) || (key == "Creversible") )
      {
         setReversibleFlag(property->valueToString().toBool());
         consumed = true;
      }
      else if (key == ADD_ALPHA_CHANNEL_KW)
      {
         m_alpha = property->valueToString().toBool();
         consumed = true;
      }
      else if ( key == THREADS_KW)
      {
         m_threads = property->valueToString().toInt32();
         consumed = true;
      }
      else if ( (key == "Clayers") ||
                (key == "Cprecincts") )
      {
         // Generic options passed through kdu_params::parse_string.

         // Make in the form of "key=value" for kdu_params::parse_string.
         ossimString option = key;
         option += "=";
         option += property->valueToString();
         
         // Add to list.
         m_options.push_back(option);
         
         consumed = true;
      } 
   }

   return consumed;
}

ossimRefPtr<ossimProperty> ossimOpjCompressor::getProperty(
   const ossimString& name)const
{
   ossimRefPtr<ossimProperty> p = 0;

   if (name == ossimKeywordNames::COMPRESSION_QUALITY_KW)
   {
      // property value
      ossimString value = getQualityTypeString();

      if ( (value == COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_USER_DEFINED] ) ||
           (value == COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_UNKNOWN])
          )
      {
         value = COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_NUMERICALLY_LOSSLESS];
      }
         
      // constraint list
      vector<ossimString> constraintList;
      constraintList.push_back(COMPRESSION_QUALITY[ossimOpjCompressor::
                                                   OPJ_NUMERICALLY_LOSSLESS]);
      constraintList.push_back(COMPRESSION_QUALITY[ossimOpjCompressor::
                                                   OPJ_VISUALLY_LOSSLESS]);
      constraintList.push_back(COMPRESSION_QUALITY[ossimOpjCompressor::
                                                   OPJ_LOSSY]);
      
      p = new ossimStringProperty(name,
                                  value,
                                  false, // not editable
                                  constraintList);
   }
   else if (name == LEVELS_KW)
   {
      p = new ossimNumericProperty(name, ossimString::toString(m_levels));
   }
   else if (name == REVERSIBLE_KW)
   {
      p = new ossimBooleanProperty(name, m_reversible);
   }
   else if (name == ADD_ALPHA_CHANNEL_KW)
   {
      p = new ossimBooleanProperty(name, m_alpha);
   }
   else if (name == THREADS_KW)
   {
      p = new ossimNumericProperty(name, ossimString::toString(m_threads));
   }   
   
   return p;
}

void ossimOpjCompressor::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(ossimKeywordNames::COMPRESSION_QUALITY_KW);
   propertyNames.push_back(LEVELS_KW);
   propertyNames.push_back(REVERSIBLE_KW);
   propertyNames.push_back(THREADS_KW);
}

bool ossimOpjCompressor::saveState(ossimKeywordlist& kwl,
                                      const char* prefix)const
{
   kwl.add( prefix,
            ossimKeywordNames::COMPRESSION_QUALITY_KW,
            getQualityTypeString().c_str(),
            true );
   
   kwl.add( prefix,
            LEVELS_KW,
            ossimString::toString(m_levels),
            true );
   
   kwl.add( prefix,
            REVERSIBLE_KW,
            ossimString::toString(m_reversible),
            true );

   kwl.add( prefix,
            ADD_ALPHA_CHANNEL_KW,
            ossimString::toString(m_alpha),
            true ); 
   
   kwl.add( prefix,
            THREADS_KW,
            ossimString::toString(m_threads),
            true );

   std::vector<ossimString>::size_type size = m_options.size();
   for (ossim_uint32 i = 0; i < size; ++i)
   {
      ossimString key = "option";
      key += ossimString::toString(i);
      
      kwl.add( prefix,
               key.c_str(),
               m_options[i].c_str(),
               true );
   }
   
   return true;
}

bool ossimOpjCompressor::loadState(const ossimKeywordlist& kwl,
                                      const char* prefix)
{
   const char* value = 0;

   value = kwl.find(prefix, ossimKeywordNames::COMPRESSION_QUALITY_KW);
   if(value)
   {
      setQualityTypeString( ossimString(value) );
   }
   
   value = kwl.find(prefix, LEVELS_KW);
   if(value)
   {
      m_levels = ossimString(value).toInt32();
   }
   
   value = kwl.find(prefix, REVERSIBLE_KW);
   if(value)
   {
      setReversibleFlag(ossimString(value).toBool());
   }

   value = kwl.find(prefix, ADD_ALPHA_CHANNEL_KW);
   if(value)
   {
      m_alpha = ossimString(value).toBool();
   }
   
   value = kwl.find(prefix, THREADS_KW);
   if(value)
   {
      m_threads = ossimString(value).toInt32();
   }

   ossimString searchKey;
   if (prefix)
   {
      searchKey = prefix;
   }
   searchKey += "option";
   ossim_uint32 nOptions = kwl.numberOf(searchKey);
   for (ossim_uint32 i = 0; i < nOptions; ++i)
   {
      ossimString key = searchKey;
      key += ossimString::toString(i);
      
      const char* lookup = kwl.find(key.c_str());
      if (lookup)
      {
         m_options.push_back(ossimString(lookup));
      }
   }
   
   return true;
}

bool ossimOpjCompressor::writeGeotiffBox(std::ostream* stream,
                                         const ossimImageGeometry* geom,
                                         const ossimIrect& rect,
                                         const ossimFilename& tmpFile,
                                         ossimPixelType pixelType)
{
   // cout << "ossimOpjCompressor::writeGeotiffBox entered..." << endl;
   
   
   bool result = false;

   if ( stream && geom )
   {
      ossimRefPtr<const ossimImageGeometry> imgGeom = geom;
      ossimRefPtr<const ossimProjection> proj = imgGeom->getProjection();
      if ( proj.valid() )
      {
         //---
         // Make a temp file.  No means currently write a tiff straight to
         // memory.
         //---
         
         // Buffer to hold the tiff box.
         std::vector<ossim_uint8> buf;
         
         // Write to buffer.
         if ( ossimGeoTiff::writeJp2GeotiffBox(tmpFile,
                                               rect,
                                               proj.get(),
                                               buf,
                                               pixelType) )
         {
            //---
            // JP2 box type uuid in ascii expressed as an int.
            // "u(75), u(75), i(69), d(64)"
            //---
            const ossim_uint8 UUID[4] = 
            {
               0x75, 0x75, 0x69, 0x64
            };

            ossim_uint32 lbox = buf.size() + 8;

            // cout << "gbox tellp: " << stream->tellp() << endl;
            // cout << "lbox: " << lbox << endl;
            
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
               ossimEndian endian;
               endian.swap( lbox );
            }
            
            stream->write( (char*)&lbox, 4 ); // LBox
            
            stream->write((const char*)UUID, 4); // TBox

            // DBox: geotiff
            stream->write((const char*)&buf.front(), (std::streamsize)buf.size());

            result = true;

            // cout << "wrote box!!!" << endl;
         }
      }
   }
   
   return result;
   
} // End: ossimOpjCompressor::writeGeotiffBox

bool ossimOpjCompressor::writeGmlBox( std::ostream* stream,
                                      const ossimImageGeometry* geom,
                                      const ossimIrect& rect )
{
   // cout << "ossimOpjCompressor::writeGmlBox entered..." << endl;
   
   bool result = false;

   if ( stream && geom )
   {
      ossimGmlSupportData* gml = new ossimGmlSupportData();

      if ( gml->initialize( geom, rect ) )  
      {
         const ossim_uint8 ASOC_BOX_ID[4] = 
         {
            0x61, 0x73, 0x6F, 0x63
         };

         const ossim_uint8 LBL_BOX_ID[4] = 
         {
             0x6C, 0x62, 0x6C, 0x20
         };

         const ossim_uint8 XML_BOX_ID[4] = 
         {
            0x78, 0x6D, 0x6C, 0x20
         };
         
         // Write the xml to a stream.
         ostringstream xmlStr;
         if ( gml->write( xmlStr ) )
         {
             ossim_uint32 xmlDataSize = xmlStr.str().size();

            // Set the 1st asoc box size and type
            ossim_uint32 boxSize = xmlDataSize + 17 + 8 + 8 + 8 + 8 + 8 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
               ossimEndian endian;
               endian.swap( boxSize );
            }
            stream->write((char*)&boxSize, 4); // 1st asoc size
            stream->write((const char*)ASOC_BOX_ID, 4); // 1st asoc type

            // Set the 1st lbl box size, type, and data
            boxSize = 8 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            stream->write((char*)&boxSize, 4); // 1st lbl size
            stream->write((const char*)LBL_BOX_ID, 4); // 1st lbl type
            stream->write("gml.data", 8); // 1st lbl data

            // Set the 2nd asoc box size and type
            boxSize = xmlDataSize + 17 + 8 + 8 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            stream->write((char*)&boxSize, 4); // 2nd asoc size
            stream->write((const char*)ASOC_BOX_ID, 4); // 2nd asoc type

            // Set the 2nd lbl box size, type, and data
            boxSize = 17 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            stream->write((char*)&boxSize, 4); // 2nd lbl size
            stream->write((const char*)LBL_BOX_ID, 4); // 2nd lbl type
            stream->write("gml.root-instance", 17); // 2nd lbl data

            // Set the xml box size, type, and data
            boxSize = xmlDataSize + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            stream->write((char*)&boxSize, 4); // xml size
            stream->write((const char*)XML_BOX_ID, 4); // xml type
            stream->write(xmlStr.str().data(), xmlDataSize); // xml data
            
            result = true;
         
            //cout << "wrote gml box!!!" << endl;
         }
      }

      // cleanup:
      delete gml;
      gml = 0;
   }
   
   return result;
   
} // End: ossimOpjCompressor::writeGmlBox

void ossimOpjCompressor::initOpjCodingParams( bool jp2,
                                              const ossimIpt& tileSize,
                                              const ossimIrect& imageRect )
{
   static const char MODULE[] = "ossimOpjCompressor::initOpjCodingParams";
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if ( m_params )
   {
      delete m_params;
   }

   m_params = new opj_cparameters_t();

   // Set encoding parameters to default values.
   opj_set_default_encoder_parameters( m_params );
   
   // Output format: 0: J2K, 1: JP2, 2: JPT   
   m_params->cod_format = jp2 ? 1 : 0;
      
   // std::string outfile = "test.jp2";
   // strncpy(m_params->outfile, outfile.data(), outfile.size());

   //---
   // Shall we do an Multi-component Transformation(MCT) ?
   // 0:no_mct;1:rgb->ycc;2:custom mct
   //---

   // Set the tiles size:
   m_params->cp_tx0 = 0;
   m_params->cp_ty0 = 0;
   m_params->cp_tdx = tileSize.x;
   m_params->cp_tdy = tileSize.y;
   m_params->tile_size_on = OPJ_TRUE;
   
   // No mct.
   m_params->tcp_mct = 0;

   //---
   // Number of resolutions:
   // This sets m_levels if not set and
   // m_params->numresolution.
   //---
   initLevels( imageRect );
      
   // Set the block size.  Note 64x64 is the current kakadu default.
   setCodeBlockSize( 64, 64 );
      
   //---
   // Set progression order to use. Note LRCP is the current opj default.
   // L=layer; R=resolution C=component; P=position
   // 
   // OPJ_LRCP, OPJ_RLCP, OPJ_RPCL, PCRL, CPRL */
   //---
   setProgressionOrder( OPJ_LRCP );
      
   // total pixels
   const ossim_float64 TP = imageRect.area();
      
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "quality type: " << getQualityTypeString() << endl;
   }
      
   //---
   // Rate is a ratio of desired bytes / total bytes.  So if you
   // want 4 bits per pixels it's total_pixels * 4 / 8 or
   // total_pixels * 4 * 0.125.
   //---
      
   // Allocation by rate/distortion.
   m_params->cp_disto_alloc = 1;
      
   switch (m_qualityType)
   {
      case ossimOpjCompressor::OPJ_NUMERICALLY_LOSSLESS:
      {
         // cout << "ossimOpjCompressor::OPJ_NUMERICALLY_LOSSLESS..." << endl;
         
         // W5X3 Kernel
         setReversibleFlag(true);

         // Tmp drb...
         m_params->tcp_numlayers = 1;
         m_params->tcp_rates[0] = 1;

#if 0
            
         m_params->tcp_numlayers = 20;
            
         m_params->tcp_rates[0] = std::ceil( TP * 0.03125 * 0.125 );
         m_params->tcp_rates[1] = std::ceil( TP * 0.0625 * 0.125 );
         m_params->tcp_rates[2] = std::ceil( TP * 0.125 * 0.125 );
         m_params->tcp_rates[3] = std::ceil( TP * 0.25 * 0.125 );
         m_params->tcp_rates[4] = std::ceil( TP * 0.5 * 0.125 );
         m_params->tcp_rates[5] = std::ceil( TP * 0.6 * 0.125 );
         m_params->tcp_rates[6] = std::ceil( TP * 0.7 * 0.125 );
         m_params->tcp_rates[7] = std::ceil( TP * 0.8 * 0.125 );
         m_params->tcp_rates[8] = std::ceil( TP * 0.9 * 0.125 );
         m_params->tcp_rates[9] = std::ceil( TP * 1.0 * 0.125 );
         m_params->tcp_rates[10] = std::ceil( TP * 1.1 * 0.125 );
         m_params->tcp_rates[11] = std::ceil( TP * 1.2 * 0.125 );
         m_params->tcp_rates[12] = std::ceil( TP * 1.3 * 0.125 );
         m_params->tcp_rates[13] = std::ceil( TP * 1.5 * 0.125 );
         m_params->tcp_rates[14] = std::ceil( TP * 1.7 * 0.125 );
         m_params->tcp_rates[15] = std::ceil( TP * 2.0 * 0.125 );
         m_params->tcp_rates[16] = std::ceil( TP * 2.3 * 0.125 );
         m_params->tcp_rates[17] = std::ceil( TP * 2.8 * 0.125 );
         m_params->tcp_rates[18] = std::ceil( TP * 3.5 * 0.125 );
            
         //---
         // Indicate that the final quality layer should include all
         // compressed bits.
         //---
         // m_params->tcp_rates[19] = OSSIM_DEFAULT_MAX_PIX_SINT32; // KDU_LONG_MAX;
         m_params->tcp_rates[19] = std::ceil( TP * 4.0 * 0.125 );

#endif
         
         break;
      }
      case ossimOpjCompressor::OPJ_VISUALLY_LOSSLESS:
      {
         // W9X7 kernel:
         setReversibleFlag(false);
            
         m_params->tcp_numlayers = 19;
            
         m_params->tcp_rates[0] = std::ceil( TP * 0.03125 * 0.125 );
         m_params->tcp_rates[1] = std::ceil( TP * 0.0625 * 0.125 );
         m_params->tcp_rates[2] = std::ceil( TP * 0.125 * 0.125 );
         m_params->tcp_rates[3] = std::ceil( TP * 0.25 * 0.125 );
         m_params->tcp_rates[4] = std::ceil( TP * 0.5 * 0.125 );
         m_params->tcp_rates[5] = std::ceil( TP * 0.6 * 0.125 );
         m_params->tcp_rates[6] = std::ceil( TP * 0.7 * 0.125 );
         m_params->tcp_rates[7] = std::ceil( TP * 0.8 * 0.125 );
         m_params->tcp_rates[8] = std::ceil( TP * 0.9 * 0.125 );
         m_params->tcp_rates[9] = std::ceil( TP * 1.0 * 0.125 );
         m_params->tcp_rates[10] = std::ceil( TP * 1.1 * 0.125 );
         m_params->tcp_rates[11] = std::ceil( TP * 1.2 * 0.125 );
         m_params->tcp_rates[12] = std::ceil( TP * 1.3 * 0.125 );
         m_params->tcp_rates[13] = std::ceil( TP * 1.5 * 0.125 );
         m_params->tcp_rates[14] = std::ceil( TP * 1.7 * 0.125 );
         m_params->tcp_rates[15] = std::ceil( TP * 2.0 * 0.125 );
         m_params->tcp_rates[16] = std::ceil( TP * 2.3 * 0.125 );
         m_params->tcp_rates[17] = std::ceil( TP * 2.8 * 0.125 );
         m_params->tcp_rates[18] = std::ceil( TP * 3.5 * 0.125 );
         break;
      }
      case ossimOpjCompressor::OPJ_LOSSY:
      {
         // W9X7 kernel:
         setReversibleFlag(false);
            
         m_params->tcp_numlayers = 10;
            
         m_params->tcp_rates[0] = std::ceil( TP * 0.03125 * 0.125 );
         m_params->tcp_rates[1] = std::ceil( TP * 0.0625 * 0.125 );
         m_params->tcp_rates[2] = std::ceil( TP * 0.125 * 0.125 );
         m_params->tcp_rates[3] = std::ceil( TP * 0.25 * 0.125 );
         m_params->tcp_rates[4] = std::ceil( TP * 0.5 * 0.125 );
         m_params->tcp_rates[5] = std::ceil( TP * 0.6 * 0.125 );
         m_params->tcp_rates[6] = std::ceil( TP * 0.7 * 0.125 );
         m_params->tcp_rates[7] = std::ceil( TP * 0.8 * 0.125 );
         m_params->tcp_rates[8] = std::ceil( TP * 0.9 * 0.125 );
         m_params->tcp_rates[9] = std::ceil( TP * 1.0 * 0.125 );
         break;
      }
         
      default:
      {
         m_params->tcp_numlayers = 1;
         m_params->tcp_rates[0] = 1.0;
            
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "unspecified quality type\n";
         }
      }
         
   } // matches: switch (m_qualityType)


   
   //---
   // Set reversible flag, note, this controls the kernel.
   // W5X3(default) if reversible = true, W9X7 if reversible is false.
   //---
   m_params->irreversible = !m_reversible;
   
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "m_reversible: " << m_reversible
         << "\nm_levels: " << m_levels << "\n";

      ossim::print( ossimNotify(ossimNotifyLevel_DEBUG), *m_params );
   }
      
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}

int ossimOpjCompressor::getNumberOfLayers() const
{
   return m_params->tcp_numlayers;
}

ossimString ossimOpjCompressor::getQualityTypeString() const
{
   return COMPRESSION_QUALITY[m_qualityType];
}

void ossimOpjCompressor::setQualityTypeString(const ossimString& s)
{
   ossimString type = s;
   type.downcase();
   
   if ( type == COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_UNKNOWN] )
   {
      setQualityType(ossimOpjCompressor::OPJ_UNKNOWN);
   }
   else if ( type == COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_USER_DEFINED] )
   {
      setQualityType(ossimOpjCompressor::OPJ_USER_DEFINED);
   }
   else if ( type == COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_NUMERICALLY_LOSSLESS] )
   {
      setQualityType(ossimOpjCompressor::OPJ_NUMERICALLY_LOSSLESS);
   }
   else if ( type == COMPRESSION_QUALITY[ossimOpjCompressor::OPJ_VISUALLY_LOSSLESS] )
   {
      setQualityType(ossimOpjCompressor::OPJ_VISUALLY_LOSSLESS);
   }
   else if (type == "lossy")
   {
      setQualityType(ossimOpjCompressor::OPJ_LOSSY);
   }
   else
   {
      if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimOpjCompressor::setQualityTypeString DEBUG"
            << "\nUnhandled quality type: " << type
            << std::endl;
      }
   }
}

void ossimOpjCompressor::initLevels( const ossimIrect& imageRect )
{
   //---
   // Number of wavelet decomposition levels, or stages.  May not exceed 32.
   // Default is 6 (r0 - r5)
   //---
   if ( m_levels == 0 )
   {
      m_levels = (ossim_int32)ossim::computeLevels(imageRect);
      if (m_levels == 0)
      {
         m_levels = 1; // Must have at least one.
      }
   }
   if ( (m_levels < 1) || (m_levels > 32) )
   {
      m_levels = 6;
   }

   m_params->numresolution = m_levels;
}

void ossimOpjCompressor::setCodeBlockSize( ossim_int32 xSize,
                                           ossim_int32 ySize )
{
   if ( m_params )
   {
      //---
      // Nominal code-block dimensions (must be powers of 2 no less than 4 and
      // no greater than 1024).
      // Default block dimensions are {64,64}
      //---
      m_params->cblockw_init = xSize;
      m_params->cblockh_init = ySize;
   }
}

void ossimOpjCompressor::setProgressionOrder( OPJ_PROG_ORDER progressionOrder )
{
   if ( m_params )
   {
      //---
      // Default progression order (may be overridden by Porder).
      // The four character identifiers have the following interpretation:
      // L=layer; R=resolution; C=component; P=position.
      // The first character in the identifier refers to the index which
      // progresses most slowly, while the last refers to the index which
      // progresses most quickly.  [Default is LRCP]
      // Enumerations:  (LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4)
      //---
      m_params->prog_order = progressionOrder;
   }
}
 
void ossimOpjCompressor::setTlmTileCount(ossim_uint32 /* tilesToWrite */)
{
   //---
   // Identifies the maximum number of tile-parts which will be written to the
   // code-stream for each tile.
   //
   // See Opj kdu_params.h "Macro = `ORGgen_tlm'" for more.
   //---
   // ossimString s = "ORGgen_tlm=1";
   // m_codestream.access_siz()->parse_string( s.c_str() );
}

opj_codec_t* ossimOpjCompressor::createOpjCodec( bool jp2 ) const
{
   opj_codec_t* codec = 0;
   if ( jp2 )
   {
      codec = opj_create_compress(OPJ_CODEC_JP2);
   }
   else
   {
      codec = opj_create_compress(OPJ_CODEC_J2K);
   }

   if ( codec )
   {
      // Catch events using our callbacks and give a local context.
      opj_set_info_handler   (codec, ossim::opj_info_callback,   00);
      opj_set_warning_handler(codec, ossim::opj_warning_callback,00);
      opj_set_error_handler  (codec, ossim::opj_error_callback,  00);
   }
   return codec;
   
} // End: opj_codec_t* createOpjCodec()

opj_stream_t* ossimOpjCompressor::createOpjStream( std::ostream* os ) const
{
   opj_stream_t* stream = 0;
   if ( os )
   {
      // stream = opj_stream_create_default_file_stream("test.jp2", OPJ_FALSE);
   
 
      // OPJ stream:
      // stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
      // stream = opj_stream_create(512, OPJ_FALSE);      
      // stream = opj_stream_create_default_file_stream("test.jp2", OPJ_FALSE); // false = output.
      stream = opj_stream_create(1048576, OPJ_FALSE);
      if ( stream )
      {
         // cout << "stream good..." << endl;
         
         // Set OPJ user stream:
         opj_stream_set_user_data(stream, os, ossim_opj_free_user_ostream_data);        
         
         // Set callbacks:
         // opj_stream_set_read_function(m_stream, ossim_opj_stream_read);
         // opj_stream_set_read_function(stream, ossim_opj_imagedata_read);
         opj_stream_set_write_function(stream, ossim_opj_ostream_write);
         opj_stream_set_skip_function(stream,  ossim_opj_ostream_skip);
         opj_stream_set_seek_function(stream,  ossim_opj_ostream_seek);
      }
   }
   return stream;

} // End: ossimOpjCompressor::createOpjStream()

opj_image_t* ossimOpjCompressor::createOpjImage( ossimScalarType scalar,
                                                 ossim_uint32 bands,
                                                 const ossimIrect& imageRect ) const
{
   opj_image_t* image = 0;

   // image component definitions:
   opj_image_cmptparm_t* imageParams = new opj_image_cmptparm_t[bands];	
   opj_image_cmptparm_t* imageParam = imageParams;
   for (ossim_uint32 i = 0; i < bands; ++i)
   {
      imageParam->dx = 1;
      imageParam->dy = 1;
      imageParam->h = imageRect.height();
      imageParam->w = imageRect.width();
      imageParam->sgnd = (OPJ_UINT32)ossim::isSigned(scalar);
      imageParam->prec = ossim::getActualBitsPerPixel(scalar);
      imageParam->bpp = ossim::getBitsPerPixel(scalar);
      imageParam->x0 = 0;
      imageParam->y0 = 0;
      ++imageParam;
   }
   
   OPJ_COLOR_SPACE clrspc;
   if ( bands == 3 )
   {
      clrspc = OPJ_CLRSPC_SRGB;
   }
   else //  if ( bands == 1 )
   {
      // Anything other than 3 band.  Not sure if this is correct way yet?(drb)
      clrspc = OPJ_CLRSPC_GRAY;
   }

   if ( clrspc != OPJ_CLRSPC_UNKNOWN )
   {
      image = opj_image_tile_create( bands, imageParams, clrspc );
      if ( image )
      {
         image->x0 = 0;
         image->y0 = 0;
         image->x1 = imageRect.width();
         image->y1 = imageRect.height();
         image->color_space = clrspc;
      }
   }

   delete [] imageParams;
   imageParams = 0;
   
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimOpjCompressor::createOpjImage DEBUG:\n";
      if ( image )
      {
         ossim::print( ossimNotify(ossimNotifyLevel_DEBUG), *image );
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimOpjCompressor::createOpjImage DEBUG image not created!"
            << std::endl;
      }
   }
   
   return image;
   
} // End: ossimOpjCompressor::createOpjImage( ... )

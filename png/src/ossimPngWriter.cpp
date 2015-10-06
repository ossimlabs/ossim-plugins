//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
// Contributor:  David A. Horner - http://dave.thehorners.com
//
// Description: OSSIM Portable Network Graphics (PNG) writer.
//
//----------------------------------------------------------------------------
// $Id: ossimPngWriter.cpp 22466 2013-10-24 18:23:51Z dburken $

#include "ossimPngWriter.h"
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <zlib.h>
#include <cstdlib>
#include <ctime>

RTTI_DEF1(ossimPngWriter,
	  "ossimPngWriter",
	  ossimImageFileWriter)

static const char DEFAULT_FILE_NAME[] = "output.png";
static const ossim_int32 DEFAULT_PNG_QUALITY = 75;

static const char COMPRESSION_LEVEL_KW[] = "compression_level";
static const char ADD_ALPHA_CHANNEL_KW[] = "add_alpha_channel";

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimPngWriter:debug" your_app_args
//---
static ossimTrace traceDebug("ossimPngWriter:debug");

//---
// For the "ident" program which will find all exanded $Id: ossimPngWriter.cpp 22466 2013-10-24 18:23:51Z dburken $ macros and print
// them.
//---
#if OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimPngWriter.cpp 22466 2013-10-24 18:23:51Z dburken $";
#endif

ossimPngWriter::ossimPngWriter()
   : ossimImageFileWriter(),
     theOutputStream(0),
     theOwnsStreamFlag(false),
     theCompressionLevel(Z_BEST_COMPRESSION),
     theInterlaceSupport(PNG_INTERLACE_NONE),
     theCompressionStratagy(Z_FILTERED),
     thePngFilter(PNG_FILTER_NONE),
     theGammaFlag(false),
     theGamma(0.0),
     theTimeFlag(true),
     theAlphaChannelFlag(false),     
     theBackgroundFlag(false),
     theBackgroundRed(0),
     theBackgroundGreen(0),
     theBackgroundBlue(0),
     theBackgroundGray(0),
     theTransparentFlag(false),
     theTransparentRed(0),
     theTransparentGreen(0),
     theTransparentBlue(0),
     theTransparentGray(0)

{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngWriter::ossimPngWriter entered" << std::endl;
#if OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif
   }

   // Since there is no internal geometry set the flag to write out one.
   setWriteExternalGeometryFlag(true);

   theOutputImageType = "ossim_png";
}

ossimPngWriter::~ossimPngWriter()
{
   // This will flush stream and delete it if we own it.
   close();
}

ossimString ossimPngWriter::getShortName() const
{
   return ossimString("ossim_png_writer");
}

ossimString ossimPngWriter::getLongName() const
{
   return ossimString("ossim png writer");
}

ossimString ossimPngWriter::getClassName() const
{
   return ossimString("ossimPngWriter");
}

bool ossimPngWriter::writeFile()
{
   if( !theInputConnection || (getErrorStatus() != ossimErrorCodes::OSSIM_OK) )
   {
      return false;
   }

   //---
   // Make sure we can open the file.  Note only the master process is used for
   // writing...
   //---
   if(theInputConnection->isMaster())
   {
      if (!isOpen())
      {
         open();
      }
   }

   return writeStream();
}

bool ossimPngWriter::writeStream()
{
   static const char MODULE[] = "ossimPngWriter::write";
   
   if (!theInputConnection) // Must have a sequencer...
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << MODULE << " DEBUG:\ntheInputConnection is NULL!" << endl;
      }
      return false;
   }
   
   //---
   // Make sure we have a stream.  Note only the master process is used for
   // writing...
   //---
   if(theInputConnection->isMaster())
   {
      if (!theOutputStream)         
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " ERROR:"
            << "\nNULL output stream!"
            << "\nReturning from method." << std::endl;

         return false;
      }
   }
   
   // make sure we have a region of interest
   if(theAreaOfInterest.hasNans())
   {
      theInputConnection->initialize();
      theAreaOfInterest = theInputConnection->getAreaOfInterest();
   }
   else
   {
      theInputConnection->setAreaOfInterest(theAreaOfInterest);
   }

   if(theAreaOfInterest.hasNans()) // Must have an area of interest...
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << MODULE << " ERROR:  Area of interest has nans!"
            << "Area of interest:  "
            << theAreaOfInterest
            << "\nReturning..." << endl;
      }

      return false;
   }

   // Get the number of bands.  Must be one or three for this writer.
   ossim_int32 bands = theInputConnection->getNumberOfOutputBands();
   if (bands != 1 && bands != 3)
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " Range Error:"
         << "\nInvalid number of input bands!  Must be one or three."
         << "\nInput bands = " << bands
         << "\nReturning from method." << endl;
      
      return false;
   }

   //---
   // PNG only supports unsigned 8 and 16 bit images, so scale if needed.
   // Note: This needs to be done on all processes.
   //---
   ossimScalarType inputScalar = theInputConnection->getOutputScalarType();
   if( ( (inputScalar != OSSIM_UINT8)  &&
         (inputScalar != OSSIM_UINT16) &&
         (inputScalar != OSSIM_USHORT11) ) ||
       ( (inputScalar != OSSIM_UINT8) && theScaleToEightBitFlag )
      )
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << " WARNING:"
            << "\nData is being scaled to 8 bit!"
            << "\nOriginal scalar type:  "
            << ossimScalarTypeLut::instance()->
            getEntryString(inputScalar).c_str()
            << std::endl;
      }

      //---
      // Attach a scalar remapper to the end of the input chain.  This will
      // need to be unattached and deleted at the end of this.
      //---
      ossimImageSource* inputSource = new ossimScalarRemapper;
      inputSource->connectMyInputTo(0, theInputConnection->getInput(0));
      theInputConnection->connectMyInputTo(0, inputSource);
      theInputConnection->initialize();
   }

   setProcessStatus(ossimProcessInterface::PROCESS_STATUS_EXECUTING);
   setPercentComplete(0.0);

   if(theInputConnection->isMaster())
   {
      ossimScalarType outputScalar = theInputConnection->getOutputScalarType();
      bool swapFlag = doSwap(outputScalar);
      ossim_int32 imageHeight
         = theAreaOfInterest.lr().y - theAreaOfInterest.ul().y + 1;
      ossim_int32 imageWidth
         = theAreaOfInterest.lr().x - theAreaOfInterest.ul().x + 1;
      ossim_int32 bands = theInputConnection->getNumberOfOutputBands();
      ossim_int32 tileHeight    = theInputConnection->getTileHeight();
      ossim_int32 bytesPerPixel = ossim::scalarSizeInBytes(outputScalar);
      ossim_int32 bytesPerRow   = bands*imageWidth*bytesPerPixel;
      ossim_int32 colorType     = getColorType(bands);

      if ( bands == 1 )
      {
         theTransparentGray = static_cast<ossim_uint16>( theInputConnection->getNullPixelValue() );
      }
      else
      if ( bands == 3 )
      {
         theTransparentRed  = static_cast<ossim_uint16>( theInputConnection->getNullPixelValue(0) );
         theTransparentGray = static_cast<ossim_uint16>( theInputConnection->getNullPixelValue(1) );
         theTransparentBlue = static_cast<ossim_uint16>( theInputConnection->getNullPixelValue(2) );
      }

      if ( theAlphaChannelFlag == true )
      {
         bands += 1; // add in the alpha channel
         bytesPerRow += imageWidth*bytesPerPixel;
      }

      // Allocate a buffer (tile) to hold a row of bip-format tiles.
      ossimRefPtr<ossimImageData> bipTile =
         new ossimImageData(0,
                            outputScalar,
                            bands,
                            imageWidth,
                            tileHeight);
      bipTile->initialize();

      //---
      // Begin of png related stuff.
      //---
      png_structp pp   = png_create_write_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
      png_infop   info = png_create_info_struct (pp);

      if ( setjmp( png_jmpbuf(pp) ) )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "Error writing image:  " << theFilename.c_str()
            << std::endl;
         return false;
      }

      //---
      // Pass the static write and flush to libpng to allow us to use our
      // stream instead of doing "png_init_io (pp, theOutputFilePtr);"
      //---
      png_set_write_fn( pp,
                        (png_voidp)theOutputStream,
                        (png_rw_ptr) &ossimPngWriter::pngWriteData,
                        (png_flush_ptr)&ossimPngWriter::pngFlush);

      // Set the compression level.
      png_set_compression_level(pp, theCompressionLevel);

      //---
      // Set the filtering.
      // Note that palette images should usually not be filtered.
      if (isLutEnabled())
      {
         png_set_filter(pp, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
         png_set_compression_strategy(pp, Z_DEFAULT_STRATEGY);
      }
      else
      {
         // leave default filter selection alone
         png_set_compression_strategy(pp, Z_FILTERED);
      }

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << MODULE << " DEBUG: "
            << "\nInput scalar:  "
            << ossimScalarTypeLut::instance()->
            getEntryString(inputScalar).c_str()
            << "\nOutput scalar: "
            <<  ossimScalarTypeLut::instance()->
            getEntryString(outputScalar).c_str()
            << "\nOutput Rect:  " << theAreaOfInterest
            << "\nSwap flag:  " << swapFlag
            << "\nbytesPerRow:         " << bytesPerRow
            << "\nimageWidth:          " << imageWidth
            << "\nimageHeight:         " << imageHeight
            << "\ngitDepth:            " << getBitDepth(outputScalar)
            << "\ngetColorType(bands): " << colorType
            << endl;
      }

      png_set_IHDR(pp,
                   info,
                   imageWidth,
                   imageHeight,
                   getBitDepth(outputScalar),
                   colorType,
                   theInterlaceSupport,
                   PNG_COMPRESSION_TYPE_DEFAULT,
                   PNG_FILTER_TYPE_DEFAULT);

      // Set the palette png_set_PLTE() in writeOptionalChunks()
      writeOptionalChunks(pp, info);

      png_write_info(pp, info);

      // png_set_packing(pp);

      // Write the image.
      if(theInterlaceSupport) // interlaced...
      {
         // Requires reading whole image into memory...
         ossimNotify(ossimNotifyLevel_WARN)
            << "Interlace support not implemented yet!"
            << std::endl;
         return false;
      }
      else // not interlaced...
      {
         theInputConnection->setAreaOfInterest(theAreaOfInterest);
         theInputConnection->setToStartOfSequence();
         ossim_uint32 maxY = theInputConnection->getNumberOfTilesVertical();

         //---
         // Loop through and grab a row of tiles and copy to the buffer.
         // Then write the buffer to the png file.
         // Get the next row of tiles ... until finished.
         //---

         // Row loop, in line direction...
         for (ossim_uint32 i=0; ( (i<maxY) && (!needsAborting()) ); ++i)
         {
            ossimIrect buf_rect = theAreaOfInterest;
            buf_rect.set_uly(theAreaOfInterest.ul().y+i*tileHeight);
            buf_rect.set_lry(buf_rect.ul().y + tileHeight - 1);
 
            bipTile->setImageRectangle(buf_rect);
            void* bipBuf = bipTile->getBuf();

            
            // Grab lines of data that span the entire width of the image.
            ossimRefPtr<ossimImageData> t = theInputConnection->getTile( buf_rect );
            if ( t.valid() )
            {
               if ( theAlphaChannelFlag )
               {
                  t->computeAlphaChannel();
                  t->unloadTileToBipAlpha(bipBuf, buf_rect, buf_rect);
               }
               else
               {
                  t->unloadTile( bipBuf, buf_rect, buf_rect, OSSIM_BIP);
               }

               // Copy the buffer to the png file.
               ossim_int32 lines_to_copy =
                  min( (buf_rect.lr().y-buf_rect.ul().y+1),
                       (theAreaOfInterest.lr().y-buf_rect.ul().y+1) );
               
               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << MODULE
                     << "buf_rect:       " << buf_rect
                     << "lines_to_copy:  " << lines_to_copy << endl;
               }

               if(!needsAborting())
               {
                  if(swapFlag)
                  {
                     ossimEndian endian;
                     endian.swap((ossim_uint16*)bipBuf,
                                 imageWidth*lines_to_copy*bands);
                  }

                  ossim_uint8* buf = (ossim_uint8*)bipBuf;
                  ossim_int32 buf_offset = 0;
                  for (ossim_int32 line=0; line<lines_to_copy; ++line)
                  {
                     png_bytep rowp = (png_bytep)&buf[buf_offset];
                     png_write_row(pp, rowp);
                     buf_offset += bytesPerRow;
                  }

               } // if ( !getProcessStatus() )

               double dPercentComplete = (100.0*(i+1)) / maxY;
               setPercentComplete( dPercentComplete );
 
            } // if ( t.valid() )

         } // End of loop through tiles in the y direction.

      } // Not interlace write block.

      png_write_end(pp, 0);
      png_destroy_write_struct(&pp, &info);

      close();

   } // End of if(theInputConnection->isMaster()) block.

   else   // slave process
   {
      theInputConnection->slaveProcessTiles();
   }

   if(needsAborting())
   {
      setProcessStatus(ossimProcessInterface::PROCESS_STATUS_ABORTED);
   }
   else
   {
      setProcessStatus(ossimProcessInterface::PROCESS_STATUS_NOT_EXECUTING);
   }

   return true;
}

void ossimPngWriter::pngWriteData(png_structp png_ptr,
                                  png_bytep   data,
                                  png_size_t  length)
{
   std::ostream* fp = (std::ostream*)png_get_io_ptr(png_ptr);

   fp->write((char*)data, length);
}

void ossimPngWriter::pngFlush( png_structp png_ptr )
{
   std::ostream* fp = (std::ostream*)png_get_io_ptr(png_ptr);
   fp->flush();
}

bool ossimPngWriter::saveState(ossimKeywordlist& kwl,
                               const char* prefix)const
{
   kwl.add( prefix,
            ADD_ALPHA_CHANNEL_KW,
            ossimString::toString( theAlphaChannelFlag ),
            true );

   return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimPngWriter::loadState(const ossimKeywordlist& kwl,
                               const char* prefix)
{
   const char* value;

   value = kwl.find(prefix, ADD_ALPHA_CHANNEL_KW);
   if(value)
   {
      setAlphaChannelFlag( ossimString(value).toBool() );
   }

   theOutputImageType = "png";

   return ossimImageFileWriter::loadState(kwl, prefix);
}

bool ossimPngWriter::isOpen() const
{
   if (theOutputStream)
   {
     return true;
   }
   return false;
}

bool ossimPngWriter::open()
{
   close();

   // Check for empty filenames.
   if (theFilename.empty())
   {
      return false;
   }

   // ossimOFStream* os = new ossimOFStream();
   std::ofstream* os = new std::ofstream();
   os->open(theFilename.c_str(), ios::out | ios::binary);
   if(os->is_open())
   {
      theOutputStream = os;
      theOwnsStreamFlag = true;
      return true;
   }
   delete os;
   os = 0;

   return false;
}

void ossimPngWriter::close()
{
   if (theOutputStream)      
   {
      theOutputStream->flush();

      if (theOwnsStreamFlag)
      {
         delete theOutputStream;
         theOutputStream = 0;
         theOwnsStreamFlag = false;
      }
   }
}

void ossimPngWriter::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back(ossimString("ossim_png"));
}

ossimString ossimPngWriter::getExtension() const
{
   return ossimString("png");
}

bool ossimPngWriter::hasImageType(const ossimString& imageType) const
{
   if ( (imageType == "ossim_png") || (imageType == "image/png") )
   {
      return true;
   }

   return false;
}

void ossimPngWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if (!property)
   {
      return;
   }

   if (property->getName() == COMPRESSION_LEVEL_KW)
   {
      setCompressionLevel(property->valueToString());
   }
   else
   if (property->getName() == ADD_ALPHA_CHANNEL_KW)
   {
      setAlphaChannelFlag( property->valueToString().toBool() );
   }
   else
   {
      ossimImageFileWriter::setProperty(property);
   }
}

ossimRefPtr<ossimProperty> ossimPngWriter::getProperty(const ossimString& name)const
{
   if (name == COMPRESSION_LEVEL_KW)
   {
      ossimStringProperty* stringProp =
         new ossimStringProperty(name,
                                 getCompressionLevel(),
                                 false); // editable flag
      stringProp->addConstraint(ossimString("z_no_compression"));
      stringProp->addConstraint(ossimString("z_best_speed"));
      stringProp->addConstraint(ossimString("z_best_compression"));
      stringProp->addConstraint(ossimString("z_default_compression"));
      return stringProp;
   }
   else if (name == ADD_ALPHA_CHANNEL_KW)
   {
      return new ossimBooleanProperty(ADD_ALPHA_CHANNEL_KW,
                                      theAlphaChannelFlag);
   }
   return ossimImageFileWriter::getProperty(name);
}

void ossimPngWriter::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(ossimString(COMPRESSION_LEVEL_KW));
   propertyNames.push_back(ossimString(ADD_ALPHA_CHANNEL_KW));
   ossimImageFileWriter::getPropertyNames(propertyNames);
}

bool ossimPngWriter::doSwap(ossimScalarType outputScalar) const
{
   bool result = false;
   if (outputScalar != OSSIM_UINT8)
   {
      if(ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
      {
         result = true;
      }
   }
   return result;
}

bool ossimPngWriter::isLutEnabled()const
{
   // Temp drb...
   return false;
}

void ossimPngWriter::writeOptionalChunks(png_structp pp, png_infop info)
{
   //---
   // NOTE:
   // Per PNG spec
   // Chunks with no explicit restrictions (``anywhere'') are nonetheless
   // implicitly constrained to come after the PNG signature and IHDR
   // chunk, before the IEND chunk, and not to fall between multiple
   // IDAT chunks.
   //---

   //---
   // Write gamma chunk (gAMMA)
   // Location:   Before first IDAT and PLTE
   // Multiple:   no
   //---
   writeGammaChunk(pp, info);

   //---
   // Write significant bits (sBIT)
   // Location: before PLTE and first IDAT
   // Multiple: no
   //---
   writeSignificantBits(pp, info);

   // Set the palette png_set_PLTE()

   //---
   // Write transparent color chunk (tRNS)
   // Location: After PLTE, before first IDAT.
   // Multiple: 
   //---
   writeTransparentColorChunk(pp, info);

   //---
   // Write background color chunk (bKGD)
   // Location: After PLTE, before first IDAT.
   // Multiple: no
   //---
   writeBackgroundColorChunk(pp, info);
   
   //---
   // Write time stamp chunk (tIME).
   // Location: anywhere
   // Multiple: no
   // 
   //---
   writeTimeStampChunk(pp, info);
   
   //---
   // Write any text (tEXt, zTXt).
   // Location: anywhere
   // Multiple: yes
   //---
   writeLatinTextChunk(pp, info);
   
   //---
   // Write any text (iTXt).
   // Location: anywhere
   // Multiple: yes
   //---
   writeInternationalTextChunk(pp, info);
   
   //---
   // Write histogram (hIST)
   // Location: after PLTE, before first IDAT
   // Multiple: no
   //---
   writeHistogram(pp, info);

   //---
   // Write suggested palette (sPLT)
   // Location: before first IDAT
   // Multiple: yes
   //---
   writeSuggestedPalette(pp, info);
   
   //---
   // Write physical pixel dimensions (pHYs)
   // Location: before first IDAT
   // Multiple: no
   //---
   writePhysicalPixelDimensions(pp, info);
   
   //---
   // Write physical scale (sCAL)
   // Location: before first IDAT
   // Multiple: no
   //---
   writePhysicalScale(pp, info);
   
   //---
   // Write image offset (oFFs)
   // Location: before first IDAT
   // Multiple: no
   //---
   writeImageOffset(pp, info);
   
   //---
   // Write pixel calibration (pCAL)
   // Location: after PLTE, before first IDAT
   // Multiple: no
   //---
   writePixelCalibration(pp, info);
   
   //---
   // Write fractal parameters (fRAc)
   // Location: anywhere
   // Multiple: yes
   //---
   writeFractalParameters(pp, info);
}

void ossimPngWriter::writeTransparentColorChunk(png_structp pp, png_infop info)
{
   if (theTransparentFlag)
   {
      png_color_16 transparent;
      transparent.red   = (png_uint_16)theTransparentRed;
      transparent.green = (png_uint_16)theTransparentGreen;
      transparent.blue  = (png_uint_16)theTransparentBlue;
      transparent.gray  = (png_uint_16)theTransparentGray;
      png_set_tRNS( pp, info, NULL, 0, &transparent );
   }
}

void ossimPngWriter::writeBackgroundColorChunk(png_structp pp, png_infop info)
{
   if(theBackgroundFlag)
   {
      png_color_16 background;
      background.red   = theBackgroundRed;
      background.green = theBackgroundGreen;
      background.blue  = theBackgroundBlue;
      background.gray  = theBackgroundGray;
      png_set_bKGD( pp, info, &background );
   }
}

void ossimPngWriter::writeGammaChunk(png_structp pp, png_infop info)
{
   if(theGammaFlag)
   {
      png_set_gAMA(pp, info, theGamma);
   }
}

void ossimPngWriter::writeTimeStampChunk(png_structp pp,
                                         png_infop info)
{
   if(theTimeFlag)
   {
      png_time  modtime;
      time_t t;
      time(&t);
      png_convert_from_time_t(&modtime, t);
      png_set_tIME(pp, info, &modtime); 
   }
}

void ossimPngWriter::writeLatinTextChunk(png_structp /* pp */,
                                         png_infop /* info */)
{}

void ossimPngWriter::writeInternationalTextChunk(png_structp /* pp */,
                                                 png_infop /* info */)
{}

void ossimPngWriter::writeHistogram(png_structp /* pp */,
                                    png_infop /* info */)
{}

void ossimPngWriter::writeSignificantBits(png_structp pp,
                                          png_infop info)
{
   // Must be called before PLTE and first IDAT.  No multiple sBIT chunks.
   if (theInputConnection->getOutputScalarType() != OSSIM_UINT8)
   {
      ossim_float64 max = theInputConnection->getMaxPixelValue();
      if (max <= 65535)
      {
         ossim_uint16 max16 = static_cast<ossim_uint16>(max);
         png_byte bits      = 16;
         ossim_uint16 s     = 0xffff;
         while ( (s != 0x0001) && (s >= max16) )
         {
            s = s >> 1;
            if (s < max) break;
            --bits;
         }
         
         // destroyed by png_destroy_write_struct ??? (drb)
         png_color_8* sig_bit = new png_color_8; 
         sig_bit->red   = bits;
         sig_bit->green = bits;
         sig_bit->blue  = bits;
         sig_bit->gray  = bits;
         sig_bit->alpha = bits;
         png_set_sBIT(pp, info, sig_bit);
      }
   }
}

void ossimPngWriter::writeSuggestedPalette(png_structp /* pp */,
                                           png_infop /* info */)
{}

void ossimPngWriter::writePhysicalPixelDimensions(png_structp /* pp */,
                                                  png_infop /* info */)
{}

void ossimPngWriter::writePhysicalScale(png_structp /* pp */,
                                        png_infop /* info */)
{}

void ossimPngWriter::writeImageOffset(png_structp /* pp */,
                                      png_infop /* info */)
{}

void ossimPngWriter::writePixelCalibration(png_structp /* pp */,
                                           png_infop /* info */)
{}

void ossimPngWriter::writeFractalParameters(png_structp /* pp */,
                                            png_infop /* info */)
{}

ossim_int32 ossimPngWriter::getColorType(ossim_int32 bands) const
{
   //---
   // 
   //---
   ossim_int32 colorType = PNG_COLOR_TYPE_RGB;

   if(theAlphaChannelFlag)
   {
      if(bands == 1)
      {
         colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
      }
      else
      {
         colorType = PNG_COLOR_TYPE_RGB_ALPHA;
      }
   }
   else
   {
      if (bands == 1)
      {
         colorType = PNG_COLOR_TYPE_GRAY;
      }
   }

   return colorType;
}

ossim_int32 ossimPngWriter::getBitDepth(ossimScalarType outputScalar) const
{
   // Can be 1, 2, 4, 8, or 16 bits/channel (from IHDR)
   ossim_int32 bitDepth = 0;
   
   switch(outputScalar)
   {
      case OSSIM_UINT8:
         bitDepth = 8;
         break;
      case OSSIM_SINT16:
      case OSSIM_UINT16:
      case OSSIM_USHORT11:
         bitDepth = 16;
         break;
      default:
         break;
   }

   return bitDepth;
}

ossimString ossimPngWriter::getCompressionLevel() const
{
   ossimString result = ossimString("z_default_compression");
   
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
   return result;
}

bool ossimPngWriter::setCompressionLevel(const ossimString& level)
{
   bool status = true;

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
         << "\nossimPngWriter::setCompressionLevel DEBUG"
         << "passed in level:  " << level.c_str()
         << "writer level: " << getCompressionLevel().c_str()
         << endl;
   }

   return status;
}

bool ossimPngWriter::getAlphaChannelFlag( void ) const
{
   return theAlphaChannelFlag;
}

void ossimPngWriter::setAlphaChannelFlag( bool flag )
{
   theAlphaChannelFlag = flag;
}

bool ossimPngWriter::setOutputStream(std::ostream& stream)
{
   if (theOwnsStreamFlag && theOutputStream)
   {
      delete theOutputStream;
   }
   theOutputStream = &stream;
   theOwnsStreamFlag = false;
   return true;
}

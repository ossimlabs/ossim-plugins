//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description:  OSSIM wrapper for building nitf, j2k compressed overviews
// using kakadu from an ossim image source.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduNitfOverviewBuilder.cpp 22363 2013-08-07 20:28:54Z dburken $

#include "ossimKakaduNitfOverviewBuilder.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduCompressor.h"
#include "ossimKakaduKeywords.h"

#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimErrorCodes.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimStdOutProgress.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>

#include <ossim/parallel/ossimMpi.h>
#include <ossim/parallel/ossimMpiMasterOverviewSequencer.h>
#include <ossim/parallel/ossimMpiSlaveOverviewSequencer.h>

#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>
#include <ossim/support_data/ossimNitfCommon.h>
#include <ossim/support_data/ossimNitfFileHeader.h>
#include <ossim/support_data/ossimNitfFileHeaderV2_1.h>
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <ossim/support_data/ossimNitfImageHeaderV2_1.h>
#include <ossim/imaging/ossimBitMaskTileSource.h>

#include <fstream>
#include <sstream>

static const ossimString OVERVIEW_TYPE = "ossim_kakadu_nitf_j2k";
static const ossimString OVERVIEW_TYPE_ALIAS = "j2k";

static const ossimIpt DEFAULT_TILE_SIZE(1024, 1024);

RTTI_DEF1(ossimKakaduNitfOverviewBuilder,
          "ossimKakaduNitfOverviewBuilder",
          ossimOverviewBuilderBase);

static const ossimTrace traceDebug(
   ossimString("ossimKakaduNitfOverviewBuilder:debug"));

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimKakaduNitfOverviewBuilder.cpp 22363 2013-08-07 20:28:54Z dburken $";
#endif

ossimKakaduNitfOverviewBuilder::ossimKakaduNitfOverviewBuilder()
   :
   ossimOverviewBuilderBase(),
   m_outputFile(ossimFilename::NIL),
   m_outputFileTmp(ossimFilename::NIL),
   m_compressor(new ossimKakaduCompressor())
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduNitfOverviewBuilder::ossimKakaduNitfOverviewBuilder"
         << " DEBUG:\n";
#ifdef OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << "\n";
#endif
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "overview stop dimension: " << m_overviewStopDimension
         << std::endl;
   }
}

ossimKakaduNitfOverviewBuilder::~ossimKakaduNitfOverviewBuilder()
{
   m_imageHandler = 0;

   if (m_compressor)
   {
      delete m_compressor;
      m_compressor = 0;
   }
}

void ossimKakaduNitfOverviewBuilder::setOutputFile(const ossimFilename& file)
{
   m_outputFile = file;
}

ossimFilename ossimKakaduNitfOverviewBuilder::getOutputFile() const
{
   ossimFilename result = m_outputFile;
   if (m_outputFile == ossimFilename::NIL)
   {
      if ( m_imageHandler.valid() )
      {
         bool usePrefix = (m_imageHandler->getNumberOfEntries()>1?true:false);
         result = m_imageHandler->
            getFilenameWithThisExtension(ossimString("ovr"), usePrefix);
      }
   }
   return result;
}

bool ossimKakaduNitfOverviewBuilder::setOverviewType(const ossimString& type)
{
   bool result = false;

   //---
   // Only have one type right now.  These are the same:
   // "ossim_kakadu_nitf_j2k" or "j2k"
   //---
   if ( (type == OVERVIEW_TYPE) || (type == OVERVIEW_TYPE_ALIAS) )
   {
      result = true;
   }

   return result;
}

ossimString ossimKakaduNitfOverviewBuilder::getOverviewType() const
{
   return OVERVIEW_TYPE;   
}

void ossimKakaduNitfOverviewBuilder::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(OVERVIEW_TYPE);
   typeList.push_back(OVERVIEW_TYPE_ALIAS);   
}

bool ossimKakaduNitfOverviewBuilder::execute()
{
   static const char MODULE[] = "ossimKakaduNitfOverviewBuilder::execute";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   
   bool result = false;

   if ( m_imageHandler.valid() ) // Make sure there is a source.
   {
      // Get the output filename.
      ossimFilename outputFile = getOutputFile();

      if (outputFile != m_imageHandler->getFilename())
      {
         //---
         // Add .tmp in case process gets aborted to avoid leaving bad .ovr
         // file.
         //---
         ossimFilename outputFileTemp = outputFile + ".tmp";
         
         // Required number of levels needed including r0.
         ossim_uint32 requiedResLevels =
            getRequiredResLevels(m_imageHandler.get());
         
         // Zero based starting resLevel.
         ossim_uint32 startingResLevel =
            m_imageHandler->getNumberOfDecimationLevels();
         
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << MODULE
               << "\nCurrent number of reduced res sets: "
               << m_imageHandler->getNumberOfDecimationLevels()
               << "\nNumber of required reduced res sets:  "
               << requiedResLevels
               << "\nStarting reduced res set:    " << startingResLevel
               << "\nHistogram mode: " << getHistogramMode()
               << std::endl;
         }

         if ( startingResLevel < requiedResLevels )
         {
            //---
            // If image handler is band selector, start with all bands.
            // Some sources, e.g. ossimEnviTileSource can pick up default
            // bands and filter out all other bands.
            //---
            m_imageHandler->setOutputToInputBandList();  
            
            // If alpha bit mask generation was requested, then need to instantiate the mask writer
            // object.  This is simply a "transparent" tile source placed after to the right of the
            // image handler that scans the pixels being pulled and accumulates alpha bit mask for
            // writing at the end:
            if (m_bitMaskSpec.getSize() > 0)
            {
               m_maskWriter = new ossimBitMaskWriter();
               m_maskWriter->loadState(m_bitMaskSpec);
               m_maskWriter->setStartingResLevel(1);
               ossimRefPtr<ossimBitMaskTileSource> bmts = new ossimBitMaskTileSource;
               bmts->setAssociatedMaskWriter(m_maskWriter.get());
               m_maskFilter->connectMyInputTo(m_imageHandler.get());
               m_maskFilter->setMaskSource((ossimImageSource*)bmts.get());
            }

            //---
            // Set up the sequencer.  This will be one of three depending
            // on if we're running mpi and if we are a master process or
            // a slave process.
            //---
            ossimRefPtr<ossimOverviewSequencer> sequencer;
            
            if(ossimMpi::instance()->getNumberOfProcessors() > 1)
            {
               if ( ossimMpi::instance()->getRank() == 0 )
               {
                  sequencer = new ossimMpiMasterOverviewSequencer();
               }
               else
               {
                  sequencer = new ossimMpiSlaveOverviewSequencer();
               }
            }
            else
            {
               sequencer = new ossimOverviewSequencer();
            }

            if( ossimMpi::instance()->getNumberOfProcessors() == 1)
            {
               //---
               // Pass the histogram mode to the sequencer.
               // Can't do with mpi/multi-process.
               //---
               sequencer->setHistogramMode( getHistogramMode() );

               if ( getScanForMinMaxNull() == true )
               {
                  sequencer->setScanForMinMaxNull(true);
               }
               else if ( getScanForMinMax() == true )
               {
                  sequencer->setScanForMinMax(true);
               }
            }
            
            sequencer->setImageHandler( m_imageHandler.get() );
            
            ossim_uint32 sourceResLevel = 0;
            if ( startingResLevel > 1)
            {
               sourceResLevel = startingResLevel - 1;
            }
         
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << MODULE
                  << "\nSource starting reduced res set: " << sourceResLevel
                  << std::endl;
            }
            
            sequencer->setSourceLevel(sourceResLevel);

            // Tmp hard coded to BOX:
            sequencer->setResampleType(
                  ossimFilterResampler::ossimFilterResampler_BOX);

            sequencer->setTileSize( DEFAULT_TILE_SIZE );

            sequencer->initialize();
            
            //---
            // If we are a slave process start the resampling of tiles.
            //---
            if (ossimMpi::instance()->getRank() != 0 )
            {
               sequencer->slaveProcessTiles();
               return true; // End of slave process.
            }

            ossim_uint32 outputTilesWide =
               sequencer->getNumberOfTilesHorizontal();
            ossim_uint32 outputTilesHigh =
               sequencer->getNumberOfTilesVertical();
            ossim_uint32 numberOfTiles =
               sequencer->getNumberOfTiles();
            ossim_uint32 tileNumber = 0;
            ossimIrect imageRect;
            sequencer->getOutputImageRectangle(imageRect);
            
            //---
            // From here on out master process only.
            //---
            ossimNitfFileHeaderV2_1* fHdr = 0;
            ossimNitfImageHeaderV2_1* iHdr = 0;
            ossimStdOutProgress* progressListener = 0;
            std::ofstream* os = 0;
            std::streampos endOfFileHdrPos;
            std::streampos endOfImgHdrPos;
            std::streampos endOfFilePos;
            ossimNitfImageInfoRecordV2_1 imageInfoRecord;
            
            // Open the output file.
            os = new std::ofstream;
            os->open(outputFileTemp.chars(), std::ios_base::out|std::ios_base::binary);
            if ( os->good() )
            {
               result = true; // Assuming we are good from this point.
               
               // Get an info dump to a keyword list.
               ossimKeywordlist kwl;
               std::shared_ptr<ossimInfoBase> info =
                  ossimInfoFactoryRegistry::instance()->
                  create(m_imageHandler->getFilename());
               if ( info )
               {
                  info->getKeywordlist(kwl);
                  info.reset();
               }
               
               fHdr = new ossimNitfFileHeaderV2_1();
               
               fHdr->addImageInfoRecord(imageInfoRecord);
               
               fHdr->setDate(ossimDate());
               fHdr->setTitle(ossimString("ossim_nitf_j2k_overview"));
               
               const char* lookup = kwl.find("nitf.FSCLAS");
               if (lookup)
               {
                  fHdr->setSecurityClassificationSys(ossimString(lookup));
               }
               fHdr->writeStream(*os);
               endOfFileHdrPos = os->tellp();
               
               iHdr = new ossimNitfImageHeaderV2_1();

               // Set the compression type:
               iHdr->setCompression(ossimString("C8"));

               // Set the Image Magnification (IMAG) field.
               setImagField(iHdr, startingResLevel);

               ossimScalarType scalarType = m_imageHandler->getOutputScalarType();

               // Set the pixel type (PVTYPE field).
               iHdr->setPixelType(
                  ossimNitfCommon::getNitfPixelType(scalarType));

               // Set the actual bits per pixel (ABPP field).
               iHdr->setActualBitsPerPixel(
                  ossim::getActualBitsPerPixel(scalarType));
               iHdr->setBitsPerPixel(ossim::getBitsPerPixel(scalarType));
               
               const ossim_uint64 BANDS =
                  m_imageHandler->getNumberOfOutputBands();

               iHdr->setNumberOfBands(BANDS);
               iHdr->setImageMode('B');// blocked
               
               if((BANDS == 3)&&
                  (scalarType == OSSIM_UCHAR))
               {
                  iHdr->setRepresentation("RGB");
                  iHdr->setCategory("VIS");
               }
               else if(BANDS == 1)
               {
                  iHdr->setRepresentation("MONO");
                  iHdr->setCategory("MS");
               }
               else
               {
                  iHdr->setRepresentation("MULTI");
                  iHdr->setCategory("MS");
               }
               
               iHdr->setBlocksPerRow(outputTilesWide);
               iHdr->setBlocksPerCol(outputTilesHigh);
               iHdr->setNumberOfPixelsPerBlockRow(DEFAULT_TILE_SIZE.y);
               iHdr->setNumberOfPixelsPerBlockCol(DEFAULT_TILE_SIZE.x);
               iHdr->setNumberOfRows(imageRect.height());
               iHdr->setNumberOfCols(imageRect.width());
               
               ossimNitfImageBandV2_1 bandInfo;
               for(ossim_uint32 band = 0; band < BANDS; ++band)
               {
                  std::ostringstream out;
                  
                  out << std::setfill('0')
                      << std::setw(2)
                      << band;
                  
                  bandInfo.setBandRepresentation(out.str().c_str());
                  iHdr->setBandInfo(band, bandInfo);
               }
               
               iHdr->writeStream(*os);
               endOfImgHdrPos = os->tellp();
               
               //---
               // Check for a listeners.  If the list is empty, add a
               // standard out listener so that command line apps like
               // img2rr will get some progress.
               //---
               ossimStdOutProgress* progressListener = 0;
               if (theListenerList.empty())
               {
                  progressListener = new ossimStdOutProgress(0, true);
                  addListener(progressListener);
               }
               setProcessStatus(
                  ossimProcessInterface::PROCESS_STATUS_EXECUTING);
               setPercentComplete(0.0);
               
               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << MODULE << " DEBUG:"
                     << "\noutputTilesWide:  " << outputTilesWide
                     << "\noutputTilesHigh:  " << outputTilesHigh
                     << "\nnumberOfTiles:    " << numberOfTiles
                     << "\nimageRect: " << imageRect
                     << std::endl;
               }

               if (m_compressor->getAlphaChannelFlag())
               {
                  //--- 
                  // Someone can set this through the generic setProperty
                  // interface. Unset, currently only supported in jp2 writer.
                  // Could be used here but I think we would have to update the
                  // nitf tags.
                  //---
                  m_compressor->setAlphaChannelFlag(false);
               }

               if ( ossim::getActualBitsPerPixel(scalarType) > 31 )
               {
                  m_compressor->setQualityType(ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS);
               }

               try
               {
                  // Make a compressor
                  m_compressor->create(os,
                                       scalarType,
                                       BANDS,
                                       imageRect,
                                       DEFAULT_TILE_SIZE,
                                       numberOfTiles,
                                       false);
                  
                  ossimNotify(ossimNotifyLevel_INFO)
                     << "Generating " << (m_compressor->getLevels()+1)
                     << " levels..." << endl;
               }
               catch (const ossimException& e)
               {
                  setErrorStatus();
                  ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
                  result = false;
               }

               if ( result )
               {
                  // Tile loop in the line direction.
                  for(ossim_uint32 y = 0; y < outputTilesHigh; ++y)
                  {
                     // Tile loop in the sample (width) direction.
                     for(ossim_uint32 x = 0; x < outputTilesWide; ++x)
                     {
                        // Grab the resampled tile.
                        ossimRefPtr<ossimImageData> t = sequencer->getNextTile();

                        // Check for errors reading tile:
                        if ( sequencer->hasError() )
                        {
                           setErrorStatus();
                           ossimNotify(ossimNotifyLevel_WARN)
                              << MODULE << " ERROR: reading tile:  "
                              << tileNumber << std::endl;
                           result = false;
                        }

                        if ( result && t.valid() )
                        {
                           //---
                           // If masking was enabled, pass the tile onto that object for
                           // processing:
                           //---
                           if ( m_maskWriter.valid())
                           {
                              m_maskWriter->generateMask(t, 0);
                           }
                           
                           if ( t->getDataObjectStatus() != OSSIM_NULL )
                           {
                              if ( ! m_compressor->writeTile( *(t.get()) ) )
                              {
                                 setErrorStatus();
                                 ossimNotify(ossimNotifyLevel_WARN)
                                    << MODULE << " ERROR:\nWriting tile:  "
                                    << tileNumber << std::endl;
                                 result = false;
                              }
                           }
                           else
                           {
                              setErrorStatus();
                              ossimNotify(ossimNotifyLevel_WARN)
                                 << MODULE << " ERROR:\nNull tile returned:  " << tileNumber
                                 << std::endl;
                              result = false;
                           }
                        }
                        
                        if ( !result )
                        {
                           // Bust out of sample loop.
                           break;
                        }
                        
                        // Increment tile number for percent complete.
                        ++tileNumber;
                        
                     } // End of tile loop in the sample (width) direction.

                     if ( !result )
                     {
                        // Bust out of line loop.
                        break;
                     }
                     
                     if ( needsAborting() )
                     {
                        setPercentComplete(100.0);
                        break;
                     }
                     else
                     {
                        double tile = tileNumber;
                        double numTiles = numberOfTiles;
                        setPercentComplete(tile / numTiles * 100.0);
                     }
                     
                  } // End of tile loop in the line (height) direction.

                  if ( result )
                  {
                     m_compressor->finish();
                     
                     // Get the file length.
                     endOfFilePos = os->tellp();
                     
                     //---
                     // Seek back to set some things that were not know until now and
                     // rewrite the nitf file and image header.
                     //---
                     os->seekp(0, std::ios_base::beg);
                     
                     // Set the file length.
                     std::streamoff length = endOfFilePos;
                     fHdr->setFileLength(static_cast<ossim_uint64>(length));
                     
                     // Set the file header length.
                     length = endOfFileHdrPos;
                     fHdr->setHeaderLength(static_cast<ossim_uint64>(length));            
                     // Set the image sub header length.
                     length = endOfImgHdrPos - endOfFileHdrPos;
                     
                     imageInfoRecord.setSubheaderLength(
                        static_cast<ossim_uint64>(length));
                     
                     // Set the image length.
                     length = endOfFilePos - endOfImgHdrPos;
                     imageInfoRecord.setImageLength(
                        static_cast<ossim_uint64>(length));
                     
                     fHdr->replaceImageInfoRecord(0, imageInfoRecord);
                     
                     // Rewrite the header.
                     fHdr->writeStream(*os);
                     
                     // Set the compression rate now that the image size is known.
                     ossimString comrat = ossimNitfCommon::getCompressionRate(
                        imageRect,
                        BANDS,
                        scalarType,
                        static_cast<ossim_uint64>(length));
                     iHdr->setCompressionRateCode(comrat);
                     
                     // Rewrite the image header.
                     iHdr->writeStream(*os);
                     
                     if (progressListener)
                     {
                        removeListener(progressListener);
                        delete progressListener;
                        progressListener = 0;
                     }
                     
                     if ( ossimMpi::instance()->getNumberOfProcessors() == 1 )
                     {
                        if ( getHistogramMode() != OSSIM_HISTO_MODE_UNKNOWN )
                        {
                           // Write the histogram.
                           ossimFilename histoFilename = getOutputFile();
                           histoFilename.setExtension("his");
                           sequencer->writeHistogram(histoFilename);
                        }
                        
                        if ( ( getScanForMinMaxNull() == true ) || ( getScanForMinMax() == true ) )
                        {
                           // Write the omd file:
                           ossimFilename file = m_outputFile;
                           file = file.setExtension("omd");
                           sequencer->writeOmdFile(file);
                        }
                     }
                     
                     // If masking was enabled, then only R0 was processed, need to process
                     // remaining levels:
                     if (m_maskWriter.valid())
                     {
                        ossim_uint32 num_rlevels = m_compressor->getLevels() + 1;
                        m_maskWriter->buildOverviews(num_rlevels);
                        m_maskFilter->disconnectMyInput(0);
                        m_maskWriter->disconnectAllInputs();
                        ossimNotify(ossimNotifyLevel_INFO)
                           << MODULE << "Writing alpha bit mask file..." << std::endl;
                        m_maskWriter->close();
                     }
                     
                  } // if ( result ) *** after end of tile loop ***

               } // End: if ( result ) from m_compressor->create

            } // End: if (os->good())
            
            if (fHdr)
            {
               delete fHdr;
               fHdr = 0;
            }
            
            if (iHdr)
            {
               delete iHdr;
               iHdr = 0;
            }

            if (progressListener)
            {
               delete progressListener;
               progressListener = 0;
            }
            
            if (os->is_open())
            {
               os->close();
               if ( result )
               {
                  outputFileTemp.rename(outputFile);
               }
               else
               {
                  ossimFilename::remove( outputFileTemp );
               }
            }
            delete os;
            os = 0;

         }
         else
         {
            ossimNotify(ossimNotifyLevel_INFO)<< MODULE << " NOTICE:"
               <<"  Image has required reduced resolution data sets.\n"<< std::endl;
         }
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "Source image file and overview file cannot be the same!"
            << std::endl;
      }
      
   } // matches: if (m_imageHandler)

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }

   finalize();  // Reset band list if a band selector.

   return result;

} // End: ossimKakaduNitfOverviewBuilder::execute()

void ossimKakaduNitfOverviewBuilder::setProperty(
   ossimRefPtr<ossimProperty> property)
{
   if ( property.valid() )
   {
      m_compressor->setProperty(property);
   }
}

void ossimKakaduNitfOverviewBuilder::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   m_compressor->getPropertyNames(propertyNames);
}

std::ostream& ossimKakaduNitfOverviewBuilder::print(std::ostream& out) const
{
   out << "ossimKakaduNitfOverviewBuilder::print"
       << std::endl;
   return out;
}

ossimObject* ossimKakaduNitfOverviewBuilder::getObject()
{
   return this;
}

const ossimObject* ossimKakaduNitfOverviewBuilder::getObject() const
{
   return this;
}

bool ossimKakaduNitfOverviewBuilder::canConnectMyInputTo(
   ossim_int32 index,
   const ossimConnectableObject* obj) const
{
   if ( (index == 0) &&
        PTR_CAST(ossimImageHandler, obj) )
   {
      return true;
   }

   return false;
}

void ossimKakaduNitfOverviewBuilder::setImagField(
   ossimNitfImageHeaderV2_1* hdr, ossim_uint32 startingResLevel) const
{
   //---
   // This assumes res level 0 is 1.0 magnification.
   //---
   if (hdr)
   {
      ossimString imagString;
      
      if (startingResLevel == 0)
      {
         imagString = "1.0  ";
      }
      else
      {
         //---
         // Assumes power of 2 decimation.
         //---
         ossim_uint32 decimation = 1<<startingResLevel;
                     
         //---
         // Using slash '/' format.  "/2" = 1/2, /  
         imagString = "/";
         imagString += ossimString::toString(decimation);
         if (imagString.size() <= 4)
         {
            if (imagString.size() == 2)
            {
               imagString += "  "; // two spaces
            }
            else if (imagString.size() == 3)
            {
               imagString += " "; // one space
            }
         }
      }
      if (imagString.size())
      {
         hdr->setImageMagnification(imagString);
      }
   }
}

//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file.
//
// Author:  David Burken
//
// Description:  NITF reader for j2k images using OpenJPEG library.
//
// $Id$
//----------------------------------------------------------------------------


#include <openjpeg.h>

#include <ossimOpjNitfReader.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/imaging/ossimTiffTileSource.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/support_data/ossimNitfImageHeader.h>

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif

static ossimTrace traceDebug("ossimOpjNitfReader:debug");


static const ossim_uint16 SOC_MARKER = 0xff4f; // start of codestream
static const ossim_uint16 SIZ_MARKER = 0xff51; // start of tile-part
static const ossim_uint16 SOT_MARKER = 0xff90; // start of tile-part

RTTI_DEF1_INST(ossimOpjNitfReader,
               "ossimOpjNitfReader",
               ossimNitfTileSource)



ossimOpjNitfReader::ossimOpjNitfReader()
   : ossimNitfTileSource()
{
}

ossimOpjNitfReader::~ossimOpjNitfReader()
{
   close();
}

bool ossimOpjNitfReader::canUncompress(
   const ossimNitfImageHeader* hdr) const
{
   if (!hdr)
   {
      return false;
   }
   if (hdr->getCompressionCode() == "C8") // jpeg
   {
      return true;
   }
   return false;
}

void ossimOpjNitfReader::initializeReadMode()
{
   // Initialize the read mode.
   theReadMode = READ_MODE_UNKNOWN;
   
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if (!hdr)
   {
      return;
   }

   if ( (hdr->getIMode() == "B") && (hdr->getCompressionCode()== "C8") )
   {
      theReadMode = READ_JPEG_BLOCK; 
   }
}

void ossimOpjNitfReader::initializeCompressedBuf()
{
   //---
   // If all block sizes are the same initialize theCompressedBuf; else,
   // we will allocate on each loadBlock.
   //---
   if (theNitfBlockSize.size() == 0)
   {
      theCompressedBuf.clear();
      return;
   }
   std::vector<ossim_uint32>::const_iterator i = theNitfBlockSize.begin();
   ossim_uint32 size = (*i);
   ++i;
   while (i != theNitfBlockSize.end())
   {
      if ((*i) != size)
      {
         theCompressedBuf.clear();
         return; // block sizes different
      }
      ++i;
   }
   theCompressedBuf.resize(size); // block sizes all the same.
}

bool ossimOpjNitfReader::scanForJpegBlockOffsets()
{
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   
   if ( !hdr || (theReadMode != READ_JPEG_BLOCK) || !theFileStr )
   {
      return false;
   }

   theNitfBlockOffset.clear();
   theNitfBlockSize.clear();

   //---
   // NOTE:
   // SOC = 0xff4f Start of Codestream
   // SOT = 0xff90 Start of tile
   // SOD = 0xff93 Last marker in each tile
   // EOC = 0xffd9 End of Codestream
   //---
   char c;

   // Seek to the first block.
   theFileStr.seekg(hdr->getDataLocation(), ios::beg);
   if (theFileStr.fail())
   {
      return false;
   }
   
   // Read the first two bytes and verify it is SOC; if not, get out.
   theFileStr.get( c );
   if (static_cast<ossim_uint8>(c) != 0xff)
   {
      return false;
   }
   theFileStr.get(c);
   if (static_cast<ossim_uint8>(c) != 0x4f)
   {
      return false;
   }

   ossim_uint32 blockSize = 2;  // Read two bytes...

   // Add the first offset.
   // theNitfBlockOffset.push_back(hdr->getDataLocation());

   // Find all the SOC markers.
   while ( theFileStr.get(c) ) 
   {
      ++blockSize;
      if (static_cast<ossim_uint8>(c) == 0xff)
      {
         if ( theFileStr.get(c) )
         {
            ++blockSize;

            if (static_cast<ossim_uint8>(c) == 0x90) // At SOC marker...
            {
               std::streamoff pos = theFileStr.tellg();
               theNitfBlockOffset.push_back(pos-2);
            }
            else if (static_cast<ossim_uint8>(c) == 0x93) // At EOC marker...
            {
               // Capture the size of this block.
               theNitfBlockSize.push_back(blockSize);
               blockSize = 0;
            }
         }
      }
   }

   theFileStr.seekg(0, ios::beg);
   theFileStr.clear();

   // We should have the same amount of offsets as we do blocks...
   ossim_uint32 total_blocks =
      hdr->getNumberOfBlocksPerRow()*hdr->getNumberOfBlocksPerCol();
   
   if (theNitfBlockOffset.size() != total_blocks)
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "DEBUG:"
            << "\nBlock offset count wrong!"
            << "\nblocks:  " << total_blocks
            << "\noffsets:  " << theNitfBlockOffset.size()
            << std::endl;
      }
      
      return false;
   }
   if (theNitfBlockSize.size() != total_blocks)
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "DEBUG:"
            << "\nBlock size count wrong!"
            << "\nblocks:  " << total_blocks
            << "\nblock size array:  " << theNitfBlockSize.size()
            << std::endl;
      }

      return false;
   }

   return true;
}

bool ossimOpjNitfReader::uncompressJpegBlock(ossim_uint32 x,
                                             ossim_uint32 y)
{
   ossim_uint32 blockNumber = getBlockNumber(ossimIpt(x,y));

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimNitfTileSource::uncompressJpegBlock DEBUG:"
         << "\nblockNumber:  " << blockNumber
         << "\noffset to block: " << theNitfBlockOffset[blockNumber]
         << "\nblock size: " << theNitfBlockSize[blockNumber]
         << std::endl;
   }
   
   // Seek to the block.
   theFileStr.seekg(theNitfBlockOffset[blockNumber], ios::beg);

   //---
   // Get a buffer to read the compressed block into.  If all the blocks are
   // the same size this will be theCompressedBuf; else we will make our own.
   //---
   ossim_uint8* compressedBuf = &theCompressedBuf.front();
   if (!compressedBuf)
   {
      compressedBuf = new ossim_uint8[theNitfBlockSize[blockNumber]];
   }


   // Read the block into memory.  We could store this
   if (!theFileStr.read((char*)compressedBuf, theNitfBlockSize[blockNumber]))
   {
      theFileStr.clear();
      ossimNotify(ossimNotifyLevel_FATAL)
         << "ossimNitfTileSource::loadBlock Read Error!"
         << "\nReturning error..." << endl;
      theErrorStatus = ossimErrorCodes::OSSIM_ERROR;
      delete [] compressedBuf;
      compressedBuf = 0;
      return false;
   }

   try
   {
      //theCacheTile = decoder.decodeBuffer(compressedBuf,
      //                                    theNitfBlockSize[blockNumber]);
   }
   catch (const ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_FATAL)
         << e.what() << std::endl;
      theErrorStatus = ossimErrorCodes::OSSIM_ERROR;
   }

   // If theCompressedBuf is null that means we allocated the compressedBuf.
   if (theCompressedBuf.size() == 0)
   {
      delete [] compressedBuf;
      compressedBuf = 0;
   }
   
   if (theErrorStatus == ossimErrorCodes::OSSIM_ERROR)
   {
      return false;
   }
   return true;
}

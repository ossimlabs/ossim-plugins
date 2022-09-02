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
#include <ossimOpjCommon.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/support_data/ossimNitfImageHeader.h>

using namespace std;

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

   // openjpeg handles scanning for blocks
   return true;
}


bool ossimOpjNitfReader::uncompressJpegBlock(ossim_uint32 x,
                                             ossim_uint32 y)
{
   const ossimNitfImageHeader *hdr = getCurrentImageHeader();
   if (!hdr)
   {
      return false;
   }
   ossim_uint32 numX = theCacheSize.x;
   ossim_uint32 numY = theCacheSize.y;
   ossim_uint32 blockX = x / numX;
   ossim_uint32 blockY = y / numY;

   ossimIrect rect(ossimIpt(blockX * numX, blockY * numY),
                   ossimIpt((blockX + 1) * numX - 1, (blockY + 1) * numY - 1));

   std::streamoff offset = hdr->getDataLocation();
   theFileStr->seekg(offset, ios::beg);
   ossim_int32 format = ossim::getCodecFormat(theFileStr.get());
   if (format == OPJ_CODEC_UNKNOWN)
   {
      ossimNotify(ossimNotifyLevel_WARN)
          << "ossimOpjNitfReader::uncompressJpegBlock WARNING!\n"
          << "Unknown openjpeg codec\n";
      return false;
   }

   if (ossim::opj_decode(theFileStr.get(), rect, 0, format, offset, theCacheTile.get()) == false)
   {
       return false;
   }

   return true;
}

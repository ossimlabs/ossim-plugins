//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file.
//
// Author:  David Burken
//
// Description:  Class declaration for NITF reader for j2k images using
// OpenJPEG library.
//
// $Id$
//----------------------------------------------------------------------------
#ifndef ossimOpjNitfReader_HEADER
#define ossimOpjNitfReader_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimNitfTileSource.h>
// #include <ossimJ2kSizRecord.h>
// #include <ossimJ2kSotRecord.h>

class OSSIM_PLUGINS_DLL ossimOpjNitfReader : public ossimNitfTileSource
{
public:

   /** default construtor */
   ossimOpjNitfReader();
   
   /** virtural destructor */
   virtual ~ossimOpjNitfReader();

protected:

   /**
    * @param hdr Pointer to image header.
    * @return true if reader can uncompress nitf.
    * */
   virtual bool canUncompress(const ossimNitfImageHeader* hdr) const;

   /**
    * Initializes the data member "theReadMode" from the current entry.
    */
   virtual void initializeReadMode();

   /**
    * Initializes the data member theCompressedBuf.
    */
   virtual void initializeCompressedBuf();

   /**
    * @brief does nothing as block offsets are scanned by openjpeg, not us
    * @return true on success, false on error.
    */
   virtual bool scanForJpegBlockOffsets();

   /**
    * @brief Uncompresses a jpeg block using the openjpeg library.
    * @param x sample location in image space.
    * @param y line location in image space.
    * @return true on success, false on error.
    */
   virtual bool uncompressJpegBlock(ossim_uint32 x, ossim_uint32 y);

private:

TYPE_DATA   
};

#endif /* #ifndef ossimOpjNitfReader_HEADER */

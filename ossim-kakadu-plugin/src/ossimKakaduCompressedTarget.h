//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:
//
// Class declaration for JPEG2000 (J2K) kdu_compressed_target that uses an
// ostream for writing to the file.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduCompressedTarget.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduCompressedTarget_HEADER
#define ossimKakaduCompressedTarget_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <kdu_compressed.h>
#include <kdu_elementary.h>
#include <iosfwd>

/**
 * @brief ossimKakaduCompressedTarget JPEG2000 (J2K) kdu_compressed_target
 * that uses an ostream for writing to the file.
 */
class ossimKakaduCompressedTarget :
   public kdu_core::kdu_compressed_target
{
public:

   /** default construtor */
   ossimKakaduCompressedTarget();
   
   /** virtural destructor */
   virtual ~ossimKakaduCompressedTarget();

   /**
    * @brief Sets the output stream.
    *
    * Note the ossimKakaduCompressedTarget only uses the stream it does not
    * take over(delete) memory on destruction.
    * 
    * @param stream  The output stream.
    */
   void setStream(std::ostream* stream);

   /**
    * @brief Write method.
    * @param buf The buffer to pull from.
    * @param num_bytes The number of bytes to write.
    * @return true on success, false on error.
    */
   virtual bool write(const kdu_core::kdu_byte *buf, int num_bytes);

   virtual bool start_rewrite(kdu_core::kdu_long backtrack);

   virtual bool end_rewrite();

private:

   std::ostream*  m_stream;
   std::streamoff m_restorePosition; // For end_rewrite.

};

#endif /* #ifndef ossimKakaduCompressedTarget_HEADER */

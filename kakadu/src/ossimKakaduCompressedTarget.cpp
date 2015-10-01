//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:
//
// Class definition for JPEG2000 (J2K) kdu_compressed_target that uses an
// ostream for writing to the file.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduCompressedTarget.cpp 22884 2014-09-12 13:14:35Z dburken $

#include "ossimKakaduCompressedTarget.h"
#include <iostream>
#include <ostream>

ossimKakaduCompressedTarget::ossimKakaduCompressedTarget()
   : m_stream(0),
     m_restorePosition(0)
{
}

ossimKakaduCompressedTarget::~ossimKakaduCompressedTarget()
{
}
 
void ossimKakaduCompressedTarget::setStream(std::ostream* stream)
{
   if ( stream )
   {
      m_stream = stream;

      //---
      // Keep track of the stream position for the start_rewrite call.
      // ostream::tellp() on windows visual studio 10 x64 fails past
      // 2^31-1(2GB) even though the std::streamoff held in the
      // std::streampos is eight bytes.
      // This call is safe as the stream is typically at 0.
      //---
      m_restorePosition = stream->tellp();
   }
}

bool ossimKakaduCompressedTarget::write(const kdu_core::kdu_byte *buf, int num_bytes)
{
   if ( m_restorePosition == 0 )
   {
      //---
      // First write call.  Do a tellp in case the stream was moved as in the case of
      // nitf where the nitf header and nitf image header get written before we write
      // pixel data.
      //---
      m_restorePosition = m_stream->tellp();
   }
   
   bool result = false;
   if (m_stream)
   {
      m_stream->write((const char*)buf,
                      static_cast<std::streamsize>(num_bytes));
      
      //---
      // Keep track of the stream postition for the start_rewrite call.  Do it
      // manually as ostream::tellp() on windows visual studio 10 x64 fails
      // past 2^31-1(2GB) even though the std::streamoff held in the
      // std::streampos is eight bytes.
      //---
      m_restorePosition += static_cast<std::streamoff>(num_bytes);
      
      result = m_stream->good();
   }
   return result;
}

bool ossimKakaduCompressedTarget::start_rewrite(kdu_core::kdu_long backtrack)
{
   bool result = false;
   if (m_stream)
   {
      std::streamoff off = m_restorePosition - static_cast<std::streamoff>(backtrack);
      m_stream->seekp(off, std::ios_base::beg);
      result = m_stream->good();
   }
   return result;
}

bool ossimKakaduCompressedTarget::end_rewrite()
{
   bool result = false;
   if (m_stream)
   {
      m_stream->seekp(m_restorePosition, std::ios_base::beg);
      result = m_stream->good();
   }
   return result;
}

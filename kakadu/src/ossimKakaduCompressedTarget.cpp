//---
//
// License: MIT
// 
//
// Description:
//
// Class definition for JPEG2000 (J2K) kdu_compressed_target that uses an
// ostream for writing to the file.
//
//---
// $Id$

#include "ossimKakaduCompressedTarget.h"
#include <iostream>
#include <ostream>

ossimKakaduCompressedTarget::ossimKakaduCompressedTarget()
   : m_stream(0),
     m_startOfCodestream(-1),
     m_restorePosition(-1)
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
   }
}

bool ossimKakaduCompressedTarget::write(const kdu_core::kdu_byte *buf, int num_bytes)
{
   bool result = false;
   if (m_stream)
   {
      // First write capture the stream position.
      if ( m_startOfCodestream == -1 )
      {
         m_startOfCodestream = m_stream->tellp();
      }

      m_stream->write((const char*)buf,
                      static_cast<std::streamsize>(num_bytes));
      result = m_stream->good();
   }
   return result;
}

bool ossimKakaduCompressedTarget::start_rewrite(kdu_core::kdu_long backtrack)
{
   bool result = false;
   if (m_stream)
   {
      m_restorePosition = m_stream->tellp();

      std::streampos pos = m_restorePosition - static_cast<std::streamoff>(backtrack);

#if 0 /* please keep for debug. drb - 20190108 */
      std::cout << "start_rewrite:"
                << "\nm_startOfCodestream: " << m_startOfCodestream
                << "\nm_restorePosition:   " << m_restorePosition
                << "\nbacktrack:           " << backtrack
                << "\npos:                 " << pos << std::endl;
#endif
      
      if ( pos >= m_startOfCodestream )
      {
         m_stream->seekp( pos );
         result = m_stream->good();
      }
   }
   return result;
}

bool ossimKakaduCompressedTarget::end_rewrite()
{
   bool result = false;
   if (m_stream)
   {
      m_stream->seekp( m_restorePosition );
      result = m_stream->good();
   }
   return result;
}

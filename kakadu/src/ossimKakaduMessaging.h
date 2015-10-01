//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Common OSSIM Kakadu messaging definitions.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduMessaging.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduMessaging_HEADER
#define ossimKakaduMessaging_HEADER 1

#include <ossim/base/ossimNotify.h>
#include <kdu_messaging.h>
#include <ostream>
#include <string>

/* ========================================================================= */
/* Set up kakadu messaging services to be sent to ossimNotify.               */
/* ========================================================================= */
class kdu_stream_message : public kdu_core::kdu_thread_safe_message
{
public: // Member classes
   kdu_stream_message( std::ostream *stream, bool throw_exc )
      : kdu_core::kdu_thread_safe_message(),
        m_stream(stream),
        m_throw_exc(throw_exc)
   {}

   virtual void put_text(const char *string)
   {
      (*m_stream) << string;
   }

   virtual void flush(bool end_of_message=false)
   {
      m_stream->flush();

      // This call unlocks mutex in kakadu_thread_safe_message:
      kdu_core::kdu_thread_safe_message::flush(end_of_message);

      if (end_of_message && m_throw_exc)
      {
         //---
         // Using ossimException caused hang.  From kakadu kdu_messaging.h:
         // Unless you have good reason to do otherwise, you are strongly
         // recommended to use the default `exception_val' of
         // `KDU_ERROR_EXCEPTION'
         //---
         throw KDU_ERROR_EXCEPTION;
      }
   }
   
private: // Data
   std::ostream* m_stream;
   bool m_throw_exc;
};

static kdu_stream_message cout_message(&ossimNotify(ossimNotifyLevel_NOTICE),
                                       false);
static kdu_stream_message cerr_message(&ossimNotify(ossimNotifyLevel_WARN),
                                       true);
static kdu_core::kdu_message_formatter pretty_cout(&cout_message);
static kdu_core::kdu_message_formatter pretty_cerr(&cerr_message);

#endif /* #ifndef ossimKakaduMessaging_HEADER */

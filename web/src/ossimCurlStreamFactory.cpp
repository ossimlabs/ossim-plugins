//---
//
// License: MIT
//
// Description: OSSIM Amazon Web Services (AWS) plugin initialization
// code.
//
//---
// $Id$

#include "ossimCurlStreamFactory.h"
#include "ossimCurlIStream.h"
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimTrace.h>

static ossimTrace traceDebug("ossimCurlStreamFactory:debug");

ossim::CurlStreamFactory* ossim::CurlStreamFactory::m_instance = 0;

ossim::CurlStreamFactory::~CurlStreamFactory()
{
}

ossim::CurlStreamFactory* ossim::CurlStreamFactory::instance()
{
   if(!m_instance)
   {
      m_instance = new ossim::CurlStreamFactory();
   }

   return m_instance;
}

std::shared_ptr<ossim::istream> ossim::CurlStreamFactory::createIstream(
   const std::string& connectionString, 
   const ossimKeywordlist& /* options */,
   std::ios_base::openmode openMode) const
{
  //std::cout << "ossim::CurlStreamFactory::createIstream............Entered\n";
   std::shared_ptr<ossim::CurlIStream> result = std::make_shared<ossim::CurlIStream>();
   if(traceDebug())
   {
     ossimNotify(ossimNotifyLevel_WARN) << "ossim::AwsStreamFactory::createIstream: Entered...............\n";
   }
   //---
   // Hack for upstream code calling ossimFilename::convertToNative()
   // wrecking s3 url.
   //---
#if defined(_WIN32)
   ossimFilename f = connectionString;
   f.convertBackToForwardSlashes();
   result->open( f.string(), openMode) ;
#else
   result->open( connectionString, openMode );
#endif
  
  if(!result->good())
  {
    result.reset();
  }
   if(traceDebug())
   {
     ossimNotify(ossimNotifyLevel_WARN) << "ossim::CurlStreamFactory::createIstream: Leaving...............\n";
   }

  return result;
}
      
std::shared_ptr<ossim::ostream> ossim::CurlStreamFactory::createOstream(
   const std::string& /*connectionString*/, 
   const ossimKeywordlist& /* options */,
   std::ios_base::openmode /*openMode*/) const
{
   return std::shared_ptr<ossim::ostream>(0);
}

std::shared_ptr<ossim::iostream> ossim::CurlStreamFactory::createIOstream(
   const std::string& /*connectionString*/, 
   const ossimKeywordlist& /* options */,
   std::ios_base::openmode /*openMode*/) const
{
   return std::shared_ptr<ossim::iostream>(0);
}

bool ossim::CurlStreamFactory::exists(const std::string& connectionString,
                                      bool& continueFlag) const
{
   bool result = false;
   if ( connectionString.size() )
   {
      ossimUrl url(connectionString);
      
      if( (url.getProtocol() == "http") || (url.getProtocol() == "https") )
      {
         continueFlag = false; // Set to false to stop registry search.

         ossimKeywordlist header;
         ossim_int64 filesize;
         ossimCurlHttpRequest curlHttpRequest;
         curlHttpRequest.set(url, header);
         filesize = m_curlHttpRequest.getContentLength();
         if ( filesize )
         {
            result = true;
         }
      }
      
   }
   return result;
}

// Hidden from use:
ossim::CurlStreamFactory::CurlStreamFactory()
   : m_curlHttpRequest()
{
}

// Hidden from use:
ossim::CurlStreamFactory::CurlStreamFactory(const ossim::CurlStreamFactory& )
   : m_curlHttpRequest()
{
}

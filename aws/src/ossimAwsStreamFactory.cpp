//---
//
// License: MIT
//
// Description: OSSIM Amazon Web Services (AWS) plugin initialization
// code.
//
//---
// $Id$

#include "ossimAwsStreamFactory.h"
#include "ossimS3IStream.h"
#include <ossim/base/ossimFilename.h>

ossim::AwsStreamFactory* ossim::AwsStreamFactory::m_instance = 0;

ossim::AwsStreamFactory::~AwsStreamFactory()
{
}

ossim::AwsStreamFactory* ossim::AwsStreamFactory::instance()
{
   if(!m_instance)
   {
      m_instance = new ossim::AwsStreamFactory();
   }

   return m_instance;
}

std::shared_ptr<ossim::istream> ossim::AwsStreamFactory::createIstream(
   const std::string& connectionString, std::ios_base::openmode openMode) const
{
   std::shared_ptr<ossim::S3IStream> result = std::make_shared<ossim::S3IStream>();

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

  return result;
}
      
std::shared_ptr<ossim::ostream> ossim::AwsStreamFactory::createOstream(
   const std::string& /*connectionString*/, std::ios_base::openmode /*openMode*/) const
{
   return std::shared_ptr<ossim::ostream>(0);
}

std::shared_ptr<ossim::iostream> ossim::AwsStreamFactory::createIOstream(
   const std::string& /*connectionString*/, std::ios_base::openmode /*openMode*/) const
{
   return std::shared_ptr<ossim::iostream>(0);
}

// Hidden from use:
ossim::AwsStreamFactory::AwsStreamFactory()
{
}

// Hidden from use:
ossim::AwsStreamFactory::AwsStreamFactory(const ossim::AwsStreamFactory& )
{
}

//---
//
// License: MIT
//
// Description: OSSIM Amazon Web Services (AWS) stream factory.
//
//---
// $Id$


#ifndef ossimAwsStreamFactory_HEADER
#define ossimAwsStreamFactory_HEADER 1

#include <ossim/base/ossimStreamFactoryBase.h>
#include <ossim/base/ossimIoStream.h>
#include <memory>

namespace ossim
{
   class AwsStreamFactory : public StreamFactoryBase
   {
   public:
      static AwsStreamFactory* instance();
      
      virtual ~AwsStreamFactory();

      virtual std::shared_ptr<ossim::istream>
         createIstream(const ossimString& connectionString,
                       std::ios_base::openmode openMode) const;
      
      virtual std::shared_ptr<ossim::ostream>
         createOstream(const ossimString& connectionString,
                       std::ios_base::openmode openMode) const;
      
      virtual std::shared_ptr<ossim::iostream>
         createIOstream(const ossimString& connectionString,
                        std::ios_base::openmode openMode) const;
   
   protected:
      AwsStreamFactory();
      AwsStreamFactory(const AwsStreamFactory&);
      
      static AwsStreamFactory* m_instance;
   };
}

#endif /* #ifndef ossimAwsStreamFactory_HEADER */


//---
//
// License: MIT
//
// Description: OSSIM Amazon Web Services (AWS) stream factory.
//
//---
// $Id$

#ifndef ossimCurlStreamFactory_HEADER
#define ossimCurlStreamFactory_HEADER 1

#include <ossim/base/ossimStreamFactoryBase.h>
#include <ossim/base/ossimIoStream.h>
#include <memory>

namespace ossim
{
   class CurlStreamFactory : public StreamFactoryBase
   {
   public:
      static CurlStreamFactory* instance();
      
      virtual ~CurlStreamFactory();

      virtual std::shared_ptr<ossim::istream>
         createIstream(const std::string& connectionString,
                       const ossimKeywordlist& options,
                       std::ios_base::openmode openMode) const;
      
      virtual std::shared_ptr<ossim::ostream>
         createOstream(const std::string& connectionString,
                       const ossimKeywordlist& options,
                       std::ios_base::openmode openMode) const;
      
      virtual std::shared_ptr<ossim::iostream>
         createIOstream(const std::string& connectionString,
                       const ossimKeywordlist& options,
                        std::ios_base::openmode openMode) const;
   
   protected:
      CurlStreamFactory();
      CurlStreamFactory(const CurlStreamFactory&);
      
      static CurlStreamFactory* m_instance;
   };
}

#endif /* #ifndef ossimCurlStreamFactory_HEADER */


//---
//
// License: MIT
//
// Description: OSSIM CURL stream factory.
//
//---
// $Id$

#ifndef ossimCurlStreamFactory_HEADER
#define ossimCurlStreamFactory_HEADER 1

#include <ossim/base/ossimStreamFactoryBase.h>
#include <ossim/base/ossimIoStream.h>
#include "ossimCurlHttpRequest.h"
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

      /**
       * @brief Methods to test if connection exists.
       *
       * @param connectionString
       * 
       * @param continueFlag Initializes by this, if set to false, indicates factory
       * handles file/url and no more factory checks are necessary.  If true,
       * connection is not handled by this factory.
       * 
       * @return true on success, false, if not.  
       */
      virtual bool exists(const std::string& connectionString,
                          bool& continueFlag) const;
   
   protected:
      CurlStreamFactory();
      CurlStreamFactory(const CurlStreamFactory&);

      ossimCurlHttpRequest m_curlHttpRequest;
      
      static CurlStreamFactory* m_instance;
   };
}

#endif /* #ifndef ossimCurlStreamFactory_HEADER */


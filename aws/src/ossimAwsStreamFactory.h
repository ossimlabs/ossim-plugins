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

namespace Aws
{
   namespace S3
   {
      class S3Client;
   }
}

namespace ossim
{
   class AwsStreamFactory : public StreamFactoryBase
   {
   public:
      static AwsStreamFactory* instance();
      
      virtual ~AwsStreamFactory();

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
       * @param connectionString
       * @param continueFlag Initializes by this, if set to true, indicates factory
       * handles file/url and no more factory checks are necessary.
       * @return true on success, false, if not.  
       */
      bool exists(const std::string& connectionString, bool& continueFlag) const;   
      
      std::shared_ptr<Aws::S3::S3Client> getSharedS3Client()const{return m_client;}

   protected:

      /**
       * @return true if starts with "s3://" or "S3://"
       */
      bool isS3Url( const ossimFilename& file ) const;
      
      void initClient() const;

      AwsStreamFactory();
      AwsStreamFactory(const AwsStreamFactory&);

     // mutable Aws::S3::S3Client* m_client;
      mutable std::shared_ptr<Aws::S3::S3Client> m_client;
      
      static AwsStreamFactory* m_instance;
   };
}

#endif /* #ifndef ossimAwsStreamFactory_HEADER */


#ifndef ossimCurlHttpRequest_HEADER
#define ossimCurlHttpRequest_HEADER
#include <ossim/base/ossimHttpResponse.h>
#include <ossim/base/ossimHttpRequest.h>
#include <curl/curl.h>

class ossimCurlHttpResponse : public ossimHttpResponse
{
public:
   
};

class ossimCurlHttpRequest : public ossimHttpRequest
{
public:
   ossimCurlHttpRequest()
   :m_curl(0)
   {
      m_curl = curl_easy_init();
   }
   virtual ~ossimCurlHttpRequest()
   {
      if(m_curl)
      {
         curl_easy_cleanup(m_curl);
         m_curl = 0;
      }
   }
   virtual ossimWebResponse* getResponse();
   virtual bool supportsProtocol(const ossimString& protocol)const;
   static int curlWriteResponseBody(void *buffer, size_t size, size_t nmemb, void *stream);
   static int curlWriteResponseHeader(void *buffer, size_t size, size_t nmemb, void *stream);
   
   virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix=0)
   {
      m_response = 0;
      return ossimHttpRequest::loadState(kwl, prefix);
   }
   
protected:
   CURL* m_curl;
   mutable ossimRefPtr<ossimCurlHttpResponse> m_response;
};
#endif

#include "ossimCurlHttpRequest.h"

ossimRefPtr<ossimWebResponse> ossimCurlHttpRequest::getResponse()
{
   ossimRefPtr<ossimWebResponse> response;
   ossimString protocol = getUrl().getProtocol();
   if(!supportsProtocol(protocol))
   {
      return 0;
   }
   curl_easy_reset(m_curl);
   clearLastError();
   switch (m_methodType) 
   {
      case ossimHttpRequest::HTTP_METHOD_GET:
      {
         ossimCurlHttpResponse* curlResponse = new ossimCurlHttpResponse();
         response = curlResponse;
         
         curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, curlWriteResponseHeader);
         curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, (void*)response.get());
         curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curlWriteResponseBody);
         curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)response.get());
         curl_easy_setopt(m_curl, CURLOPT_HEADER, 0); 
         curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0); 
         //curl_easy_setopt(m_curl, CURLOPT_CONNECT_ONLY, 1);
        // ossimKeywordlist::KeywordMap& headerMap = m_headerOptions.getMap();
        // struct curl_slist *headers=0; /* init to NULL is important */
         ossimString range = m_headerOptions.find("Range");
         range=range.substitute("bytes=", "");
         curl_easy_setopt(m_curl, CURLOPT_RANGE, range.c_str());
         // ossimKeywordlist::KeywordMap::iterator iter = headerMap.begin();
         // while(iter != headerMap.end())
         // {
         //    std::cout << ((*iter).first + ":"+(*iter).second).c_str() << std::endl;
         //    headers = curl_slist_append(headers, ((*iter).first + ":"+(*iter).second).c_str());
         //    ++iter;
         // }
         ossimString urlString = getUrl().toString();
         curl_easy_setopt(m_curl, CURLOPT_URL, urlString.c_str());
         
         
         if(protocol == "https")
         {
            curl_easy_setopt(m_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);//);
            curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
         }
         
         
         int rc = curl_easy_perform(m_curl);
         
         if(rc == CURLE_SSL_CONNECT_ERROR)
         {
            // try default if an error for SSL connect ocurred
            curl_easy_setopt(m_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
            rc = curl_easy_perform(m_curl);
         }
         bool result = (rc < 1);
         if(result)
         {
            curlResponse->convertHeaderStreamToKeywordlist();
         }
         else
         {
            m_lastError = curl_easy_strerror((CURLcode)rc);
            //std::cout << curl_easy_strerror((CURLcode)rc) << std::endl;
            response = 0;
         }
         break;
      }
         
      default:
         break;
   }
   
   return response;
}

bool ossimCurlHttpRequest::supportsProtocol(const ossimString& protocol)const
{
   bool result = false;
   if(protocol == "http")
   {
      result = true;
   }
   else if(protocol == "https")
   {
      curl_version_info_data * vinfo = curl_version_info( CURLVERSION_NOW ); 
      if( vinfo->features & CURL_VERSION_SSL ) 
      { 
         result = true;
      } 
   }
   return result;
}

int ossimCurlHttpRequest::curlWriteResponseBody(void *buffer, size_t size, size_t nmemb, void *stream)
{
   ossimHttpResponse* cStream = static_cast<ossimHttpResponse*>(stream);

   if(cStream)
   {
      cStream->bodyStream().write((char*)buffer, nmemb*size);
      return nmemb*size;
   }
   
   return 0;
}

int ossimCurlHttpRequest::curlWriteResponseHeader(void *buffer, size_t size, size_t nmemb, void *stream)
{
   ossimHttpResponse* cStream = static_cast<ossimHttpResponse*>(stream);
   if(cStream)
   {
      cStream->headerStream().write((char*)buffer, nmemb*size);
      return nmemb*size;
   }
   
   return 0;
}

ossim_int64 ossimCurlHttpRequest::getContentLength()const
{
   double contentLength=-1;
   curl_easy_reset(m_curl);
   clearLastError();
   ossimString urlString = getUrl().toString();
   ossimString protocol = getUrl().getProtocol();
   ossimRefPtr<ossimCurlHttpResponse> response = new ossimCurlHttpResponse();
   
   curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, curlWriteResponseHeader);
   curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, (void*)response.get());
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curlWriteResponseBody);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)response.get());
   curl_easy_setopt(m_curl, CURLOPT_HEADER, 0); 
   curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1); 
   curl_easy_setopt(m_curl, CURLOPT_RANGE, "");

   const ossimKeywordlist::KeywordMap& headerMap = m_headerOptions.getMap();
   struct curl_slist *headers=0; /* init to NULL is important */
   
   ossimKeywordlist::KeywordMap::const_iterator iter = headerMap.begin();
   while(iter != headerMap.end())
   {
//      std::cout << ((*iter).first + ":"+(*iter).second).c_str() << std::endl;
      headers = curl_slist_append(headers, ((*iter).first + ":"+(*iter).second).c_str());
      ++iter;
   }
   curl_easy_setopt(m_curl, CURLOPT_URL, urlString.c_str());
   if(protocol == "https")
   {
      curl_easy_setopt(m_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);//);
      curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
   }
   int rc = curl_easy_perform(m_curl);
         
   //rc = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
   if(rc == CURLE_SSL_CONNECT_ERROR)
   {
      // try default if an error for SSL connect ocurred
      curl_easy_setopt(m_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
      rc = curl_easy_perform(m_curl);   
   }
   bool result = (rc < 1);
   if(result)
   {
      response->convertHeaderStreamToKeywordlist();
      rc = curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
   //if(rc>=1) contentLength = -1;
   // response->convertHeaderStreamToKeywordlist();
   //std::cout << response->headerKwl() << "\n";
   }
   else
   {
      m_lastError = curl_easy_strerror((CURLcode)rc);
      //std::cout << curl_easy_strerror((CURLcode)rc) << std::endl;
   }

   return static_cast<ossim_int64> (contentLength);
}


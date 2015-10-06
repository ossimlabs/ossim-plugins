#include "ossimWebPluginRequestFactory.h"
#include "ossimCurlHttpRequest.h"
ossimWebPluginRequestFactory* ossimWebPluginRequestFactory::m_instance = 0;

ossimWebPluginRequestFactory::ossimWebPluginRequestFactory()
{
   m_instance = this;
}

ossimWebPluginRequestFactory* ossimWebPluginRequestFactory::instance()
{
   if(!m_instance)
   {
      m_instance = new ossimWebPluginRequestFactory();
   }
   
   return m_instance;
}

ossimWebRequest* ossimWebPluginRequestFactory::create(const ossimUrl& url)
{

   ossimRefPtr<ossimCurlHttpRequest> request = new ossimCurlHttpRequest();
   
   if(request->supportsProtocol(url.getProtocol()))
   {
      request->set(url, ossimKeywordlist());
   }
   else
   {
      request = 0;
   }
   return request.release();
}

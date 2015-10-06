#ifndef ossimWebPluginRequestFactory_HEADER
#define ossimWebPluginRequestFactory_HEADER
#include <ossim/base/ossimWebRequestFactoryBase.h>

class ossimWebPluginRequestFactory : public ossimWebRequestFactoryBase
{
public:
   static ossimWebPluginRequestFactory* instance();
   virtual ossimWebRequest* create(const ossimUrl& url);
   
protected:
   ossimWebPluginRequestFactory();
   
   static ossimWebPluginRequestFactory* m_instance;
};

#endif

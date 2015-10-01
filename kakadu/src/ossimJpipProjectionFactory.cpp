#include "ossimJpipProjectionFactory.h"
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimUrl.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>

RTTI_DEF1(ossimJpipProjectionFactory, "ossimJpipProjectionFactory", ossimProjectionFactoryBase)
ossimJpipProjectionFactory::ossimJpipProjectionFactory()
{
}

ossimJpipProjectionFactory* ossimJpipProjectionFactory::instance()
{
   static ossimJpipProjectionFactory inst;
   return &inst;
}

ossimProjection* ossimJpipProjectionFactory::createProjection(const ossimFilename& filename,
                                          ossim_uint32 entryIdx)const
{
   ossimProjection* result = 0;
   bool canGetInfo = (filename.ext().downcase() == "jpip");
   if(!canGetInfo)
   {
      ossimUrl url(filename.c_str());
      ossimString protocol  =url.getProtocol().downcase();
      if((protocol == "jpip")||
         (protocol == "jpips"))
      {
         canGetInfo = true;
      }
   }
      
   if(canGetInfo)
   {
      ossimRefPtr<ossimInfoBase> infoBase = ossimInfoFactoryRegistry::instance()->create(filename);
      if(infoBase.valid())
      {
         ossimKeywordlist kwl;
         infoBase->getKeywordlist(kwl);
         ossimString prefix = "jpip.image" + ossimString::toString(entryIdx) + ".";
         result = ossimProjectionFactoryRegistry::instance()->createProjection(kwl, prefix.c_str());
      }
   }
   return result;
}

ossimProjection* ossimJpipProjectionFactory::createProjection(const ossimString& /*name*/)const
{
   return 0;
}

ossimProjection* ossimJpipProjectionFactory::createProjection(const ossimKeywordlist& kwl,
                                          const char* prefix)const
{
   ossimProjection* result = 0;
   ossimString geojp2Prefix = ossimString(prefix) + "geojp2.";
   if(kwl.getNumberOfSubstringKeys(geojp2Prefix) > 0)
   {
      // try creating an ossim projection from a geojp2
      result = ossimProjectionFactoryRegistry::instance()->createProjection(kwl, geojp2Prefix.c_str());
   }
   else
   {
      // we will try to pick out XML type projeciton information
      //
   }
   return result;
}

ossimObject* ossimJpipProjectionFactory::createObject(const ossimString& /*typeName*/)const
{
   return 0;
}

ossimObject* ossimJpipProjectionFactory::createObject(const ossimKeywordlist& /*kwl*/,
                                  const char* /*prefix*/)const
{
   return 0;
}

void ossimJpipProjectionFactory::getTypeNameList(std::vector<ossimString>& /*typeList*/)const
{
   
}


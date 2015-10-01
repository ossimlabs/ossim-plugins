#include "ossimKakaduJpipImageGeometryFactory.h"
#include "ossimKakaduJpipHandler.h"
#include <ossim/projection/ossimProjectionFactoryRegistry.h>

ossimKakaduJpipImageGeometryFactory::ossimKakaduJpipImageGeometryFactory()
{
}

ossimKakaduJpipImageGeometryFactory* ossimKakaduJpipImageGeometryFactory::instance()
{
   static ossimKakaduJpipImageGeometryFactory inst;
   return &inst;
}

ossimImageGeometry* ossimKakaduJpipImageGeometryFactory::createGeometry(const ossimString& typeName)const
{   
   return 0;
}

ossimImageGeometry* ossimKakaduJpipImageGeometryFactory::createGeometry(
                                                                        const ossimKeywordlist& kwl, 
                                                                        const char* prefix)const
{
   ossimRefPtr<ossimImageGeometry> result = 0;   
   //std::cout << "ossimKakaduJpipImageGeometryFactory::createGeometry (kwl, prefix).................. entered" << std::endl;
   
   ossimRefPtr<ossimProjection> proj = ossimProjectionFactoryRegistry::instance()->createProjection(kwl, prefix);
   if(proj.valid())
   {
      result = new ossimImageGeometry();
      result->setProjection(proj.get());
      
      ossimRefPtr<ossim2dTo2dTransform> transform = createTransform(kwl, prefix);
      result->setTransform(transform.get());
   }
   return result.release();
}

ossimImageGeometry* ossimKakaduJpipImageGeometryFactory::createGeometry(
                                                              const ossimFilename& /* filename */, ossim_uint32 /* entryIdx */)const
{
   //std::cout << "ossimKakaduJpipImageGeometryFactory::createGeometry(filename) .................. entered" << std::endl;
   // currently don't support this option just yet by this factory
   return 0;
}

bool ossimKakaduJpipImageGeometryFactory::extendGeometry(ossimImageHandler* handler)const
{
   //std::cout << "ossimKakaduJpipImageGeometryFactory::extendGeometry(handler) .................. entered" << std::endl;
   bool result = false;
   if (handler)
   {
      bool add2D = true;
      ossimRefPtr<ossimImageGeometry> geom = handler->getImageGeometry();
      if(geom.valid())
      {
         if(!geom->getProjection())
         {
//            geom->setProjection(createProjection(handler));
//            result = geom->hasProjection();
         }
         if(geom->getProjection())
         {
//            if( !(dynamic_cast<ossimSensorModel*>(geom->getProjection())))
//            {
//               add2D = false;
//            }
         }
         if(!geom->getTransform()&&add2D)
         {
            geom->setTransform(createTransform(handler));
            result |= geom->hasTransform();
         }
      }
   }
   
   return result;
}

void ossimKakaduJpipImageGeometryFactory::getTypeNameList(
                                                std::vector<ossimString>& typeList)const
{
}


ossim2dTo2dTransform* ossimKakaduJpipImageGeometryFactory::createTransform(
                                                                 ossimImageHandler* handler)const
{
   // Currently nothing to do...
   
   ossimRefPtr<ossim2dTo2dTransform> result = 0;
   
   return result.release();
}

ossim2dTo2dTransform* ossimKakaduJpipImageGeometryFactory::createTransform(const ossimKeywordlist& kwl, 
                                                                           const char* prefix)const
{
   // Currently nothing to do...
   
   ossimRefPtr<ossim2dTo2dTransform> result = 0;
   
   return result.release();
}


ossimProjection* ossimKakaduJpipImageGeometryFactory::createProjection(
                                                             ossimImageHandler* handler) const
{
   ossimRefPtr<ossimProjection> result =
   ossimProjectionFactoryRegistry::instance()->createProjection(handler);
   
   return result.release();
}

  #include "ossimIvtGeomXformVisitor.h"
  #include <ossim/imaging/ossimImageRenderer.h>
  #include <ossim/imaging/ossimImageHandler.h>


  void ossimIvtGeomXformVisitor::visit(ossimObject* obj)
  {
    if(!hasVisited(obj))
    {
      ossimImageRenderer* renderer = dynamic_cast<ossimImageRenderer*>(obj);
      if(renderer)
      {
        ossimImageViewProjectionTransform* ivpt = dynamic_cast<ossimImageViewProjectionTransform*>(renderer->getImageViewTransform());
        if(ivpt)
        {
          m_transformList.push_back(new ossimIvtGeomXform(ivpt, ivpt->getImageGeometry()) );
        }
        else
        {
          ossimImageViewAffineTransform* ivat = dynamic_cast<ossimImageViewAffineTransform*>(renderer->getImageViewTransform());
          if(ivat&&renderer->getInput())
          {
            ossimTypeNameVisitor v("ossimImageHandler", true);
            renderer->accept(v);
            
            ossimImageHandler* handler = v.getObjectAs<ossimImageHandler>(0);
            if(handler)
            {
              ossimRefPtr<ossimImageGeometry> geom = handler->getImageGeometry();
              if(geom.valid())
              {
                m_transformList.push_back(new ossimIvtGeomXform(ivat, geom.get()) );
              }
            }
          }
        }
      }
      ossimVisitor::visit(obj);
    }
  }

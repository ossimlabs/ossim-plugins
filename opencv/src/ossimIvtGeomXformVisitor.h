#ifndef ossimIvtGeomXformVisitor_HEADER
#define ossimIvtGeomXformVisitor_HEADER
#include <ossim/base/ossimVisitor.h>
#include "ossimIvtGeomXform.h"

class ossimIvtGeomXformVisitor : public ossimVisitor
{
public:
   typedef std::vector<ossimRefPtr<ossimIvtGeomXform> > TransformList;

   ossimIvtGeomXformVisitor(int visitorType =(VISIT_INPUTS|VISIT_CHILDREN))
   :ossimVisitor(visitorType)
   {}
   
   virtual ossimRefPtr<ossimVisitor> dup()const{return new ossimIvtGeomXformVisitor(*this);}
   virtual void visit(ossimObject* obj);

   TransformList& getTransformList(){return m_transformList;}
   const TransformList& getTransformList()const{return m_transformList;}

protected:
   TransformList m_transformList;
};

#endif
#ifndef ossimIvtGeomXform_HEADER
#define ossimIvtGeomXform_HEADER
#include <ossim/base/ossimReferenced.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>
#include <ossim/projection/ossimImageViewAffineTransform.h>

class ossimIvtGeomXform : public ossimReferenced
{
public:
    ossimIvtGeomXform(ossimImageViewTransform* trans, 
                           ossimImageGeometry* geom)
    :
    m_ivt(trans),
    m_geom(geom)
    {}

    void viewToImage(const ossimDpt& viewPt, ossimDpt& ipt);
    void imageToView(const ossimDpt& ipt, ossimDpt& viewPt);
    void imageToGround(const ossimDpt& ipt, ossimGpt& gpt);
    void groundToImage(const ossimGpt& gpt, ossimDpt& ipt);
    void viewToGround(const ossimDpt& viewPt, ossimGpt& gpt);
    void groundToView(const ossimGpt& gpt, ossimDpt& viewPt);
    ossimImageViewTransform* getIvt(){return m_ivt.get();}
    const ossimImageViewTransform* getIvt()const {return m_ivt.get();}
    ossimImageGeometry* getGeom(){return m_geom.get();}
    const ossimImageGeometry* getGeom()const{return m_geom.get();}

protected:
    ossimRefPtr<ossimImageViewTransform> m_ivt;
    ossimRefPtr<ossimImageGeometry> m_geom;
};

#endif

#include "ossimIvtGeomXform.h"


void ossimIvtGeomXform::viewToImage(const ossimDpt& viewPt, ossimDpt& ipt)
{
  ipt.makeNan();
  if(m_ivt.valid())
  {
    m_ivt->viewToImage(viewPt, ipt);
  }
}

void ossimIvtGeomXform::imageToView(const ossimDpt& ipt, ossimDpt& viewPt)
{
  viewPt.makeNan();
  if(m_ivt.valid())
  {
    m_ivt->imageToView(ipt, viewPt);
  }
}
void ossimIvtGeomXform::imageToGround(const ossimDpt& ipt, ossimGpt& gpt)
{
  gpt.makeNan();
  if(m_geom.valid())
  {
    m_geom->localToWorld(ipt, gpt);
  }
}
void ossimIvtGeomXform::groundToImage(const ossimGpt& gpt, ossimDpt& ipt)
{
  ipt.makeNan();
  if(m_geom.valid())
  {
    m_geom->worldToLocal(gpt, ipt);
  }
}

void ossimIvtGeomXform::viewToGround(const ossimDpt& viewPt, ossimGpt& gpt)
{
  ossimDpt ipt;
  gpt.makeNan();
  viewToImage(viewPt, ipt);
  if(!ipt.hasNans())
  {
    imageToGround(ipt, gpt);
  }
}

void ossimIvtGeomXform::groundToView(const ossimGpt& gpt, ossimDpt& viewPt)
{
  ossimDpt ipt;
  viewPt.makeNan();

  groundToImage(gpt, ipt);
  if(!ipt.hasNans())
  {
    imageToView(ipt, viewPt);
  }
}


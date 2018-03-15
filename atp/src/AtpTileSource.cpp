//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "AtpTileSource.h"
#include <ossim/imaging/ossimImageData.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/base/ossimVisitor.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>

using namespace std;

namespace ATP
{
AtpTileSource::AtpTileSource()
:  m_maxResidualMagnitude(0)
{
}

AtpTileSource::AtpTileSource(ossimConnectableObject::ConnectableObjectList& inputSources)
:  ossimImageCombiner(inputSources),
   m_maxResidualMagnitude(0)
{
}

AtpTileSource::AtpTileSource(AtpGenerator* generator)
:  m_generator(generator),
   m_maxResidualMagnitude(0)
{
   theComputeFullResBoundsFlag = true;
   if (m_generator)
   {
      connectMyInputTo(0, m_generator->m_refChain.get());
      connectMyInputTo(1, m_generator->m_cmpChain.get());

      m_refIVT = m_generator->m_refIVT;
      m_cmpIVT = m_generator->m_cmpIVT;
   }
   addListener((ossimConnectableObjectListener*)this);
   initialize();
}

void AtpTileSource::initialize()
{
   ossimImageCombiner::initialize();
   allocate();
   m_tiePoints.clear();
   m_maxResidualMagnitude = 0.0;
   if ( getNumberOfInputs() < 2)
      return;

   m_refChain = (ossimImageSource*) getInput(0);
   m_cmpChain = (ossimImageSource*) getInput(1);

   // Need an IVT for each chain. They may already have one:
   if (!m_refIVT && !m_cmpIVT)
   {
      ossimTypeNameVisitor vref ("ossimImageRenderer");
      m_refChain->accept(vref);
      ossimImageRenderer* r = vref.getObjectAs<ossimImageRenderer>();
      if (r)
         m_refIVT = dynamic_cast<ossimImageViewProjectionTransform*>(r->getImageViewTransform());
      ossimTypeNameVisitor vcmp ("ossimImageRenderer");
      m_cmpChain->accept(vcmp);
      r = vcmp.getObjectAs<ossimImageRenderer>();
      if (r)
         m_cmpIVT = dynamic_cast<ossimImageViewProjectionTransform*>(r->getImageViewTransform());
   }
}

void AtpTileSource::allocate()
{
   m_tile = NULL;
   if ( getNumberOfInputs() > 1)
   {
      m_tile = ossimImageDataFactory::instance()->create(this, OSSIM_DOUBLE, 3);
      m_tile->initialize();
   }
}

string  AtpTileSource::getRefImageID()
{
   ossimImageHandler* handler = getImageHandler(m_refChain);
   if (!handler)
      return "";
   return handler->getImageID().string();
}

string AtpTileSource::getCmpImageID()
{
   ossimImageHandler* handler = getImageHandler(m_cmpChain);
   if (!handler)
      return "";
   return handler->getImageID().string();
}

string  AtpTileSource::getRefFilename()
{
   ossimImageHandler* handler = getImageHandler(m_refChain);
   if (!handler)
      return "";
   return handler->getFilename().string();
}

string  AtpTileSource::getCmpFilename()
{
   ossimImageHandler* handler = getImageHandler(m_cmpChain);
   if (!handler)
      return "";
   return handler->getFilename().string();
}

void AtpTileSource::setViewGeom(ossimImageGeometry* geom)
{
   if (!geom)
      return;

   if (m_refIVT)
      m_refIVT->setViewGeometry(geom);
   else if (m_refChain)
   {
      ossimImageHandler* handler = getImageHandler(m_refChain);
      if (handler)
         m_refIVT = new ossimImageViewProjectionTransform(handler->getImageGeometry().get(), geom);
   }
   if (m_cmpIVT)
      m_cmpIVT->setViewGeometry(geom);
   else if (m_cmpChain)
   {
      ossimImageHandler* handler = getImageHandler(m_cmpChain);
      if (handler)
         m_cmpIVT = new ossimImageViewProjectionTransform(handler->getImageGeometry().get(), geom);
   }
}

ossimImageHandler* AtpTileSource::getImageHandler(ossimRefPtr<ossimImageSource>& chain)
{
   if (!chain)
      return 0;

   ossimTypeNameVisitor visitor ("ossimImageHandler");
   chain->accept(visitor);
   ossimImageHandler* handler = (ossimImageHandler*) visitor.getObjectAs<ossimImageHandler>();

   return handler;
}


}


//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "AtpTileSource.h"
#include "../AtpCommon.h"
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

void AtpTileSource::filterPoints()
{
   const char* MODULE = "AtpTileSource::filterPoints() -- ";

   AtpConfig& config = AtpConfig::instance();
   if (config.diagnosticLevel(3))
   {
      // Annotate red before filtering:
      m_generator->m_annotatedRefImage->annotateResiduals(m_tiePoints, 255, 0, 0);
      m_generator->m_annotatedCmpImage->annotateCorrelations(m_tiePoints, 255, 0, 0);
   }

   if (!config.getParameter("useRasterMode").asBool())
   {
      removeBadMatches();
      if (config.diagnosticLevel(3))
      {
         // Annotate yellow before pruning:
         m_generator->m_annotatedRefImage->annotateResiduals(m_tiePoints, 255, 255, 0);
         m_generator->m_annotatedCmpImage->annotateCorrelations(m_tiePoints, 255, 255, 0);
      }

      pruneList();

      if (config.diagnosticLevel(2))
         CINFO << MODULE << "After filtering: num TPs in tile = " << m_tiePoints.size() << endl;
   }

   if (config.diagnosticLevel(3))
   {
      // Annotate Green after accepting TP:
      m_generator->m_annotatedRefImage->annotateResiduals(m_tiePoints, 0, 255, 0);
      m_generator->m_annotatedCmpImage->annotateCorrelations(m_tiePoints, 0, 255, 0);
      //m_annotatedRefImage->annotateTPIDs(m_tiePoints, 180, 180, 0);
   }

}

void AtpTileSource::removeBadMatches()
{
   const char* MODULE = "AtpTileSource::removeBadMatches()  ";

   // Check for consistency check override:
   AtpConfig& config = AtpConfig::instance();
   unsigned int minNumConsistent = config.getParameter("minNumConsistentNeighbors").asUint();
   if (minNumConsistent == 0)
      return;

   // Can't filter without neighbors, so remove all just in case they are bad:
   if (m_tiePoints.size() < minNumConsistent)
   {
      m_tiePoints.clear();
      return;
   }

   // Loop to eliminate bad peaks and inconsistent ATPs in tile:
   auto iter = m_tiePoints.begin();
   while (iter != m_tiePoints.end())
   {
      if (!(*iter)) // Should never happen
      {
         CWARN<<MODULE<<"WARNING: Null AutoTiePoint encountred in list. Skipping point."<<endl;
         ++iter;
         continue;
      }

      // The "tiepoint" may not have any matches -- it may be just a feature:
      if ((*iter)->hasValidMatch())
         (*iter)->findBestConsistentMatch(m_tiePoints);

      if (!(*iter)->hasValidMatch())
      {
         // Remove this TP from our list as soon as it is discovered that there are no valid
         // peaks. All TPs in list must have valid peaks:
         if (config.diagnosticLevel(5))
            CINFO<<MODULE<<"Removing TP "<<(*iter)->getTiePointId()<<endl;
         iter = m_tiePoints.erase(iter);
         if (m_tiePoints.empty())
            break;
      }
      else
         iter++;
   }
}

void AtpTileSource::pruneList()
{
   const char* MODULE = "AtpTileSource::pruneList()  ";

   AtpConfig &config =  AtpConfig::instance();
   if (config.getParameter("useRasterMode").asBool())
   {
      // For raster mode, simply remove entries with no match:
      auto atp = m_tiePoints.begin();
      while (atp != m_tiePoints.end())
      {
         if (*atp && (*atp)->hasValidMatch())
            ++atp;
         else
            atp = m_tiePoints.erase(atp);
      }
      return;
   }

   // Use a map to sort by confidence measure:
   multimap<double, shared_ptr<AutoTiePoint> > atpMap;
   auto atp = m_tiePoints.begin();
   double confidence;
   while (atp != m_tiePoints.end())
   {
      if (!(*atp)) // Should never happen
      {
         CWARN<<MODULE<<"WARNING: Null AutoTiePoint encountred in list. Skipping point."<<endl;
         ++atp;
         continue;
      }

      // The "tiepoint" may not have any matches -- it may be just a feature:
      bool hasValidMatch = (*atp)->getConfidenceMeasure(confidence);
      if (hasValidMatch)
         atpMap.emplace(1.0/confidence, *atp);

      atp++;
   }

   // Skim off the top best:
   unsigned int N = config.getParameter("numTiePointsPerTile").asUint();
   m_tiePoints.clear();
   multimap<double, shared_ptr<AutoTiePoint> >::iterator tp = atpMap.begin();
   while (tp != atpMap.end())
   {
      m_tiePoints.push_back(tp->second);
      if (m_tiePoints.size() == N)
         break;
      tp++;
   }
}

}


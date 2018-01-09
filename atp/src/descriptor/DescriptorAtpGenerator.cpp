//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "DescriptorAtpGenerator.h"
#include "DescriptorMatchTileSource.h"
#include "../AtpConfig.h"
#include "../AutoTiePoint.h"
#include <ossim/base/ossimException.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimBandAverageFilter.h>
#include <ossim/imaging/ossimBandSelector.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/imaging/ossimTiffWriter.h>
#include <ossim/projection/ossimUtmProjection.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <ossim/imaging/ossimAnnotationSource.h>
#include <ossim/imaging/ossimAnnotationLineObject.h>
#include <ossim/imaging/ossimBlendMosaic.h>
#include <ossim/imaging/ossimVertexExtractor.h>
#include <cmath>

namespace ATP // Namespace for all ossim-isa classes
{

DescriptorAtpGenerator::DescriptorAtpGenerator()
{
}

DescriptorAtpGenerator::~DescriptorAtpGenerator()
{

}

void DescriptorAtpGenerator::initialize()
{
   AtpGeneratorBase::initialize();
   AtpConfig& config = AtpConfig::instance();

   // Add the CorrelationSource filter to the REF chain, then add the CMP image to the correlator:
   m_atpTileSource = new DescriptorMatchTileSource;
   m_atpTileSource->connectMyInputTo(m_refChain.get());
   m_atpTileSource->connectMyInputTo(m_cmpChain.get());
   m_atpTileSource->setViewGeom(m_viewGeom.get());

   // Adjust AOI for half-width of correlation window:
   int patch_center = (config.getParameter("corrWindowSize").asUint() + 1) / 2;
   ossimIpt first_pos(m_aoiView.ul().x + patch_center, m_aoiView.ul().y + patch_center);
   ossimIpt last_pos(m_aoiView.lr().x - patch_center, m_aoiView.lr().y - patch_center);
   m_aoiView.set_ul(first_pos);
   m_aoiView.set_lr(last_pos);

}

} // End of namespace ATP

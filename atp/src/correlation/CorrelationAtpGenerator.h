//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef CorrelationAtpGenerator_H_
#define CorrelationAtpGenerator_H_

#include <AtpGeneratorBase.h>
#include <correlation/ossimCorrelationSource.h>
#include <AutoTiePoint.h>
#include <vector>

namespace ATP
{

/**
 * Top level controller for pixel-based, auto-correlation tiepoint finder for a pair of images.
 * The REF image contains the features, the CMP image is where that feature will be searched.
 */
class CorrelationAtpGenerator : public AtpGeneratorBase
{
public:
   CorrelationAtpGenerator();

   ~CorrelationAtpGenerator();

//   virtual bool renderTiePointDataAsImages(const ossimFilename& outBaseName,
//                                           const ossimFilename& lutFile=ossimFilename(""));

protected:
   virtual void initialize();
   ossimRefPtr<ossimImageChain> constructChain(shared_ptr<Image> image,
                                               vector<ossimDpt>& validVertices);

   ossimIpt m_tileInterval;
};
}
#endif /* CorrelationAtpGenerator_H_ */

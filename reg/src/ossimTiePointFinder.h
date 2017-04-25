//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimTiePointFinder_HEADER
#define ossimTiePointFinder_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandler.h>
#include "ossimTiePoint.h"
#include <vector>
#include <memory>

class ossimTiePointFinder
{
public:
   static const char* DESCRIPTION;

   ossimTiePointFinder();

   virtual ~ossimTiePointFinder();

   virtual void setImageList(std::vector<shared_ptr<ossimSingleImageChain> >& imageList);

   virtual bool execute();

   const std::vector<shared_ptr<ossimTiePoint> >& getTiePointList() const;

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

private:
   std::vector<shared_ptr<ossimTiePoint> > m_tiePoints;
};

#endif /* #ifndef ossimTiePointFinder_HEADER */

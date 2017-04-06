//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimRegTool_HEADER
#define ossimRegTool_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/util/ossimChipProcTool.h>

class OSSIM_DLL ossimRegTool : public ossimChipProcTool
{
public:
   static const char* DESCRIPTION;

   ossimRegTool();

   virtual ~ossimRegTool();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual void initialize(const ossimKeywordlist& ap);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimRegTool"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

private:
   virtual void initProcessingChain();
   virtual void finalizeChain();
};

#endif /* #ifndef ossimRegTool_HEADER */

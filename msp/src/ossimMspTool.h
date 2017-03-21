//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimMspTool_HEADER
#define ossimMspTool_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/util/ossimTool.h>

class OSSIM_DLL ossimMspTool : public ossimTool
{
public:
   static const char* DESCRIPTION;

   ossimMspTool();

   virtual ~ossimMspTool();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual void initialize(const ossimKeywordlist& ap);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimMspTool"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

private:
};

#endif /* #ifndef ossimMspTool_HEADER */

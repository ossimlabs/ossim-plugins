//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimPotraceUtility_HEADER
#define ossimPotraceUtility_HEADER 1

#include <ossim/util/ossimUtility.h>
#include <ossim/plugin/ossimPluginConstants.h>

class OSSIM_DLL ossimPotraceUtil : public ossimUtility
{
public:
   static const char* DESCRIPTION;

   ossimPotraceUtil();

   virtual ~ossimPotraceUtil();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual void initialize(const ossimKeywordlist& ap);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimPotraceUtil"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

private:
   double m_tolerance;

};

#endif /* #ifndef ossimPotraceUtility_HEADER */

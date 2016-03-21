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
#include <ossim/imaging/ossimImageHandler.h>

extern "C" {
#include "potracelib.h"
}

class OSSIM_DLL ossimPotraceUtil : public ossimUtility
{
public:
   static const char* DESCRIPTION;
   enum OutputMode { POLYGON, LINESTRING };

   ossimPotraceUtil();

   virtual ~ossimPotraceUtil();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual void initialize(const ossimKeywordlist& ap);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimPotraceUtil"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

private:
   potrace_bitmap_t* convertToBitmap();
   bool writeGeoJSON(potrace_path_t* vectorList);

   ossimFilename m_inputRasterFname;
   ossimFilename m_outputVectorFname;
   ossimFilename m_bitmapFname;
   OutputMode m_mode;
   ossimRefPtr<ossimImageHandler> m_inputHandler;
};

#endif /* #ifndef ossimPotraceUtility_HEADER */

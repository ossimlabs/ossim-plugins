//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/util/ossimUtilityRegistry.h>
#include <ossim/util/ossimUtility.h>
#include <assert.h>

int main(int argc, char** argv)
{
   ossimArgumentParser ap (&argc, argv);
   ossimInit::instance()->initialize(ap);

   ossimRefPtr<ossimUtility> util = ossimUtilityRegistry::instance()->createUtility("potrace");
   if (!util.valid())
   {
      cout << "Bad pointer returned from utility factory. Make sure ossim_potrace_plugin library "
            "exists and is properly referenced in your OSSIM preferences file." << endl;
      return 1;
   }

   if (!util->initialize(ap))
      return 1;

   if (!util->execute())
      return 1;

   return 0;
}



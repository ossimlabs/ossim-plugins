//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id: plugin-test.cpp 23401 2015-06-25 15:00:31Z okramer $
#include "../src/ossimCsm3Loader.h"
#include <ossim/base/ossimPreferences.h>

#define TEST_READER false

int main(int argc, char** argv)
{
   ossimPreferences::instance()->loadPreferences();
   ossimCsm3Loader ocl;
   return 0;
}



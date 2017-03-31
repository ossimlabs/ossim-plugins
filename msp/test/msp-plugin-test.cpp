//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************

#include "../src/ossimMspTool.h"
#include <common/csm/NitfIsd.h>

int main(int argc, char** argv)
{
   csm::Nitf21Isd isd  (argv[1]);
   cout<<"HEADER=<"<<isd.fileHeader()<<">\n"<<endl;
   return 0;
}



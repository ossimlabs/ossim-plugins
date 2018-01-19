//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef DescriptorAtpGenerator_H_
#define DescriptorAtpGenerator_H_

#include <ossim/base/ossimConstants.h>
#include "ossimDescriptorSource.h"
#include "../AtpGeneratorBase.h"
#include <ossim/base/ossimRefPtr.h>

namespace ATP // Namespace for all ossim-isa classes
{
class OSSIMDLLEXPORT DescriptorAtpGenerator : public AtpGeneratorBase
{
public:
   DescriptorAtpGenerator();

   ~DescriptorAtpGenerator();

private:
   virtual void initialize();
};
}
#endif /* DescriptorAtpGenerator_H_ */

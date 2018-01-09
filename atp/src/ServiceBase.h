//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef ServiceBase_HEADER
#define ServiceBase_HEADER 1

#include <ossim/base/JsonInterface.h>
#include <ossim/base/ossimConstants.h>
#include <string>
#include <memory>

namespace ATP
{

/**
 * Base class for Top-level classes exposed as JNI'd java classes.
 */
class OSSIM_DLL ServiceBase : public ossim::JsonInterface,
                              public std::enable_shared_from_this<ServiceBase>
{
public:
   ServiceBase() { }

   virtual ~ServiceBase() {}

   virtual void execute() = 0;

};

} // End namespace ATP

#endif

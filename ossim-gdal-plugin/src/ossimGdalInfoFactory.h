//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Info factory for gdal info objects.
// 
//----------------------------------------------------------------------------
// $Id: ossimGdalInfoFactory.h 539 2010-02-23 20:32:45Z ming.su $

#ifndef ossimGdalInfoFactory_HEADER
#define ossimGdalInfoFactory_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoFactoryInterface.h>

class ossimFilename;
class ossimInfoBase;

/**
 * @brief Info factory.
 */
class ossimGdalInfoFactory : public ossimInfoFactoryInterface
{
public:

   /** virtual destructor */
   virtual ~ossimGdalInfoFactory();

   static ossimGdalInfoFactory* instance();

   /**
    * @brief create method.
    *
    * @param file Some file you want info for.
    *
    * @return ossimInfoBase* on success 0 on failure.  Caller is responsible
    * for memory.
    */
   virtual ossimInfoBase* create(const ossimFilename& file) const;
   
private:
   
   /** hidden from use default constructor */
   ossimGdalInfoFactory();

   /** hidden from use copy constructor */
   ossimGdalInfoFactory(const ossimGdalInfoFactory& obj);

   /** hidden from use operator = */
   const ossimGdalInfoFactory& operator=(const ossimGdalInfoFactory& rhs);
};

#endif /* End of "#ifndef ossimGdalInfoFactory_HEADER" */

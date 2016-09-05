//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author: David Burken
//
// Copied from Mingjie Su's ossimHdfInfoFactory
//
// Description: Factory for hdf info objects.
// 
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimH5InfoFactory_HEADER
#define ossimH5InfoFactory_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoFactoryInterface.h>

class ossimFilename;
class ossimInfoBase;

/**
 * @brief Info factory.
 */
class ossimH5InfoFactory : public ossimInfoFactoryInterface
{
public:

   /** virtual destructor */
   virtual ~ossimH5InfoFactory();

   static ossimH5InfoFactory* instance();

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
   ossimH5InfoFactory();

   /** hidden from use copy constructor */
   ossimH5InfoFactory(const ossimH5InfoFactory& obj);

   /** hidden from use operator = */
   const ossimH5InfoFactory& operator=(const ossimH5InfoFactory& rhs);
};

#endif /* End of "#ifndef ossimH5InfoFactory_HEADER" */

//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Factory for Ogr info objects.
// 
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfInfoFactory.h 19869 2011-07-23 13:25:34Z dburken $
#ifndef ossimGeoPdfInfoFactory_HEADER
#define ossimGeoPdfInfoFactory_HEADER

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoFactoryInterface.h>

class ossimFilename;
class ossimInfoBase;

/**
 * @brief Info factory.
 */
class ossimGeoPdfInfoFactory : public ossimInfoFactoryInterface
{
public:

   /** virtual destructor */
   virtual ~ossimGeoPdfInfoFactory();

   static ossimGeoPdfInfoFactory* instance();

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
   ossimGeoPdfInfoFactory();

   /** hidden from use copy constructor */
   ossimGeoPdfInfoFactory(const ossimGeoPdfInfoFactory& obj);

   /** hidden from use operator = */
   const ossimGeoPdfInfoFactory& operator=(const ossimGeoPdfInfoFactory& rhs);
};

#endif /* End of "#ifndef ossimInfoFactory_HEADER" */

//---
//
// License: MIT
// 
// Author: David Burken
//
// Description: Singleton class to hold a common kdu_core::membroker.
// 
//---
// $Id$

#ifndef ossimKakaduMembroker_HEADER
#define ossimKakaduMembroker_HEADER 1

#include <ossim/base/ossimConstants.h>

// Forward declarations:
namespace kdu_core
{
   class kdu_membroker;
}

/**
 * @class ossimKakaduMembroker
 */
class ossimKakaduMembroker
{
public:

   ~ossimKakaduMembroker();

   static ossimKakaduMembroker* instance();

   kdu_core::kdu_membroker* getMembroker() const;
   
private:
   
   /** hidden from use default constructor */
   ossimKakaduMembroker();

   /** hidden from use copy constructor */
   ossimKakaduMembroker(const ossimKakaduMembroker& obj);

   /** hidden from use operator = */
   const ossimKakaduMembroker& operator=(const ossimKakaduMembroker& rhs);

   /** The single instance of this class. */
   static ossimKakaduMembroker* m_instance;

   kdu_core::kdu_membroker* m_membroker;
};

#endif /* End of "#ifndef ossimKakaduMembroker_HEADER" */

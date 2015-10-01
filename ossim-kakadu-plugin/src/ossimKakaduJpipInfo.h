//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
// Description: Does an information dump of any XML JP2 boxes plus basic
//              image information
// 
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimKakduJpipInfo_HEADER
#define ossimKakduJpipInfo_HEADER

#include <iosfwd>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimReferenced.h>
#include "ossimKakaduJpipHandler.h"
class ossimFilename;
class ossimKeywordlist;

/**
 * @brief Info Base.
 *
 * This is the base class for all info objects.  The purpose of an Info object
 * is to dump whatever info is available for a given file name to user.
 */
class ossimKakaduJpipInfo : public ossimInfoBase
{
public:
   
   /** default constructor */
   ossimKakaduJpipInfo();
   
   
   /**
    * @brief open method.
    *
    * @param file File name to open.
    *
    * @return true on success false on error.
    */
   virtual bool open(const ossimFilename& file);
   
   /**
    * Print method.
    *
    * @param out Stream to print to.
    * 
    * @return std::ostream&
    */
   virtual std::ostream& print(std::ostream& out) const;
   
   virtual void setHandler(ossimKakaduJpipHandler* handler);
protected:
   /** virtual destructor */
   virtual ~ossimKakaduJpipInfo();
   mutable ossimRefPtr<ossimKakaduJpipHandler> m_handler;
   
};

#endif /* End of "#ifndef ossimInfoBase_HEADER" */

//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: HDF5 Info class.
// 
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimH5Info_HEADER
#define ossimH5Info_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>
#include "../ossimPluginConstants.h"

class ossimEndian;

/**
 * @brief TIFF info class.
 *
 * Encapsulates the listgeo functionality.
 */
class OSSIM_PLUGINS_DLL ossimH5Info : public ossimInfoBase
{
public:
   
   /** default constructor */
   ossimH5Info();

   /** virtual destructor */
   virtual ~ossimH5Info();

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

private:

   ossimFilename  m_file;
};

#endif /* End of "#ifndef ossimH5Info_HEADER" */

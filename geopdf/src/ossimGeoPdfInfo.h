//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Gdal Ogr Info object.
// 
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfInfo.h 19869 2011-07-23 13:25:34Z dburken $
#ifndef ossimGeoPdfInfo_HEADER
#define ossimGeoPdfInfo_HEADER

#include <iosfwd>
#include <string>
#include <vector>

//PoDoFo includes
#include <podofo/podofo.h>

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>

class ossimKeywordlist;
class PdfMemDocument;
/**
 * @brief GeoPdf info class.
 *
 * Encapsulates the GeoPdf functionality.
 */
class ossimGeoPdfInfo : public ossimInfoBase
{
public:

   /** default constructor */
   ossimGeoPdfInfo();

   /** virtual destructor */
   virtual ~ossimGeoPdfInfo();

   /**
    * @brief open method.
    *
    * @param file File name to open.
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

   bool isOpen();

   private: 
   
     ossimFilename              theFile;
     PoDoFo::PdfMemDocument*    m_PdfMemDocument;
};

#endif /* End of "#ifndef ossimGeoPdfInfo_HEADER" */

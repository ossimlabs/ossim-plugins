//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Info object for HDF data.
// 
//----------------------------------------------------------------------------
// $Id: ossimHdfInfo.h 2645 2011-05-26 15:21:34Z oscar.kramer $
#ifndef ossimHdfInfo_HEADER
#define ossimHdfInfo_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>
#include <ossimHdfReader.h>

/**
 * @brief HDF info class.
 *
 * Encapsulates the HDF functionality.
 */
class ossimHdfInfo : public ossimInfoBase
{
public:
   
   /** default constructor */
   ossimHdfInfo();
   
   /** virtual destructor */
   virtual ~ossimHdfInfo();
   
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
   
private: 
   
   ossimFilename                       theFile;
   ossimRefPtr<ossimHdfReader>         m_hdfReader;
   ossimString                         m_driverName;
   std::map<ossimString, ossimString, ossimStringLtstr>  m_globalMeta;
   std::vector<ossimString>            m_globalMetaVector;
};

#endif /* End of "#ifndef ossimHdfInfo_HEADER" */

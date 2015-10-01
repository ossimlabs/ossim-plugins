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
// $Id: ossimOgrInfo.h 2645 2011-05-26 15:21:34Z oscar.kramer $
#ifndef ossimOgrInfo_HEADER
#define ossimOgrInfo_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>

//OGR Includes
// #include <ogrsf_frmts/ogrsf_frmts.h>
#include <ogrsf_frmts.h>
#include <gdal.h>

#include <iosfwd>
#include <string>
#include <vector>

class ossimKeywordlist;

/**
 * @brief Ogr info class.
 *
 * Encapsulates the Ogr functionality.
 */
class ossimOgrInfo : public ossimInfoBase
{
public:

   /** default constructor */
   ossimOgrInfo();

   /** virtual destructor */
   virtual ~ossimOgrInfo();

   /**
    * @brief open method.
    *
    * @param file File name to open.
    * The example of SDE file name: SDE:server,instance,database,username,password,layername
    * e.g ossim-info -p -d SDE:SPATCDT001,5151,SDE,SADATABASE,SAPASSWORD,SATABLENAME
    *
    * The example of VPF file name: C:/vpfdata/mpp1/vmaplv0/eurnasia/cat
    * e.g ossim-info -p -d D:/OSSIM_Data/vpf_data/WVSPLUS/WVS120M/CAT
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

   virtual bool getKeywordlist(ossimKeywordlist& kwl)const;

   private: 
     ossimString getDriverName(ossimString driverName)const;

     /**
    * Parse the VPF metadata
    *
    */
     void parseMetadata(ossimString metaData, ossimKeywordlist& kwl, ossimString metaPrefix)const;

     ossimFilename  m_file;
     OGRDataSource* m_ogrDatasource;
     OGRSFDriver*   m_ogrDriver;
};

#endif /* End of "#ifndef ossimOgrInfo_HEADER" */

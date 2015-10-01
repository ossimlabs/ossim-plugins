//*******************************************************************
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalTiledDataset.h 14589 2009-05-20 23:51:14Z dburken $
#ifndef ossimGdalTiledDataset_HEADER
#define ossimGdalTiledDataset_HEADER

#include <gdal_priv.h>
#include <memdataset.h>
#include <ossim/vpfutil/set.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimListenerManager.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>

class MEMTiledRasterBand;
class ossimImageSourceSequencer;
class ossimImageData;

class CPL_DLL MEMTiledDataset : public MEMDataset,
   public ossimListenerManager
{
private:
   friend class MEMTiledRasterBand;

   ossimRefPtr<ossimImageData> theData;
   ossimImageSourceSequencer*  theInterface;
   ossimIpt                    theTileSize;
   ossimIrect                  theAreaOfInterest;
   bool                        theJustCreatedFlag;

   /**
    * DRB - 20081020
    * If true (default) the no data value will be set to null pixel
    * value.
    */
   bool theSetNoDataValueFlag;
   
   void create(ossimImageSourceSequencer* iface);

 

public:
   MEMTiledDataset();
   MEMTiledDataset(ossimImageSourceSequencer* iface);
   ~MEMTiledDataset();

   static GDALDataset *Create( const char * pszFilename,
                               int nXSize, int nYSize, int nBands,
                               GDALDataType eType, char ** papszParmList );

   /**
    * DRB - 20081020
    * If true (default) the no data value will be set to null pixel
    * value.
    *
    * The call to rasterBand->SetNoDataValue causes snow with some viewer
    * with some data types.  Like J2K.  This can disable it.
    *
    * @param flag If true the call to rasterBand->SetNoDataValue will be
    * performed.  If false it will be bypassed.
    */
   void setNoDataValueFlag(bool flag);
};

/************************************************************************/
/*                            MEMRasterBand                             */
/************************************************************************/

class CPL_DLL MEMTiledRasterBand : public MEMRasterBand
{
protected:
   
   friend class MEMTiledDataset;
   MEMTiledDataset* theDataset;
   ossimImageSourceSequencer* theInterface;
   
public:

   MEMTiledRasterBand( GDALDataset *poDS,
                       int nBand,
                       GByte *pabyData,
                       GDALDataType eType,
                       int nPixelOffset,
                       int nLineOffset,
                       int bAssumeOwnership );
   
    virtual        ~MEMTiledRasterBand();

    // should override RasterIO eventually.
    
    virtual CPLErr IReadBlock( int, int, void * );
    virtual CPLErr IWriteBlock( int, int, void * );

private:

    /**
     * Copies null values to pImage.
     * @param pImage Buffer to copy to.
     * @param count pixels to null out.
     */
    void copyNulls(void* pImage, int count) const;
    
    /**
     * Copies null values to pImage.
     * @param pImage Buffer to copy to.
     * @param count pixels to null out.
     * @param dummyTemplate Dummy for scalar type.
     */
    template <class T> void copyNulls(void* pImage,
                                      int count,
                                      T dummyTemplate) const;
    
};

#endif

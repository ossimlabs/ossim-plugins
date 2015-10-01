//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: A gdal data set from an ossim image handler.
//
//----------------------------------------------------------------------------
// $Id: ossimGdalDataset.h 15766 2009-10-20 12:37:09Z gpotts $
#ifndef ossimGdalDataset_HEADER
#define ossimGdalDataset_HEADER


#include <gdal_pam.h>
#include <cpl_string.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageHandler.h>

class ossimGdalDatasetRasterBand;
class ossimFilename;

/**
 * @brief ossimGdalDataset This is a gdal data set that wraps an ossim image
 * handler.
 *
 * @note
 * Currently no write implemented.
 */
class ossimGdalDataset : public GDALPamDataset
{
public:

   /** @brief default constructor */
   ossimGdalDataset();

   /** @brief virtual destructor */
   virtual ~ossimGdalDataset();

   /**
    * @brief open method.
    * @param file The file to open.
    * @return true on success, false on error.
    */
   bool open(const ossimFilename& file);

   /**
    * @brief Open for static gdal driver.
    */
   static GDALDataset *Open( GDALOpenInfo * );

   /**
    * @brief Sets theImageHandler.
    * @param ih Pointer to image handler.
    */
   void setImageHandler(ossimImageHandler* ih);

   /** @return Pointer(const) to the image handler or 0 if not set. */
   const ossimImageHandler* getImageHandler() const;

   /** @return Pointer(not const) to the image handler or 0 if not set. */
   ossimImageHandler*       getImageHandler();

   /**
    * @brief Calls gdal's oOvManager.Initialize.  This must be called if
    * you are building overviews or the code will fail.
    */
   void initGdalOverviewManager();

   /**
    * @brief Set the access data member.
    * Either GA_ReadOnly = 0, or GA_Update = 1
    */
   void setGdalAcces(GDALAccess access);

private:
   
   /**
    * Initializes this object from the image handler.
    */
   void init();
   
   friend class ossimGdalDatasetRasterBand;

   ossimRefPtr<ossimImageHandler>  theImageHandler;
};

/**
 * @brief ossimGdalDatasetRasterBand Represents a single band within the
 * image.
 */
class ossimGdalDatasetRasterBand : public GDALPamRasterBand
{
   friend class ossimGdalDataset;
   
public:

   /**
    * @brief Constructor that takes a ossimGdalDataset, band and image handler.
    * @param ds The parent data set.
    * @param band The "ONE" based band.
    * @param ih The pointer to the image handler.
    */
   ossimGdalDatasetRasterBand( ossimGdalDataset* ds,
                               int band,
                               ossimImageHandler* ih);

   /** virtual destructor */
   virtual ~ossimGdalDatasetRasterBand();

   /**
    * This returns 0 right now and should probably be implemented if anything
    * serious is to be done with this data set with the gdal library.
    */
   virtual double GetNoDataValue( int *pbSuccess = 0 );
   
protected:

   /**
    * @brief Read block method.
    * 
    * @param nBlockXOff X Block offset, "0" being upper left sample of image,
    * 1 being sample at nBlockXOff * nBlockXSize and so on.
    * 
    * @param nBlockYOff YBlock offset, "0" being upper left sample of image,
    * 1 being sample at nBlockYOff * nBlockYSize and so on.
    *
    * @param pImage Buffer to put image data in.  Must be at least:
    * pixel_size_in_bytes * nBlockXSize * nBlockYSize
    *
    * @return CE_None on success, CE_Failure on failure.
    */
   virtual CPLErr IReadBlock(int nBlockXOff,
                             int nBlockYOff,
                             void* pImage);

private:
   ossimRefPtr<ossimImageHandler> theImageHandler;
};

#endif /* End of "#ifndef ossimGdalDataset_HEADER" */

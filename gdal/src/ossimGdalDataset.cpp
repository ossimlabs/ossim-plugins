//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: A gdal data set from an ossim image handler.
//
//----------------------------------------------------------------------------
// $Id: ossimGdalDataset.cpp 23003 2014-11-24 17:13:40Z dburken $


#include <fstream>

#include <ossimGdalDataset.h>
#include <ossimGdalType.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>

static GDALDriver *poOssimGdalDriver = 0;

static const ossimTrace traceDebug(ossimString("ossimGdalDataset:debug"));


CPL_C_START
void	GDALRegister_ossimGdalDataset(void);
CPL_C_END

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimGdalDataset.cpp 23003 2014-11-24 17:13:40Z dburken $";
#endif

ossimGdalDataset::ossimGdalDataset()
   : theImageHandler(0)
{
   if (!poOssimGdalDriver)
   {
      GDALRegister_ossimGdalDataset();
      poDriver     = poOssimGdalDriver;
   }
      
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDataset::ossimGdalDataset  entered..."
         << std::endl;
#ifdef OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif       
   }
}

ossimGdalDataset::~ossimGdalDataset()

{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDataset::~ossimGdalDataset "
         << "\n"
         << std::endl;
   }

   if (theImageHandler.valid())
      theImageHandler = 0;
}

bool ossimGdalDataset::open(const ossimFilename& file)
{
   theImageHandler = ossimImageHandlerRegistry::instance()->open(file);
   
   if ( !theImageHandler.valid() )
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGdalDataset::open DEBUG:"
            << "\nCould not open:  " << file.c_str() << std::endl;
      }
      return false;
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDataset::open DEBUG:"
         << "\nOpened:  " << file.c_str() << std::endl;
   }

   init();
   return true;
}


GDALDataset* ossimGdalDataset::Open( GDALOpenInfo * poOpenInfo )

{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDataset::Open  entered..."
         << std::endl;
   }

   ossimFilename f = poOpenInfo->pszFilename;
   GDALDataset* ds = new ossimGdalDataset;
   if ( ((ossimGdalDataset*)ds)->open(f) == true)
   {
      return ds;
   }

   return 0;
}

void ossimGdalDataset::setImageHandler(ossimImageHandler* ih)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDataset::setImageHandler entered..."
         << std::endl;
   }

   theImageHandler = ih;
   init();
}

const ossimImageHandler* ossimGdalDataset::getImageHandler() const
{
   return theImageHandler.get();
}

ossimImageHandler* ossimGdalDataset::getImageHandler()
{
   return theImageHandler.get();
}

void ossimGdalDataset::initGdalOverviewManager()
{
   ossimFilename f = theImageHandler->getFilename();
   sDescription = f.c_str();
   
   if (theImageHandler.valid())
   {
      oOvManager.Initialize( this, f.c_str() );
   }
}

void ossimGdalDataset::init()
{
   nRasterXSize = theImageHandler->getImageRectangle(0).width();
   nRasterYSize = theImageHandler->getImageRectangle(0).height();
   nBands       = theImageHandler->getNumberOfOutputBands();
   eAccess      = GA_ReadOnly;
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGdalDataset::init DEBUG:"
            << "\nWidth:  " << nRasterXSize
            << "\nHeight: " << nRasterYSize
            << "\nBands:  " << nBands << std::endl;
   }
   
   for( int iBand = 0; iBand < nBands; ++iBand )
   {
      ossimGdalDatasetRasterBand* rb =
         new ossimGdalDatasetRasterBand( this,
                                         iBand+1,
                                         theImageHandler.get());
      SetBand( iBand+1, rb );
   }
}

void ossimGdalDataset::setGdalAcces(GDALAccess access)
{
   eAccess = access;
}

ossimGdalDatasetRasterBand::ossimGdalDatasetRasterBand(ossimGdalDataset* ds,
                                                       int band,
                                                       ossimImageHandler* ih)
   : GDALPamRasterBand(),
     theImageHandler(ih)

{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDatasetRasterBand::ossimGdalDatasetRasterBand entered..."
         << "band: " << band
         << std::endl;
   }
   if (!ih)
   {
      return;
   }
   
   poDS     = ds;
   nBand    = band;

   nRasterXSize     = theImageHandler->getImageRectangle(0).width();
   nRasterYSize     = theImageHandler->getImageRectangle(0).height();
   
   // eAccess  = GA_ReadOnly;
   eAccess  = GA_Update;
   
   ossimGdalType gt;
   eDataType = gt.toGdal(theImageHandler->getOutputScalarType());
   
   nBlockXSize      = theImageHandler->getTileWidth();
   nBlockYSize      = theImageHandler->getTileHeight();

    // ESH 06/2009: Prevent divide by zero.
   nBlockXSize      = (nBlockXSize==0) ? 1 : nBlockXSize;
   nBlockYSize      = (nBlockYSize==0) ? 1 : nBlockYSize;
   
   nBlocksPerRow    = nRasterXSize / nBlockXSize;
   nBlocksPerColumn = nRasterYSize / nBlockYSize;
   if (nRasterXSize % nBlockXSize) ++nBlocksPerRow;
   if (nRasterYSize % nBlockYSize) ++nBlocksPerColumn;

   nSubBlocksPerRow = 0;
   nSubBlocksPerColumn = 0;

   bSubBlockingActive = FALSE;
   papoBlocks = 0;

   nBlockReads = 0;
   bForceCachedIO = false;
}

ossimGdalDatasetRasterBand::~ossimGdalDatasetRasterBand()
{
}

CPLErr ossimGdalDatasetRasterBand::IReadBlock(int nBlockXOff,
                                              int nBlockYOff,
                                              void * pImage )
{
   if ( !theImageHandler.valid() || !pImage)
   {
      return CE_Failure;
   }

   ossimIpt startPt(nBlockXOff*nBlockXSize, nBlockYOff*nBlockYSize);
//    ossimIpt endPt( min(startPt.x+nBlockXSize-1, nRasterXSize-1),
//                    min(startPt.y+nBlockYSize-1, nRasterYSize-1));
   ossimIpt endPt( startPt.x+nBlockXSize-1,
                   startPt.y+nBlockYSize-1);
   ossimIrect rect( startPt, endPt);

   ossimRefPtr<ossimImageData> id = theImageHandler->getTile(rect, 0);
   
   if (id.valid())
   {
      if( (id->getDataObjectStatus() == OSSIM_FULL) ||
          (id->getDataObjectStatus() == OSSIM_PARTIAL) )
      {
         id->unloadBand(pImage, rect, nBand-1);
         return CE_None;
      }
   }

   memset(pImage, 0, nBlockXSize * nBlockYSize);      

   return CE_None;
}

double ossimGdalDatasetRasterBand::GetNoDataValue( int * /* pbSuccess */)

{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalDatasetRasterBand::GetNoDataValue entered..."
         << "\n"
         << std::endl;
   }

   return 0.0;
}

void GDALRegister_ossimGdalDataset()

{
   GDALDriver	*poDriver;
   
   if( poOssimGdalDriver == 0 )
   {
      poDriver = new GDALDriver();
      poOssimGdalDriver = poDriver;
      
      poDriver->SetDescription( "ossimGdalDataset" );
      poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                 "ossim data set" );
      poDriver->pfnOpen = ossimGdalDataset::Open;
      GetGDALDriverManager()->RegisterDriver( poDriver );
   }
}

//------------------------------------------------------------------------
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
// $Id: ossimGdalTiledDataset.cpp 19668 2011-05-27 13:26:58Z gpotts $
//------------------------------------------------------------------------
#include "ossimGdalTiledDataset.h"
#include <ossim/vpfutil/set.h>
#include "ossimGdalType.h"

#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/base/ossimProcessProgressEvent.h>
#include <cpl_string.h>

static GDALDriver	*poMEMTiledDriver = NULL;

#include <ossim/base/ossimTrace.h>
static ossimTrace traceDebug(ossimString("ossimGdalTiledDataset:debug"));

GDALRasterBandH MEMTiledCreateRasterBand( GDALDataset *poDS, int nBand, 
                                          GByte *pabyData, GDALDataType eType, 
                                          int nPixelOffset, int nLineOffset, 
                                          int bAssumeOwnership )

{
    return (GDALRasterBandH) 
        new MEMTiledRasterBand( poDS, nBand, pabyData, eType, nPixelOffset, 
                                nLineOffset, bAssumeOwnership );
}

/************************************************************************/
/*                           MEMTiledRasterBand()                            */
/************************************************************************/

MEMTiledRasterBand::MEMTiledRasterBand( GDALDataset *poDS,
                                        int nBand,
                                        GByte *pabyData,
                                        GDALDataType eType,
                                        int nPixelOffset,
                                        int nLineOffset,
                                        int bAssumeOwnership )
   :
   MEMRasterBand(poDS,
                 nBand,
                 pabyData,
                 eType,
                 nPixelOffset,
                 nLineOffset,
                 bAssumeOwnership),
   theDataset(NULL),
   theInterface(NULL)
{
}

/************************************************************************/
/*                           ~MEMTiledRasterBand()                           */
/************************************************************************/

MEMTiledRasterBand::~MEMTiledRasterBand()

{
    CPLDebug( "MEM", "~MEMTiledRasterBand(%p)", this );
    if( bOwnData )
    {
        CPLDebug( "MEM", "~MEMTiledRasterBand() - free raw data." );
        VSIFree( pabyData );
    }
}


/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr MEMTiledRasterBand::IReadBlock( int nBlockXOff,
                                       int nBlockYOff,
                                       void * pImage )

{
#if 0
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "MEMTiledRasterBand::IReadBlock DEBUG: entered..."
         << "\nnBlockXSize:  " << nBlockXSize
         << "\nnBlockYSize:  " << nBlockYSize
         << "\nnBlockXOff:   " << nBlockXSize
         << "\nnBlockYOff:   " << nBlockYSize
         
         << endl;
   }
#endif

   if (!theDataset->theData)  // Check for a valid buffer.
   {
      return CE_None;
   }

   ossimIrect bufferRect = theDataset->theData->getImageRectangle();

   ossimIpt ul(theDataset->theAreaOfInterest.ul().x + nBlockXOff*nBlockXSize,
               theDataset->theAreaOfInterest.ul().y + nBlockYOff*nBlockYSize);

   ossimIrect requestRect(ul.x,
                          ul.y,
                          ul.x + nBlockXSize - 1,
                          ul.y + nBlockYSize - 1);
#if 0
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "\nrequestRect:  " << requestRect
         << "\nbufferRect:   " << bufferRect
         << endl;
   }
#endif

   if(requestRect.height() > 1)
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "MEMTiledRasterBand::IReadBlock WARN!"
         << "\nOnly one scanline block reads allowed" << endl;
      return CE_None;
   }

   if(nBlockYOff==0)
   {
      theDataset->theInterface->setToStartOfSequence();
   }

   bool loadBuffer = false;
   
   if ( (requestRect.completely_within(bufferRect) == false)  ||
        (theDataset->theData->getDataObjectStatus() == OSSIM_EMPTY) )
   {
      loadBuffer = true;
   }

#if 0
   ossim_int32 scanlineTile = ((nBlockYOff*nBlockYSize)%theDataset->theTileSize.y);
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "\nscanlineTile: " << scanlineTile
         << endl;
   }
#endif

   if(loadBuffer)
   {
      theDataset->theData->makeBlank();
      theDataset->theData->setOrigin(requestRect.ul());
      // fill the tile with one row;
      ossim_uint32 numberOfTiles = (ossim_uint32)theDataset->theInterface->getNumberOfTilesHorizontal();
      for(ossim_uint32 i = 0; i < numberOfTiles; ++i)
      {
         ossimIpt tileOrigin(ul.x+theDataset->theTileSize.x*i, ul.y);
#if 0
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "\ntileOrigin:  " << tileOrigin << endl;
         }
#endif
         ossimRefPtr<ossimImageData> data =
            theDataset->theInterface->getTile(
               ossimIrect(tileOrigin.x,
                          tileOrigin.y,
                          tileOrigin.x + (theDataset->theTileSize.x - 1),
                          tileOrigin.y + (theDataset->theTileSize.y - 1)));
         if(data.valid())
         {
            if (data->getBuf())
            {
               theDataset->theData->loadTile(data.get());
            }
            else
            {
               // Hmmm???
               ossimRefPtr<ossimImageData> tempData =
                  (ossimImageData*)data->dup();
               tempData->initialize();
               theDataset->theData->loadTile(tempData.get());
            }
         }
      }
      theDataset->theData->validate();

      // Capture the buffer rectangle.
      bufferRect = theDataset->theData->getImageRectangle();
   }

#if 0
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "nBlockYOff:  " << nBlockYOff
         << "\ntheDataset->theData->getImageRectangle()"
         << theDataset->theData->getImageRectangle()
         << endl;
   }
#endif
   
   // Bytes per pixel.
   const ossim_int32 BPP = static_cast<ossim_int32>(
      theDataset->theData->getScalarSizeInBytes());

   if(theDataset->theData->getDataObjectStatus() == OSSIM_EMPTY)
   {
      copyNulls(pImage, nBlockYSize * nBlockXSize);
   }
   else 
   {
      int nWordSize = GDALGetDataTypeSize( eDataType ) / 8;
      CPLAssert( nBlockXOff == 0 );

      if( nPixelOffset == nWordSize )
      {
         ossim_uint32 offset = (ul.y - bufferRect.ul().y) *
            bufferRect.width() * BPP +
            (ul.x - bufferRect.ul().x) * BPP;

         GByte *pabyCur = ((GByte*) (theDataset->theData->getBuf(nBand-1)))
            + offset;

         memcpy( pImage,
                 pabyCur,
                 nPixelOffset * nBlockXSize);
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "MEMTiledRasterBand::IReadBlock WARN!"
            << "\nUnhandled wordsize..."
            << endl;
#if 0
         // shift to start of scanline
         GByte *pabyCur = (GByte*) (theDataset->theData->getBuf(nBand-1))
                          + (theDataset->theData->getScalarSizeInBytes()*
                             theDataset->theData->getWidth()*scanlineTile);
         
         for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
         {
            memcpy( (GByte *) pImage+ iPixel*nWordSize, 
                    pabyCur + iPixel*nPixelOffset,
                    nWordSize );
         }
#endif
      }
   }

   return CE_None;
}

/************************************************************************/
/*                            IWriteBlock()                             */
/************************************************************************/

CPLErr MEMTiledRasterBand::IWriteBlock( int /*nBlockXOff*/, int /*nBlockYOff*/,
                                     void * /*pImage*/ )

{
//     int     nWordSize = GDALGetDataTypeSize( eDataType );
//     CPLAssert( nBlockXOff == 0 );

//     if( nPixelOffset*8 == nWordSize )
//     {
//         memcpy( pabyData+nLineOffset*nBlockYOff, 
//                 pImage, 
//                 nPixelOffset * nBlockXSize );
//     }
//     else
//     {
//         GByte *pabyCur = pabyData + nLineOffset*nBlockYOff;

//         for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
//         {
//             memcpy( pabyCur + iPixel*nPixelOffset, 
//                     ((GByte *) pImage) + iPixel*nWordSize, 
//                     nWordSize );
//         }
//     }

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*      MEMTiledDataset                                                     */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                            MEMTiledDataset()                             */
/************************************************************************/

MEMTiledDataset::MEMTiledDataset()
   :
   MEMDataset(),
   theData(),
   theInterface(NULL),
   theTileSize(),
   theAreaOfInterest(),
   theJustCreatedFlag(false),
   theSetNoDataValueFlag(true)
{
}

MEMTiledDataset::MEMTiledDataset(ossimImageSourceSequencer* iface)
   :
   MEMDataset(),
   theData(),
   theInterface(iface),
   theTileSize(),
   theAreaOfInterest(),
   theJustCreatedFlag(false),
   theSetNoDataValueFlag(true)
{
   create(theInterface);
}

/************************************************************************/
/*                            ~MEMTiledDataset()                            */
/************************************************************************/

MEMTiledDataset::~MEMTiledDataset()

{
    FlushCache();
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/
void MEMTiledDataset::create(ossimImageSourceSequencer* iface)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "MEMTiledDataset::create DEBUG: entered..."
         << endl;
   }

   theInterface = iface;
   
   if(theInterface)
   {

      ossimKeywordlist kwl;

      GByte  	**papBandData;
      int nBands = theInterface->getNumberOfOutputBands();
      theData = ossimImageDataFactory::instance()->create(NULL, // no owner specified
                                                          theInterface);
      
      GDALDataType gdalType = ossimGdalType().toGdal(theInterface->getOutputScalarType());
      int         nWordSize = GDALGetDataTypeSize(gdalType) / 8;
      int tw = theInterface->getTileWidth();
      int th = theInterface->getTileHeight();
      int band = 0;

      papBandData = (GByte **) CPLCalloc(sizeof(void *),nBands);
      
      for( band = 0; band < nBands; band++ )
      {
         papBandData[band] = (GByte *) VSICalloc( nWordSize, tw * th );
      }
      
      poDriver = poMEMTiledDriver;
      ossimIrect bounds = theInterface->getBoundingRect();

      theTileSize = ossimIpt(theInterface->getTileWidth(),
                             theInterface->getTileHeight());
      
      nRasterXSize = bounds.width();
      nRasterYSize = bounds.height();

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG:"
            << "\nnRasterXSize" << nRasterXSize
            << "\nnRasterYSize" << nRasterYSize
            << endl;
      }

      eAccess      = GA_Update;
      if(theData.valid())
      {
         theData->setWidth(nRasterXSize);
         theData->setHeight(theTileSize.y);
         theData->initialize();
      }

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG:"
            << "\ntheData:  " << *theData.get()
            << endl;
      }


/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */

      for( band = 0; band < nBands; band++ )
      {
       
         MEMTiledRasterBand* rasterBand =
            new MEMTiledRasterBand( this,
                                    band+1,
                                    papBandData[band],
                                    gdalType,
                                    0,
                                    0,
                                    TRUE );
         rasterBand->theDataset = this;
         if (theSetNoDataValueFlag)
         {
            rasterBand->SetNoDataValue(theInterface->getNullPixelValue(band));
         }
         SetBand( band+1, 
                  rasterBand);
      }

      theJustCreatedFlag = true;
      CPLFree( papBandData );

      theAreaOfInterest = theInterface->getBoundingRect();
/* -------------------------------------------------------------------- */
/*      Try to return a regular handle on the file.                     */
/* -------------------------------------------------------------------- */
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "MEMTiledDataset::create DEBUG: exited..."
         << endl;
   }

}

void MEMTiledDataset::setNoDataValueFlag(bool flag)
{
   theSetNoDataValueFlag = flag;
}

/************************************************************************/
/*                          GDALRegister_MEM()                          */
/************************************************************************/

void GDALRegister_MEMTiled()

{
    GDALDriver	*poDriver;

    if( poMEMTiledDriver == NULL )
    {
        poMEMTiledDriver = poDriver = new GDALDriver();
        
        poDriver->SetDescription( "MEM TILED" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "In Memory Raster OSSIM tile bridge" );

        poDriver->pfnOpen   = MEMDataset::Open;
        poDriver->pfnCreate = MEMDataset::Create;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

void MEMTiledRasterBand::copyNulls(void* pImage, int count) const
{
   if (theDataset && pImage)
   {
      if (theDataset->theData.valid())
      {
         switch (theDataset->theData->getScalarType())
         {
            case OSSIM_UINT8:
            {
               return copyNulls(pImage, count, ossim_uint8(0));
            }
            case OSSIM_SINT8:
            {
               return copyNulls(pImage, count, ossim_sint8(0));
            }
            
            case OSSIM_UINT16:
            case OSSIM_USHORT11:
            {
               return copyNulls(pImage, count, ossim_uint16(0));
            }  
            case OSSIM_SINT16:
            {
               return copyNulls(pImage, count, ossim_sint16(0));
            }
            
            case OSSIM_UINT32:
            {
               return copyNulls(pImage, count, ossim_uint32(0));
            }  
            case OSSIM_SINT32:
            {
               return copyNulls(pImage, count, ossim_sint32(0));
            }  
            case OSSIM_FLOAT32:
            case OSSIM_NORMALIZED_FLOAT:
            {
               return copyNulls(pImage, count, ossim_float32(0.0));
            }
            
            case OSSIM_NORMALIZED_DOUBLE:
            case OSSIM_FLOAT64:
            {
               return copyNulls(pImage, count, ossim_float64(0.0));
            }  
            case OSSIM_SCALAR_UNKNOWN:
            default:
            {
               break;
            }
            
         } // End of "switch (theDataset->theData->getScalarType())"
         
      } // End of "if (theDataset->theData.valid())"
      
   } // End of "if (theDataset && pImage)"
}

template <class T>
void MEMTiledRasterBand::copyNulls(void* pImage,
                                   int count,
                                   T /* dummyTemplate */ ) const
{
   // All pointer checking performed by caller.
   
   T* p       = static_cast<T*>(pImage);
   T  nullPix = static_cast<T>(theDataset->theData->getNullPix(0));

   for (int i = 0; i < count; ++i)
   {
      p[i] = nullPix;
   }
}

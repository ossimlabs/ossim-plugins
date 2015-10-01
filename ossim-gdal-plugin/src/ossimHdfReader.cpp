//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  Class definition for HDF reader.
//
//----------------------------------------------------------------------------
// $Id: ossimHdfReader.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

//Std includes
#include <set>

//ossim includes
#include <ossimHdfReader.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimUnitTypeLut.h>
#include <ossim/imaging/ossimImageGeometryRegistry.h>
#include <ossim/imaging/ossimImageDataFactory.h>

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif

static ossimTrace traceDebug("ossimHdfReader:debug");

RTTI_DEF1_INST(ossimHdfReader, "ossimHdfReader", ossimImageHandler)

bool doubleEquals(ossim_float64 left, ossim_float64 right, ossim_float64 epsilon) 
{
  return (fabs(left - right) < epsilon);
}

ossimHdfReader::ossimHdfReader()
  : ossimImageHandler(),
    m_gdalTileSource(0),
    m_entryFileList(),
    m_numberOfBands(0),
    m_scalarType(OSSIM_SCALAR_UNKNOWN),
    m_currentEntryRender(0),
    m_tile(0)
{
}

ossimHdfReader::~ossimHdfReader()
{
   close();
   ossimImageHandler::close();
}

ossimString ossimHdfReader::getShortName()const
{
   return ossimString("ossim_hdf_reader");
}

ossimString ossimHdfReader::getLongName()const
{
   return ossimString("ossim hdf reader");
}

ossimString ossimHdfReader::getClassName()const
{
   return ossimString("ossimHdfReader");
}

ossim_uint32 ossimHdfReader::getNumberOfLines(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if (resLevel == 0)
   {
      if (m_gdalTileSource.valid())
      {
         result = m_gdalTileSource->getNumberOfLines(resLevel);
      }
   }
   else if (theOverview.valid())
   {
      result = theOverview->getNumberOfSamples(resLevel);
   }
   return result;
}

ossim_uint32 ossimHdfReader::getNumberOfSamples(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if (resLevel == 0)
   {
      if (m_gdalTileSource.valid())
      {
         result = m_gdalTileSource->getNumberOfSamples(resLevel);
      }
   }
   else if (theOverview.valid())
   {
      result = theOverview->getNumberOfSamples(resLevel);
   }
   return result;
}

bool ossimHdfReader::open()
{
   static const char MODULE[] = "ossimHdfReader::open";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n"
         << "image: " << theImageFile << "\n";
   }

   bool result = false;

   if (!isSupportedExtension())
   {
      return false;
   }

   if (m_entryFileList.size() == 0)
   {
      if(isOpen())
      {
         close();
      }

      m_gdalTileSource = new ossimGdalTileSource;
      m_gdalTileSource->setFilename(theImageFile);

      if ( m_gdalTileSource->open() == false )
      {
         m_gdalTileSource = 0;
         return false;
      }

      std::vector<ossimString> entryStringList;
      if (m_gdalTileSource != 0)
      {
         m_gdalTileSource->getEntryNames(entryStringList);
      }
      
      // bool isSwathImage = false;
      if (entryStringList.size() > 0)
      {
         for (ossim_uint32 i = 0; i < entryStringList.size(); i++)
         {
            m_entryFileList.push_back(i);
         }
      }
      else
      {
        result = false;
      }
   }
      
   //set entry id for the gdal tile source
   if (m_currentEntryRender < m_entryFileList.size())
   {
      
      m_gdalTileSource->setCurrentEntry(m_currentEntryRender);
      m_numberOfBands = m_gdalTileSource->getNumberOfInputBands();

      m_tile = ossimImageDataFactory::instance()->create(this, this);
      m_tile->initialize();
      
      completeOpen();
      result = true;
   }
   else
   {
      result = false;
   }
   
   if (result == false)
   {
      close();
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }
   
   return result;
}

bool ossimHdfReader::isSDSDataset(ossimString fileName)
{
  std::vector<ossimString> fileList = fileName.split(":");
  if (fileList.size() > 0)
  {
    ossimString  subName = fileList[0];
    if (subName.contains("_SDS"))
    {
      return true;
    }
  }
  return false;
}

bool ossimHdfReader::isOpen()const
{
   return m_gdalTileSource.get();
}

void ossimHdfReader::close()
{
   m_tile = 0;

   if (m_gdalTileSource.valid())
   {
      m_gdalTileSource = 0;
   }
   m_entryFileList.clear();
}

ossimRefPtr<ossimImageData> ossimHdfReader::getTile(const ossimIrect& tile_rect, 
                                                    ossim_uint32 resLevel)
{
   if (m_tile.valid())
   {
      // Image rectangle must be set prior to calling getTile.
      m_tile->setImageRectangle(tile_rect);
      
      if ( getTile( m_tile.get(), resLevel ) == false )
      {
         if (m_tile->getDataObjectStatus() != OSSIM_NULL)
         {
            m_tile->makeBlank();
         }
      }
   }

   return m_tile;
}

bool ossimHdfReader::getTile(ossimImageData* result,
                             ossim_uint32 resLevel)
{
   bool status = false;

   //---
   // Not open, this tile source bypassed, or invalid res level,
   // return a blank tile.
   //---
   if( isOpen() && isSourceEnabled() && isValidRLevel(resLevel) &&
       result && (result->getNumberOfBands() == getNumberOfOutputBands()) )
   {
      result->ref();  // Increment ref count.

      //---
      // Check for overview tile.  Some overviews can contain r0 so always
      // call even if resLevel is 0.  Method returns true on success, false
      // on error.
      //---
      status = getOverviewTile(resLevel, result);

      if (!status) // Did not get an overview tile.
      {
         status = true;

         ossimIrect tile_rect = result->getImageRectangle();     

         if (getImageRectangle().intersects(tile_rect))
         {
            // Make a clip rect.
            ossimIrect clipRect = tile_rect.clipToRect(getImageRectangle());

            if (tile_rect.completely_within(clipRect) == false)
            {
               // Not filling whole tile so blank it out first.
               result->makeBlank();
            }

            if (m_gdalTileSource.valid())
            {
               ossimRefPtr<ossimImageData> imageData =
                  m_gdalTileSource->getTile(tile_rect, resLevel);
               result->loadTile(imageData->getBuf(), tile_rect, clipRect, OSSIM_BSQ);
            }
     
         }
         else // No intersection...
         {
            result->makeBlank();
         }
      }
      result->validate();

      result->unref();  // Decrement ref count.
   }

   return status;
}

bool ossimHdfReader::isSupportedExtension()
{
   ossimString ext = theImageFile.ext();
   ext.downcase();

   if ( ext == "hdf" || ext == "h4" || ext == "hdf4" || 
     ext == "he4" || ext == "hdf5" || ext == "he5" || ext == "h5" || 
     ext == "l1r")
   {
      return true;
   }
   return false;
}

ossim_uint32 ossimHdfReader::getNumberOfInputBands() const
{
   return m_numberOfBands;
}

ossim_uint32 ossimHdfReader::getNumberOfOutputBands()const
{
   return m_numberOfBands;
}

ossim_uint32 ossimHdfReader::getImageTileWidth() const
{
   ossim_uint32 result = 128;
   if (m_gdalTileSource.valid())
   {
      result = m_gdalTileSource->getImageTileWidth();
   }
   return result;
}

ossim_uint32 ossimHdfReader::getImageTileHeight() const
{
   ossim_uint32 result = 128;
   if (m_gdalTileSource.valid())
   {
      result = m_gdalTileSource->getImageTileHeight();
   }
   return result;
}

ossimScalarType ossimHdfReader::getOutputScalarType() const
{
   if (m_gdalTileSource.valid())
   {
      return m_gdalTileSource->getOutputScalarType(); 
   }
   return m_scalarType;
}

ossimRefPtr<ossimImageGeometry> ossimHdfReader::getImageGeometry()
{
   if ( !theGeometry )
   {
      // Check the internal geometry first to avoid a factory call.
      if ( m_gdalTileSource.valid() )
      {
         theGeometry = m_gdalTileSource->getImageGeometry();
      }
   }
   return theGeometry;
}

bool ossimHdfReader::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   bool result = false;
   if ( ossimImageHandler::loadState(kwl, prefix) )
   {
      result = open();
   }
   return result;
}

bool ossimHdfReader::setCurrentEntry(ossim_uint32 entryIdx)
{
   if (m_currentEntryRender == entryIdx)
   {
      return true; // Nothing to do...
   }
   theDecimationFactors.clear();
   theGeometry = 0;
   theOverview = 0;
   theOverviewFile.clear();
   m_currentEntryRender = entryIdx;
   return open();
}

ossim_uint32 ossimHdfReader::getNumberOfEntries()const
{
   return m_entryFileList.size();
}

void ossimHdfReader::getEntryList(std::vector<ossim_uint32>& entryList) const
{
   entryList.clear();
   for (ossim_uint32 i = 0; i < m_entryFileList.size(); i++)
   {
      entryList.push_back(m_entryFileList[i]);
   }
}

ossimString ossimHdfReader::getEntryString(ossim_uint32 entryId) const 
{
   if (m_gdalTileSource.valid())
   {
      std::vector<ossimString> entryStringList;
      m_gdalTileSource->getEntryNames(entryStringList);
      if (entryId < entryStringList.size())
      {
         return entryStringList[entryId];
      }
   }
   return "";
}

ossimString ossimHdfReader::getDriverName()
{
   ossimString result = "";
   if (m_gdalTileSource.valid())
   {
      GDALDriverH driver = m_gdalTileSource->getDriver();
      if (driver)
      {
         result = GDALGetDriverShortName(driver);
      }
   }
   return result;
}

bool ossimHdfReader::setOutputBandList(const vector<ossim_uint32>& outputBandList)
{
   if (outputBandList.size() && m_gdalTileSource != 0)
   {
      m_gdalTileSource->setOutputBandList(outputBandList);
      open();
      return true;
   }
   return false;
}

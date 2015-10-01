//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM MrSid writer class definition.
//
//----------------------------------------------------------------------------
// $Id: ossimMrSidWriter.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

#ifdef OSSIM_ENABLE_MRSID_WRITE

#include <ostream>

#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimEpsgProjectionDatabase.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimImageDataFactory.h>

//LizardTech Includes
#include <lt_base.h>
#include <lt_fileSpec.h>
#include <lt_ioFileStream.h>
#include <lti_imageReader.h>
#include <lti_pixel.h>
#include <lti_scene.h>
#include <lti_sceneBuffer.h>
#include <lt_fileSpec.h>
#include <lti_geoCoord.h>
#include <lt_ioFileStream.h>
#include <lti_pixelLookupTable.h>
#include <lti_utils.h>
#include <lti_metadataDatabase.h>
#include <lti_metadataRecord.h>
#include <lt_utilStatusStrings.h>
#include <ogr_spatialref.h>

#include "ossimMrSidWriter.h"

RTTI_DEF1(ossimMrSidWriter, "ossimMrSidWriter", ossimImageFileWriter)

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimMrSidWriter:debug" your_app_args
//---
static ossimTrace traceDebug("ossimMrSidWriter:debug");

//---
// For the "ident" program which will find all exanded $Id: ossimMrSidWriter.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $
// them.
//---
#if OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimMrSidWriter.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $";
#endif

template <class T>
class LTIDLLPixel : public T
{
public:
  LTIDLLPixel(LTIColorSpace colorSpace, lt_uint16 numBands,
    LTIDataType dataType) : T(colorSpace,numBands,dataType) {}
  virtual ~LTIDLLPixel() {};
};

ossimMrSidWriter::ossimMrSidWriter()
: ossimImageFileWriter(),
m_fileSize(0),
m_makeWorldFile(false)
{
  // Set the output image type in the base class.
  setOutputImageType(getShortName());
}

ossimMrSidWriter::~ossimMrSidWriter()
{
  close();
}

ossimString ossimMrSidWriter::getShortName() const
{
  return ossimString("ossim_mrsid");
}

ossimString ossimMrSidWriter::getLongName() const
{
  return ossimString("ossim mrsid writer");
}

ossimString ossimMrSidWriter::getClassName() const
{
  return ossimString("ossimMrSidWriter");
}

bool ossimMrSidWriter::writeFile()
{
  // This method is called from ossimImageFileWriter::execute().
  bool result = false;

  if( theInputConnection.valid() &&
    (getErrorStatus() == ossimErrorCodes::OSSIM_OK) )
  {
    //---
    // Note only the master process used for writing...
    //---
    if(theInputConnection->isMaster())
    {
      if (!isOpen())
      {
        open();
      }
      if ( isOpen() )
      {
        result = writeStream();
      }
    }
    else // Slave process.
    {
      // This will return after all tiles for this node have been processed.
      theInputConnection->slaveProcessTiles();
      result = true;
    }
  }

  return result;
}

bool ossimMrSidWriter::writeStream()
{
  static const char MODULE[] = "ossimMrSidWriter::writeStream";

  if (traceDebug())
  {
    ossimNotify(ossimNotifyLevel_DEBUG)
      << MODULE << " entered..." << endl;
  }

  //Since the LizardTech algorithms process an entire row at a time, 
  //we need to set the tile width equals to the width of theAreaOfInterest
  //set the tile height to 32, so the writer will write 32 rows at a time
  theInputConnection->setTileSize(theAreaOfInterest.width(), 32);
  theInputConnection->initialize();

  theInputConnection->setAreaOfInterest(theAreaOfInterest);
  theInputConnection->setToStartOfSequence();

  MrSIDDummyImageReader imageReader(theInputConnection);
  if (!LT_SUCCESS(imageReader.initialize()))
  {
    ossimNotify(ossimNotifyLevel_DEBUG)
      << MODULE << ": Failed to open the input image!" << endl;
    return false;
  }
  LTIMetadataDatabase& metadataDatabase = 
    const_cast<LTIMetadataDatabase&>(imageReader.getMetadata());
  writeMetaDatabase(metadataDatabase);

  MG3ImageWriter imageWriter;
  if (!LT_SUCCESS(imageWriter.initialize(&imageReader)))
  {
    return false;
  }

  // set output filename
  if (!LT_SUCCESS(imageWriter.setOutputFileSpec(theFilename)))
  {
    return false;
  }

  LT_STATUS stat = imageWriter.setEncodingApplication(getShortName(), "1.0");

  if ( LT_FAILURE(stat) )
  {
     ossimNotify(ossimNotifyLevel_FATAL)
        << "ossimMrSidWriter::writeMetaDatabase ERROR:"
        << "\nMG3ImageWriter.setEncodingApplication() failed. "
        << getLastStatusString(stat)
        << std::endl;
     return false;
  }

  // Only allow the cartridge check disable when ossimMrSid is compiled for
  // development mode. MRSID_DISABLE_CARTRIDGE_CHECK is defined as preprocessor 
  // in the project with ReleaseWithSymbols
#ifdef MRSID_DISABLE_CARTRIDGE_CHECK
  imageWriter.setUsageMeterEnabled(false);
#else
  // V Important: This enables proper usage of cartridge. Changing the param
  // to false will cause Mr. Sid writer license violation.
  imageWriter.setUsageMeterEnabled(true); 
#endif 

  imageWriter.setStripHeight(32); //set rows offset. it's same as the tile height
  if (!LT_SUCCESS(imageWriter.params().setBlockSize(imageWriter.params().getBlockSize())))
  {
    return false;
  }

  if (m_makeWorldFile)
  {
    imageWriter.setWorldFileSupport(m_makeWorldFile);
  }

  if (m_fileSize > 0)
  {
    imageWriter.params().setTargetFilesize(m_fileSize);
  }

  const LTIScene scene(0, 0, theInputConnection->getAreaOfInterest().width(), 
    theInputConnection->getAreaOfInterest().height(), 1.0);
  if (!LT_SUCCESS(imageWriter.write(scene)))
  {
    ossimNotify(ossimNotifyLevel_FATAL)
      << "ossimMrSidWriter::writeMetaDatabase ERROR:"
      << "\nFailed to write SID file. Is cartridge installed correctly?"
      << std::endl;
    return false;
  }

  return true;
}

bool ossimMrSidWriter::saveState(ossimKeywordlist& kwl,
                                 const char* prefix)const
{
  return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimMrSidWriter::loadState(const ossimKeywordlist& kwl,
                                 const char* prefix)
{
  return ossimImageFileWriter::loadState(kwl, prefix);
}

bool ossimMrSidWriter::isOpen() const
{
  if (theFilename.size() > 0)
  {
    return true;
  }
  return false;
}

bool ossimMrSidWriter::open()
{
  bool result = false;

  // Check for empty filenames.
  if (theFilename.size())
  {
    result = true;
  }

  if (traceDebug())
  {
    ossimNotify(ossimNotifyLevel_DEBUG)
      << "ossimMrSidWriter::open()\n"
      << "File " << theFilename << (result ? " opened" : " not opened")
      << std::endl;
  }

  return result;
}

void ossimMrSidWriter::close()
{
}

void ossimMrSidWriter::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
  imageTypeList.push_back(getShortName());
}

ossimString ossimMrSidWriter::getExtension() const
{
  return ossimString("sid");
}

bool ossimMrSidWriter::hasImageType(const ossimString& imageType) const
{
  bool result = false;
  if ( (imageType == getShortName()) ||
    (imageType == "image/mrsid") )
  {
    result = true;
  }
  return result;
}

void ossimMrSidWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
  if (property.get())
  {
    if(property->getName() == "FILESIZE")
    {
      m_fileSize = property->valueToString().toInt32();
    }

    if(property->getName() == "WORLDFILE")
    {
      if (!property->valueToString().empty())
      {
        m_makeWorldFile = true;
      }
    }

    ossimImageFileWriter::setProperty(property);
  }
}

ossimRefPtr<ossimProperty> ossimMrSidWriter::getProperty(
  const ossimString& name)const
{
  // Currently no properties.
  return ossimImageFileWriter::getProperty(name);
}

void ossimMrSidWriter::getPropertyNames(
                                        std::vector<ossimString>& propertyNames)const
{
  ossimImageFileWriter::getPropertyNames(propertyNames);
}

void ossimMrSidWriter::writeMetaDatabase(LTIMetadataDatabase& metadataDatabase)
{
  // Get current input geometry
  ossimRefPtr<ossimImageGeometry> image_geom = theInputConnection->getImageGeometry();
  if (!image_geom.valid())
  {
    ossimNotify(ossimNotifyLevel_WARN)
      << "ossimMrSidWriter::writeMetaDatabase WARNING:"
      << "\ntheInputConnection->getImageGeometry returned NULL."
      << std::endl;
    return;
  }

  ossimProjection* proj = image_geom->getProjection();
  if (!proj)
  {
    ossimNotify(ossimNotifyLevel_WARN)
      << "ossimMrSidWriter::writeMetaDatabase WARNING:"
      << "\nFailed to create projection!"
      << std::endl;
    return; 
  }

  ossimMapProjection* mapProj = PTR_CAST(ossimMapProjection, proj);
  if (!mapProj)
  {
    ossimNotify(ossimNotifyLevel_WARN)
      << "ossimMrSidWriter::writeMetaDatabase WARNING:"
      << "\nProjection failed to cast to map projection!"
      << std::endl;
    return;  
  }

  ossimUnitType unitType = mapProj->getModelTransformUnitType();
  int epsg_code = (int) mapProj->getPcsCode();

  if (mapProj->getProjectionName() == "ossimEquDistCylProjection" || 
      mapProj->getProjectionName() == "ossimLlxyProjection")
  {
    LTIMetadataRecord mrGeoCSType( "GEOTIFF_NUM::2048::GeographicTypeGeoKey",
      LTI_METADATA_DATATYPE_SINT32, &epsg_code);
    metadataDatabase.add(mrGeoCSType);

    const char* angularUnit = NULL;
    if (unitType == OSSIM_UNIT_UNKNOWN || unitType == OSSIM_DEGREES)
    {
      angularUnit = "Angular_Degree";
    }
    else if (unitType == OSSIM_MINUTES)
    {
      angularUnit = "Angular_Minute";
    }
    else if (unitType == OSSIM_SECONDS)
    {
      angularUnit = "Angular_Second";
    }

    LTIMetadataRecord mrGeogAngularUnitsStr( "GEOTIFF_CHAR::GeogAngularUnitsGeoKey",
      LTI_METADATA_DATATYPE_ASCII, &angularUnit);
    metadataDatabase.add(mrGeogAngularUnitsStr);
  }
  else
  {
    LTIMetadataRecord mrProjectedCSType("GEOTIFF_NUM::3072::ProjectedCSTypeGeoKey",
      LTI_METADATA_DATATYPE_SINT32, &epsg_code);
    metadataDatabase.add(mrProjectedCSType);

    const char* projLinearUnitsStr = NULL;
    if (unitType == OSSIM_UNIT_UNKNOWN || unitType == OSSIM_METERS)
    {
      projLinearUnitsStr = "Linear_Meter";
    }
    else if (unitType == OSSIM_FEET)
    {
      projLinearUnitsStr = "Linear_Foot";
    }
    else if (unitType == OSSIM_US_SURVEY_FEET)
    {
      projLinearUnitsStr = "Linear_Foot_US_Survey";
    }
    LTIMetadataRecord mrProjLinearUnitsStr( "GEOTIFF_CHAR::ProjLinearUnitsGeoKey",
      LTI_METADATA_DATATYPE_ASCII, &projLinearUnitsStr );
    metadataDatabase.add( mrProjLinearUnitsStr );
  }

  const char* GTRasterType = "RasterPixelIsArea";
  LTIMetadataRecord mrGTRasterType( "GEOTIFF_CHAR::GTRasterTypeGeoKey",
    LTI_METADATA_DATATYPE_ASCII, &GTRasterType );
  metadataDatabase.add( mrGTRasterType );
}

MrSIDDummyImageReader::MrSIDDummyImageReader(ossimRefPtr<ossimImageSourceSequencer> theInputConnection) 
: LTIImageReader(), 
theInputSource(theInputConnection)
{
  ltiPixel = NULL;
}

MrSIDDummyImageReader::~MrSIDDummyImageReader()
{
  if ( ltiPixel )
  {
    delete ltiPixel;
  }
}

LT_STATUS MrSIDDummyImageReader::initialize()
{
  if ( !LT_SUCCESS(LTIImageReader::init()) )
    return LT_STS_Failure;

  lt_uint16 numBands = (lt_uint16)theInputSource->getNumberOfOutputBands();
  LTIColorSpace colorSpace = LTI_COLORSPACE_RGB;
  switch ( numBands )
  {
  case 1:
    colorSpace = LTI_COLORSPACE_GRAYSCALE;
    break;
  case 3:
    colorSpace = LTI_COLORSPACE_RGB;
    break;
  default:
    colorSpace = LTI_COLORSPACE_MULTISPECTRAL;
    break;
  }

  ossimScalarType scalarType = theInputSource->getOutputScalarType();

  switch (scalarType)
  {
  case OSSIM_UINT16:
    sampleType = LTI_DATATYPE_UINT16;
    break;
  case OSSIM_SINT16:
    sampleType = LTI_DATATYPE_SINT16;
    break;
  case OSSIM_UINT32:
    sampleType = LTI_DATATYPE_UINT32;
    break;
  case OSSIM_SINT32:
    sampleType = LTI_DATATYPE_SINT32;
    break;
  case OSSIM_FLOAT32:
    sampleType = LTI_DATATYPE_FLOAT32;
    break;
  case OSSIM_FLOAT64:
    sampleType = LTI_DATATYPE_FLOAT64;
    break;
  default:
    sampleType = LTI_DATATYPE_UINT8;
    break;
  }

  ltiPixel = new LTIDLLPixel<LTIPixel>( colorSpace, numBands, sampleType );

  if ( !LT_SUCCESS(setPixelProps(*ltiPixel)) )
    return LT_STS_Failure;

  if ( !LT_SUCCESS(setDimensions(theInputSource->getAreaOfInterest().width()+1, theInputSource->getAreaOfInterest().height()+1)))
  {
    return LT_STS_Failure;
  }

  ossimRefPtr<ossimImageGeometry> image_geom = theInputSource->getImageGeometry();
  if (!image_geom.valid())
  {
    return LT_STS_Failure;
  }

  ossimProjection* proj = image_geom->getProjection();
  if (!proj)
  {
    return LT_STS_Failure; 
  }

  ossimMapProjection* mapProj = dynamic_cast<ossimMapProjection*>(proj);
  if (!mapProj)
  {
    return LT_STS_Failure;  
  }

  double ulx = 0.0;       
  double uly = 0.0; 
  double perPixelX = 0.0;
  double perPixelY = 0.0;

  ossimString type(mapProj->getProjectionName());
  if(type == "ossimLlxyProjection" || type == "ossimEquDistCylProjection")
  {
    perPixelX =  mapProj->getDecimalDegreesPerPixel().x;
    perPixelY = -mapProj->getDecimalDegreesPerPixel().y;
    ulx =  mapProj->getUlGpt().lond();
    uly =  mapProj->getUlGpt().latd();
  }
  else
  {
    perPixelX =  mapProj->getMetersPerPixel().x;
    perPixelY = -mapProj->getMetersPerPixel().y;
    ulx =  mapProj->getUlEastingNorthing().u;
    uly =  mapProj->getUlEastingNorthing().v;
  }

  LTIGeoCoord geoCoord(ulx + fabs(perPixelX)/2, 
    uly - fabs(perPixelY)/2, perPixelX,perPixelY, 0.0, 0.0, NULL);

  ossimString wkt = getWkt(mapProj);
  if (!wkt.empty())
  {
    geoCoord.setWKT(wkt.c_str());
  }

  if ( !LT_SUCCESS(setGeoCoord(geoCoord)) )
  {
    return LT_STS_Failure;  
  }

  setDefaultDynamicRange();

  return LT_STS_Success;
}

ossimString MrSIDDummyImageReader::getWkt(ossimMapProjection* map_proj)
{
  ossimString wktString;
  OGRSpatialReference spatialRef;

  if(map_proj == NULL)
  {
    return wktString;
  }

  ossim_uint32 code = map_proj->getPcsCode();
  if (code != 0)
  {
     spatialRef.importFromEPSG(code);
     
     char* exportString = NULL;
     spatialRef.exportToWkt(&exportString);
     
     if(exportString)
     {
        wktString = exportString;
        delete exportString;
     }
  }

  return wktString;
}

LT_STATUS MrSIDDummyImageReader::decodeStrip(LTISceneBuffer& stripData,
                                             const LTIScene& stripScene)

{
  ossimRefPtr<ossimImageData> imageData = theInputSource->getNextTile();
  if (imageData.valid() && ( imageData->getDataObjectStatus() != OSSIM_NULL ) )
  {
    stripData.importDataBSQ(imageData->getBuf());
  }
  return LT_STS_Success;
}

#endif /* #ifdef OSSIM_ENABLE_MRSID_WRITE */

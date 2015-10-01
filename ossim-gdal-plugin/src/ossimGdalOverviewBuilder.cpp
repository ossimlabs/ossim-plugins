//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file.
//
// Author:  David Burken
//
// Description:  OSSIM wrapper for building gdal overviews (tiff or hfa) from
// an ossim image handler using the GDAL library.
//
//----------------------------------------------------------------------------
// $Id: ossimGdalOverviewBuilder.cpp 15766 2009-10-20 12:37:09Z gpotts $

#include <cmath>
#include <gdal_priv.h>

#include <ossimGdalOverviewBuilder.h>
#include <ossimGdalTiledDataset.h>
#include <ossimGdalDataset.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>

RTTI_DEF1(ossimGdalOverviewBuilder,
          "ossimGdalOverviewBuilder",
          ossimOverviewBuilderBase);

static const char* OVR_TYPE[] = { "unknown",
                                  "gdal_tiff_nearest",
                                  "gdal_tiff_average",
                                  "gdal_hfa_nearest",
                                  "gdal_hfa_average" };

static const ossimTrace traceDebug(
   ossimString("ossimGdalOverviewBuilder:debug"));

ossimGdalOverviewBuilder::ossimGdalOverviewBuilder()
   :
   theDataset(0),
   theOutputFile(),
   theOverviewType(ossimGdalOverviewTiffAverage),
   theLevels(0),
   theGenerateHfaStatsFlag(false)
{
}

ossimGdalOverviewBuilder::~ossimGdalOverviewBuilder()
{
   if ( theDataset )
   {
      delete theDataset;
      theDataset = 0;
   }
}

void ossimGdalOverviewBuilder::setOutputFile(const ossimFilename& file)
{
   theOutputFile = file;
}

ossimFilename ossimGdalOverviewBuilder::getOutputFile() const
{
   if (theOutputFile == ossimFilename::NIL)
   {
      if (theDataset)
      {
         if (theDataset->getImageHandler())
         {
            ossimFilename outputFile =
               theDataset->getImageHandler()->getFilename();

            switch (theOverviewType)
            {
               case ossimGdalOverviewHfaNearest:
               case ossimGdalOverviewHfaAverage:  
                  outputFile.setExtension(getExtensionFromType());
                  break;
               default:
                  outputFile += ".ovr";
                  break;
            }
            return outputFile;
         }
      }
   }
   
   return theOutputFile;
}

bool ossimGdalOverviewBuilder::open(const ossimFilename& file)
{
   if (theDataset)
   {
      delete theDataset;
   }
   theDataset = new ossimGdalDataset;
   return theDataset->open(file);
}

bool ossimGdalOverviewBuilder::setInputSource(ossimImageHandler* imageSource)
{
   if ( !imageSource )
   {
      return false;
   }
   
   if (theDataset)
   {
      delete theDataset;
   }

   theDataset = new ossimGdalDataset();
   theDataset->setImageHandler(imageSource);
   return true;
}

bool ossimGdalOverviewBuilder::setOverviewType(const ossimString& type)
{
   if(type == OVR_TYPE[ossimGdalOverviewTiffNearest])
   {
      theOverviewType = ossimGdalOverviewTiffNearest;
   }
   else if(type == OVR_TYPE[ossimGdalOverviewTiffAverage])
   {
      theOverviewType = ossimGdalOverviewTiffAverage;
   }
   else if(type == OVR_TYPE[ossimGdalOverviewHfaNearest])
   {
      theOverviewType = ossimGdalOverviewHfaNearest;
   }
   else if(type == OVR_TYPE[ossimGdalOverviewHfaAverage])
   {
      theOverviewType = ossimGdalOverviewHfaAverage;
   }
   else
   {
      return false;
   }
   return true;
}

ossimString ossimGdalOverviewBuilder::getOverviewType() const
{
   return ossimString(OVR_TYPE[theOverviewType]);
}

void ossimGdalOverviewBuilder::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(
      ossimString(OVR_TYPE[ossimGdalOverviewTiffNearest]));
   typeList.push_back(
      ossimString(OVR_TYPE[ossimGdalOverviewTiffAverage]));
   typeList.push_back(
      ossimString(OVR_TYPE[ossimGdalOverviewHfaNearest]));
   typeList.push_back(
      ossimString(OVR_TYPE[ossimGdalOverviewHfaAverage]));
}

bool ossimGdalOverviewBuilder::execute()
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalOverviewBuilder::execute entered..."
         << endl;
   }

   bool result = false;
   
   if (!theDataset || !theDataset->getImageHandler())
   {
      return result;
   }

   // Get the output file.
   ossimFilename overviewFile = getOutputFile();

   // Check the file.  Disallow same file overview building.
   if (theDataset->getImageHandler()->getFilename() == overviewFile)
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "Source image file and overview file cannot be the same!"
         << std::endl;
      return result;
   }

   if (theGenerateHfaStatsFlag)
   {
      theDataset->setGdalAcces(GA_Update);
      theDataset->initGdalOverviewManager();
      if (generateHfaStats() == false)
      {
         cerr << " generateHfaStats failed..." << endl;
      }
      else
      {
         // delete theDataset;
         // return true;
      }
   }

   // theDataset->setGdalAcces(GA_Update);

   theDataset->initGdalOverviewManager();
   
   // Get the number of bands.
   // ossim_uint32 bands = theDataset->getImageHandler()->
   // getNumberOfOutputBands();

   // Get the resampling string.
   ossimString pszResampling = getGdalResamplingType();

   // Compute the number of levels.
   ossimIrect bounds = theDataset->getImageHandler()->getBoundingRect();
   ossim_uint32 minBound = ossim::min( bounds.width(), bounds.height() );

   //---
   // Set the decimation levels.  If set through the property interface use
   // that; else compute.
   //---
   ossim_int32 numberOfLevels = 0;

   if (theLevels.size())
   {
      numberOfLevels = theLevels.size();
   }
   else
   {
      ossim_uint32 stopDim = getOverviewStopDimension();
      while (minBound > stopDim)
      {
         minBound = minBound / 2;
         ++numberOfLevels;
      }
      
      if (numberOfLevels == 0)
      {
         return result; // nothing to do.
      }
   }
   
   ossim_int32* levelDecimationFactor = new ossim_int32[numberOfLevels];

   ossim_uint32 idx;

   if (theLevels.size())
   {
      for (idx = 0; idx < theLevels.size(); ++idx)
      {
         levelDecimationFactor[idx] = theLevels[idx];
      }
   }
   else
   {
      levelDecimationFactor[0] = 2;
      for(idx = 1; idx < static_cast<ossim_uint32>(numberOfLevels); ++idx)
      {
         levelDecimationFactor[idx] = levelDecimationFactor[idx-1]*2;
      }
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalOverviewBuilder::execute DEBUG:"
         << "\noverviewFilename:   " << overviewFile
         << "\npszResampling:      " << pszResampling
         << "\nnumberOfLevels:     " << numberOfLevels
         << endl;
      for(idx = 0; idx < static_cast<ossim_uint32>(numberOfLevels); ++idx)
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "levelDecimationFactor["
            << idx << "]: " << levelDecimationFactor[idx]
            << endl;
      }
   }

   CPLErr eErr = CE_None;

   if ( (theOverviewType == ossimGdalOverviewHfaAverage) ||
        (theOverviewType == ossimGdalOverviewHfaNearest) )
   {
      CPLSetConfigOption("USE_RRD", "YES");
   }

   if( theDataset->BuildOverviews( pszResampling.c_str(), 
                                   numberOfLevels,
                                   levelDecimationFactor,
                                   0,
                                   0,
                                   GDALTermProgress,
                                   0 ) != CE_None )
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "Overview building failed." << std::endl;
   }

   if ( levelDecimationFactor )
   {
      delete [] levelDecimationFactor;
      levelDecimationFactor = 0;
   }
   
   if (eErr  == CE_None )
   {
      result = true;
   }

   if (result == true)
   {
      ossimNotify(ossimNotifyLevel_NOTICE)
         << "Wrote file:  " << overviewFile << std::endl;
   }

   return result;
}

void ossimGdalOverviewBuilder::setProperty(ossimRefPtr<ossimProperty> property)
{
   if (property.valid() == false)
   {
      return;
   }

   ossimString s = property->getName();
   s.downcase();
   if ( s == "levels" )
   {
      ossimString value;
      property->valueToString(value);
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGdalOverviewBuilder::setProperty DEBUG:"
            << std::endl;
      }
      theLevels.clear();
      std::vector<ossimString> v1 = value.split(",");
      for (ossim_uint32 i = 0; i < v1.size(); ++i)
      {
         ossim_int32 level = v1[i].toInt32();
         theLevels.push_back(level);
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "level[" << i << "]:  " << level << std::endl;
         }
      }
   }
   else if ( s == "generate-hfa-stats" )
   {
      theGenerateHfaStatsFlag = true;
   }
}

void ossimGdalOverviewBuilder::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(ossimString("levels"));
   propertyNames.push_back(ossimString("generate-hfa-stats"));
}

std::ostream& ossimGdalOverviewBuilder::print(std::ostream& out) const
{
   out << "ossimGdalOverviewBuilder::print"
       << "\nfilename: " << theOutputFile.c_str()
       << "\noverview_type: "
       << OVR_TYPE[theOverviewType]
       << "\nresampling type: " << getGdalResamplingType()
       << std::endl;
   return out;
}

bool ossimGdalOverviewBuilder::generateHfaStats() const
{
   // GDALAllRegister();
   
   //---
   // Equivalent:
   // gdal_translate -of HFA -co AUX=YES -co STATISTICS=YES
   // -co DEPENDENT_FILE=utm.tif utm.tif utm.aux
   //---
   bool result = false;

   if (!theDataset)
   {
      return result;
   }
   if (!theDataset->getImageHandler())
   {
      return result;
   }
   
   ossimFilename sourceImageFile =
      theDataset->getImageHandler()->getFilename();
   if (sourceImageFile.empty())
   {
      return result;
   }

   // This is the output driver.
   GDALDriverH hDriver = GDALGetDriverByName( "HFA" );
   if (!hDriver)
   {
      return false;
   }

   // This is the source data set.
   GDALDatasetH hDataset = theDataset;

   GDALDatasetH	hOutDS   = 0; 
   int bStrict = true;
   
   char** papszCreateOptions = 0;
   GDALProgressFunc pfnProgress = GDALTermProgress;
   
   ossimString s = "DEPENDENT_FILE=";
   s += sourceImageFile.file(); // Must not have absolute path...
   
   papszCreateOptions = CSLAddString( papszCreateOptions, "AUX=YES");
   papszCreateOptions = CSLAddString( papszCreateOptions, "STATISTICS=YES");
   papszCreateOptions = CSLAddString( papszCreateOptions, s.c_str() );

   hOutDS = GDALCreateCopy( hDriver,
                            getOutputFile().c_str(),
                            hDataset, 
                            bStrict,
                            papszCreateOptions, 
                            pfnProgress,
                            0 );

   CSLDestroy( papszCreateOptions );
   if( hOutDS != 0 )
   {
      GDALClose( hOutDS );
   }
   
   return true;
}

ossimString ossimGdalOverviewBuilder::getGdalResamplingType() const
{
   ossimString result;
   switch (theOverviewType)
   {
      case ossimGdalOverviewTiffNearest:
      case ossimGdalOverviewHfaNearest:
         result = "nearest";
         break;
      case ossimGdalOverviewTiffAverage:
      case ossimGdalOverviewHfaAverage:
         result = "average";
         break;
      case ossimGdalOverviewType_UNKNOWN:
         result = "unknown";
         break;
   }
   return result;
}

ossimString ossimGdalOverviewBuilder::getExtensionFromType() const
{
   ossimString result;
   switch (theOverviewType)
   {
      case ossimGdalOverviewHfaNearest:
      case ossimGdalOverviewHfaAverage:  
         result = "aux";
         break;

      default:
         result = "ovr";
         break;
   }
   return result;
}



ossimObject* ossimGdalOverviewBuilder::getObject()
{
   return this;
}

const ossimObject* ossimGdalOverviewBuilder::getObject() const
{
   return this;
}

bool ossimGdalOverviewBuilder::canConnectMyInputTo(
   ossim_int32 index,
   const ossimConnectableObject* obj) const
{
   if ( (index == 0) &&
        PTR_CAST(ossimImageHandler, obj) )
   {
      return true;
   }

   return false;
}

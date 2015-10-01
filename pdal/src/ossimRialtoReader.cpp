#include "ossimRialtoReader.h"
#include <set>
#include <vector>
#include <ossim/base/ossimNotify.h>
#include <pdal/PointViewIter.hpp>
#include <rialto/GeoPackageCommon.hpp>

RTTI_DEF1(ossimRialtoReader, "ossimRialtoReader" , ossimPdalReader)

using namespace pdal;
using namespace rialto;


ossimRialtoReader::ossimRialtoReader()
{
}

ossimRialtoReader::~ossimRialtoReader()
{
   close();
}

bool ossimRialtoReader::open(const ossimFilename& db_name)
{
   m_inputFilename = db_name;
   rewind();

   if (!m_inputFilename.ext().contains("gpkg"))
      return false;
   try
   {
      // Connect to Rialto:
      m_pdalOptions.add("filename", m_inputFilename.string());
      m_pdalPipe = new RialtoReader;
      m_pdalOptions.add("verbose", LogLevel::Debug);
      m_pdalPipe->setOptions(m_pdalOptions);
      m_pointTable = PointTablePtr(new PointTable);
      m_pdalPipe->prepare(*m_pointTable);
   }
   catch (std::exception& e)
   {
      ossimNotify() << e.what() << endl;
      return false;
   }

   m_geometry = new ossimPointCloudGeometry();

   establishMinMax();

   return true;
}

void ossimRialtoReader::getFileBlock(ossim_uint32 /*offset*/,
                                     ossimPointBlock& block,
                                     ossim_uint32 /*np*/) const
{
   // Rialto does not support file-based reads. Only allowed through the Db
   ossimNotify(ossimNotifyLevel_WARN) << "ossimRialtoReader::getFileBlock() WARNING: "
         "Rialto does not support file-based reads. Returning blank point block."<<endl;
   block.clear();
}

void ossimRialtoReader::getBlock(const ossimGrect& bounds, ossimPointBlock& block) const
{
   // getBlock() (unlike getFileBlock) necessarily starts with an empty data buffer. Only the
   // field code is referenced to know which fields to pick out of the PDAL table.
   block.clear();

   // First set up the bounds to be understood by the PDAL rialto reader:
   double minx = bounds.ul().lon;
   double maxx = bounds.lr().lon;
   double miny = bounds.lr().lat;
   double maxy = bounds.ul().lat;
   BOX2D bbox_rect (minx, miny, maxx, maxy);
   std::string bbox_name ("bounds");
   Option pdalOption;
   pdalOption.setName(bbox_name);
   m_pdalOptions.remove(pdalOption);
   m_pdalOptions.add(bbox_name, bbox_rect);
   m_pdalPipe->setOptions(m_pdalOptions);
   m_pointTable = PointTablePtr(new PointTable);
   m_pdalPipe->prepare(*m_pointTable);

   // Fetch the points:
   m_pvs = m_pdalPipe->execute(*m_pointTable);

   // Set up loop over all point view sets:
   m_currentPID = 0;
   PointViewSet::iterator pvs_iter = m_pvs.begin();
   while (pvs_iter != m_pvs.end())
   {
      // Set up loop over points in each PointView:
      m_currentPV = *pvs_iter;
      m_currentPvOffset = 0;
      parsePointView(block);
      ++pvs_iter;
   }
}

void ossimRialtoReader::establishMinMax()
{
   if (!m_pdalPipe)
      return;

   if (!m_minRecord.valid())
   {
      m_minRecord = new ossimPointRecord;
      m_maxRecord = new ossimPointRecord;
   }

   m_availableFields = 0;
   if (!m_pdalPipe)
      return;

   ossimGpt minGpt, maxGpt;
   RialtoReader* reader = (RialtoReader*) m_pdalPipe;
   const GpkgMatrixSet& info = reader->getMatrixSet();
   const std::vector<GpkgDimension>& dimList = info.getDimensions();
   std::vector<GpkgDimension>::const_iterator dim_iter = dimList.begin();

   while (dim_iter != dimList.end())
   {
      double min_value = dim_iter->getMinimum();
      double max_value = dim_iter->getMaximum();
      ossimString dimName = dim_iter->getName();

      if (dimName.contains("X")) // longitude in rialto world
      {
         minGpt.lon = min_value;
         maxGpt.lon = max_value;
      }
      if (dimName.contains("Y")) // latitude in rialto world
      {
         minGpt.lat = min_value;
         maxGpt.lat = max_value;
      }
      if (dimName.contains("Z")) // height (meters) in rialto world
      {
         minGpt.hgt = min_value;
         maxGpt.hgt = max_value;
      }
      if (dimName.contains("Intensity"))
      {
         m_availableFields |= ossimPointRecord::Intensity;
         m_minRecord->setField(ossimPointRecord::Intensity, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::Intensity, (float) max_value);
      }
      else if (dimName.contains("ReturnNumber"))
      {
         m_availableFields |= ossimPointRecord::ReturnNumber;
         m_minRecord->setField(ossimPointRecord::ReturnNumber, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::ReturnNumber, (float) max_value);
      }
      else if (dimName.contains("NumberOfReturns"))
      {
         m_availableFields |= ossimPointRecord::NumberOfReturns;
         m_minRecord->setField(ossimPointRecord::NumberOfReturns, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::NumberOfReturns, (float) max_value);
      }
      else if (dimName.contains("Red"))
      {
         m_availableFields |= ossimPointRecord::Red;
         m_minRecord->setField(ossimPointRecord::Red, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::Red, (float) max_value);
      }
      else if (dimName.contains("Green"))
      {
         m_availableFields |= ossimPointRecord::Green;
         m_minRecord->setField(ossimPointRecord::Green, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::Green, (float) max_value);
      }
      else if (dimName.contains("Blue"))
      {
         m_availableFields |= ossimPointRecord::Blue;
         m_minRecord->setField(ossimPointRecord::Blue, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::Blue, (float) max_value);
      }
      else if (dimName.contains("GpsTime"))
      {
         m_availableFields |= ossimPointRecord::GpsTime;
         m_minRecord->setField(ossimPointRecord::GpsTime, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::GpsTime, (float) max_value);
      }
      else if (dimName.contains("Infrared"))
      {
         m_availableFields |= ossimPointRecord::Infrared;
         m_minRecord->setField(ossimPointRecord::Infrared, (float) min_value);
         m_maxRecord->setField(ossimPointRecord::Infrared, (float) max_value);
      }
      ++dim_iter;
   }

   m_minRecord->setPosition(minGpt);
   m_maxRecord->setPosition(maxGpt);

   // TODO: REMOVE DEBUG
   {
      cout<<"minPt: "<<*m_minRecord<<endl;
      cout<<"maxPt: "<<*m_maxRecord<<endl;
   }
}






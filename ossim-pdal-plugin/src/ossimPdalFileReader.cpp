//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id: ossimPdalFileReader.cpp 23401 2015-06-25 15:00:31Z okramer $

#include "ossimPdalFileReader.h"
#include <ossim/point_cloud/ossimPointCloudGeometry.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <ossim/point_cloud/ossimPointRecord.h>
#include <pdal/pdal.hpp>
#include <pdal/PointViewIter.hpp>
#include <pdal/StatsFilter.hpp>
#include <pdal/Reader.hpp>
#include <pdal/FauxReader.hpp>


RTTI_DEF1(ossimPdalFileReader, "ossimPdalFileReader" , ossimPdalReader)

using namespace pdal;
using namespace pdal::Dimension;

ossimPdalFileReader::ossimPdalFileReader()
{
}

/** virtual destructor */
ossimPdalFileReader::~ossimPdalFileReader()
{
}

bool ossimPdalFileReader::open(const ossimFilename& fname)
{
   // PDAL opens the file here.
   m_inputFilename = fname;
   pdal::Stage* reader = 0;
   try
   {
      if (m_inputFilename.contains("fauxreader"))
      {
         // Debug generated image:
         reader = new FauxReader;
         m_pdalPipe = reader;
         m_pdalOptions.add("mode", "ramp");
         BOX3D bbox(-0.001, -0.001, -100.0, 0.001, 0.001, 100.0);
         m_pdalOptions.add("bounds", bbox);
         m_pdalOptions.add("num_points", "11");
         m_pdalPipe->setOptions(m_pdalOptions);
      }
      else
      {
         StageFactory factory;
         const string driver = factory.inferReaderDriver(m_inputFilename.string());
         if (driver == "")
            throw pdal_error("File type not supported by PDAL");
         reader = factory.createStage(driver);
         if (!reader)
            throw pdal_error("No reader created by PDAL");

         m_pdalOptions.add("filename", m_inputFilename.string());
         reader->setOptions(m_pdalOptions);

         // Stick a stats filter in the pipeline:
         m_pdalPipe = new StatsFilter();
         m_pdalPipe->setInput(*reader);
      }

      m_pointTable = PointTablePtr(new PointTable);
      m_pdalPipe->prepare(*m_pointTable);
      m_pvs = m_pdalPipe->execute(*m_pointTable);
      if (!m_pvs.empty())
      {
         m_currentPV = *(m_pvs.begin());

         // Determine available data fields:
         establishAvailableFields();
      }

      std::string wkt;
      const SpatialReference& srs = reader->getSpatialReference();
      wkt = srs.getWKT(SpatialReference::eCompoundOK, false);
      m_geometry = new ossimPointCloudGeometry(wkt);
   }
   catch (std::exception& e)
   {
      //ossimNotify() << e.what() << endl;
      return false;
   }

   // Establish the bounds (geographic & radiometric). These are stored in two extreme point records
   // maintained by the base class:
   establishMinMax();

   return true;
}

ossim_uint32 ossimPdalFileReader::getNumPoints() const
{
   if (!m_currentPV)
      return 0;

   return m_currentPV->size();
}


void ossimPdalFileReader::getFileBlock(ossim_uint32 offset,
                                       ossimPointBlock& block,
                                       ossim_uint32 requested) const
{
   // A single input file means a single point view coming out of the manager:
   if (!m_currentPV)
   {
      block.clear();
      return;
   }

   ossim_uint32 numPoints = m_currentPV->size();

   // Expecting the block object passed in to have been allocated to the desired block size before
   // this call. Interpreting a size = 0 to mean do nothing:
   if ((requested == 0) || (offset >= numPoints))
   {
      block.clear();
      return;
   }

   m_currentPID = offset;
   parsePointView(block, requested);
}

void ossimPdalFileReader::establishMinMax()
{
   if (!m_pdalPipe || !m_currentPV || !m_geometry.valid())
      return;

#define USE_STATS_FILTER
#ifdef USE_STATS_FILTER

   if (!m_minRecord.valid())
   {
      m_minRecord = new ossimPointRecord(getFieldCode());
      m_maxRecord = new ossimPointRecord(getFieldCode());
   }

   // Attempt to cast down to a stats filter for faster min/max access:
   ossimString filterName(m_pdalPipe->getName());
   if (!filterName.contains("filters.stats"))
   {
      ossimPdalReader::establishMinMax();
      return;
   }
   pdal::StatsFilter* stats = (pdal::StatsFilter*) m_pdalPipe;

   // The ossimPointRecord should already be initialized with the desired fields set to default
   // values (i.e., their NULL values):
   ossimDpt3d minPt, maxPt;
   const IdList& idList = m_currentPV->dims();
   IdList::const_iterator dim_iter = idList.begin();
   while (dim_iter != idList.end())
   {
      Id::Enum id = *dim_iter;
      const stats::Summary& summary = stats->getStats(id);

      switch (id)
      {
      case Id::Enum::X: // always do position
         minPt.x = summary.minimum();
         maxPt.x = summary.maximum();
         break;
      case Id::Enum::Y: // always do position
         minPt.y = summary.minimum();
         maxPt.y = summary.maximum();
         break;
      case Id::Enum::Z: // always do position
         minPt.z = summary.minimum();
         maxPt.z = summary.maximum();
         break;
      case Id::Enum::ReturnNumber:
         if (hasFields(ossimPointRecord::ReturnNumber))
         {
            m_minRecord->setField(ossimPointRecord::ReturnNumber, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::ReturnNumber, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::NumberOfReturns:
         if (hasFields(ossimPointRecord::NumberOfReturns))
         {
            m_minRecord->setField(ossimPointRecord::NumberOfReturns, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::NumberOfReturns, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::Intensity:
         if (hasFields(ossimPointRecord::Intensity))
         {
            m_minRecord->setField(ossimPointRecord::Intensity, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::Intensity, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::Red:
         if (hasFields(ossimPointRecord::Red))
         {
            m_minRecord->setField(ossimPointRecord::Red, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::Red, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::Green:
         if (hasFields(ossimPointRecord::Green))
         {
            m_minRecord->setField(ossimPointRecord::Green, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::Green, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::Blue:
         if (hasFields(ossimPointRecord::Blue))
         {
            m_minRecord->setField(ossimPointRecord::Blue, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::Blue, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::Infrared:
         if (hasFields(ossimPointRecord::Infrared))
         {
            m_minRecord->setField(ossimPointRecord::Infrared, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::Infrared, (ossim_float32) summary.maximum());
         }
         break;
      case Id::Enum::GpsTime:
         if (hasFields(ossimPointRecord::GpsTime))
         {
            m_minRecord->setField(ossimPointRecord::GpsTime, (ossim_float32) summary.minimum());
            m_maxRecord->setField(ossimPointRecord::GpsTime, (ossim_float32) summary.maximum());
         }
         break;
      default:
         break;
      }
      ++dim_iter;
   }

   // Need to convert X, Y, Z to geographic point (if necessary).
   ossimGpt min_gpt, max_gpt;
   m_geometry->convertPos(minPt, min_gpt);
   m_minRecord->setPosition(min_gpt);
   m_geometry->convertPos(maxPt, max_gpt);
   m_maxRecord->setPosition(max_gpt);

#else

   ossimPdalReader::establishMinMax();

#endif

}

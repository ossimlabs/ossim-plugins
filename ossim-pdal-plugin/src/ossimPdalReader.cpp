//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id$

#include "ossimPdalReader.h"
#include <ossim/point_cloud/ossimPointCloudGeometry.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <ossim/point_cloud/ossimPointRecord.h>
#include <pdal/PointViewIter.hpp>

RTTI_DEF1(ossimPdalReader, "ossimPdalReader" , ossimPointCloudHandler)

using namespace pdal;
using namespace pdal::Dimension;

ossimPdalReader::ossimPdalReader()
:  m_currentPV (0),
   m_currentPvOffset (0),
   m_pdalPipe (0),
   m_availableFields(0)
{
   // create unity-transform, geographic geometry. derived classes may overwrite this later
   m_geometry = new ossimPointCloudGeometry();
}

/** virtual destructor */
ossimPdalReader::~ossimPdalReader()
{
   delete m_pdalPipe;
}

void ossimPdalReader::rewind() const
{
   m_currentPvOffset = 0;
   if (m_pvs.size())
      m_currentPV = *(m_pvs.begin());
   else
      m_currentPV = 0;
   m_currentPID = 0;
}

void ossimPdalReader::parsePointView(ossimPointBlock& block, ossim_uint32 maxNumPoints) const
{
   ossim_uint32 field_code = block.getFieldCode();
   while (m_currentPvOffset < m_currentPV->size())
   {
      if (block.size() == maxNumPoints)
         break;

      // Convert point data to OSSIM-friendly format:
#ifdef NORMALIZE_FIELDS
      ossim_float32 minI, delI, minC, delC;
      bool hasIntensity = false;
      bool hasRGB = false;
      if (field_code & ossimPointRecord::Intensity)
      {
         hasIntensity = true;
         minI = m_minRecord->getField(ossimPointRecord::Intensity);
         delI = m_maxRecord->getField(ossimPointRecord::Intensity) - minI;
      }
      ossim_uint32 rgb_code = ossimPointRecord::Red | ossimPointRecord::Green | ossimPointRecord::Blue;
      if ((field_code & rgb_code) == rgb_code)
      {
         hasRGB = true;
         minC = m_minRecord->getField(ossimPointRecord::Red);
         delC = m_maxRecord->getField(ossimPointRecord::Red) - minC;
      }
#endif

      //   cout << "ossimPdalReader::parsePointView() -- point_id = "<< point_id<<endl;

      // Reads directly from the PDAL point buffer:
      ossimRefPtr<ossimPointRecord> opr = new ossimPointRecord(field_code);
      parsePoint(opr.get());

      // Normalize intensity and color:
#ifdef NORMALIZE_FIELDS
      if (hasIntensity)
      {
         float normI = (opr->getField(ossimPointRecord::Intensity) - minI) / delI;
         opr->setField(ossimPointRecord::Intensity, normI);
      }
      if (hasRGB)
      {
         float normC = (opr->getField(ossimPointRecord::Red) - minC) / delC;
         opr->setField(ossimPointRecord::Red, normC);
         normC = (opr->getField(ossimPointRecord::Green) - minC) / delC;
         opr->setField(ossimPointRecord::Green, normC);
         normC = (opr->getField(ossimPointRecord::Blue) - minC) / delC;
         opr->setField(ossimPointRecord::Blue, normC);
      }
#endif

      // Add this point to the output block:
      opr->setPointId(m_currentPID++);
      block.addPoint(opr.get());
      ++m_currentPvOffset;
   }
}

void ossimPdalReader::parsePoint(ossimPointRecord* point) const
{
   if (!point)
      return;

   ossimDpt3d dpt3d;
   ossim_uint8 I8;
   float F32;

   // The ossimPointRecord should already be initialized with the desired fields set to default
   // values (i.e., their NULL values):
   const IdList& idList = m_currentPV->dims();
   IdList::const_iterator dim_iter = idList.begin();
   while (dim_iter != idList.end())
   {
      Id::Enum id = *dim_iter;
      switch (id)
      {
      case Id::Enum::X: // always do position
         dpt3d.x = m_currentPV->getFieldAs<double>(Id::Enum::X, m_currentPvOffset);
         break;
      case Id::Enum::Y: // always do position
         dpt3d.y = m_currentPV->getFieldAs<double>(Id::Enum::Y, m_currentPvOffset);
         break;
      case Id::Enum::Z: // always do position
         dpt3d.z = m_currentPV->getFieldAs<double>(Id::Enum::Z, m_currentPvOffset);
         break;
      case Id::Enum::ReturnNumber:
         if (point->hasFields(ossimPointRecord::ReturnNumber))
         {
            I8 = m_currentPV->getFieldAs<uint8_t>(Id::Enum::ReturnNumber, m_currentPvOffset);
            point->setField(ossimPointRecord::ReturnNumber, (ossim_float32) I8);
         }
         break;
      case Id::Enum::NumberOfReturns:
         if (point->hasFields(ossimPointRecord::NumberOfReturns))
         {
            I8 = m_currentPV->getFieldAs<uint8_t>(Id::Enum::NumberOfReturns, m_currentPvOffset);
            point->setField(ossimPointRecord::NumberOfReturns, (ossim_float32) I8);
         }
         break;
      case Id::Enum::Intensity:
         if (point->hasFields(ossimPointRecord::Intensity))
         {
            F32 = m_currentPV->getFieldAs<float>(Id::Enum::Intensity, m_currentPvOffset);
            point->setField(ossimPointRecord::Intensity, (ossim_float32) F32);
         }
         break;
      case Id::Enum::Red:
         if (point->hasFields(ossimPointRecord::Red))
         {
            F32 = m_currentPV->getFieldAs<float>(Id::Enum::Red, m_currentPvOffset);
            point->setField(ossimPointRecord::Red, (ossim_float32) F32);
         }
         break;
      case Id::Enum::Green:
         if (point->hasFields(ossimPointRecord::Green))
         {
            F32 = m_currentPV->getFieldAs<float>(Id::Enum::Green, m_currentPvOffset);
            point->setField(ossimPointRecord::Green, (ossim_float32) F32);
         }
         break;
      case Id::Enum::Blue:
         if (point->hasFields(ossimPointRecord::Blue))
         {
            F32 = m_currentPV->getFieldAs<float>(Id::Enum::Blue, m_currentPvOffset);
            point->setField(ossimPointRecord::Blue, (ossim_float32) F32);
         }
         break;
      case Id::Enum::Infrared:
         if (point->hasFields(ossimPointRecord::Infrared))
         {
            F32 = m_currentPV->getFieldAs<float>(Id::Enum::Infrared, m_currentPvOffset);
            point->setField(ossimPointRecord::Infrared, (ossim_float32) F32);
         }
         break;
      case Id::Enum::GpsTime:
         if (point->hasFields(ossimPointRecord::GpsTime))
         {
            F32 = m_currentPV->getFieldAs<float>(Id::Enum::GpsTime, m_currentPvOffset);
            point->setField(ossimPointRecord::GpsTime, (ossim_float32) F32);
         }
         break;
      default:
         break;
      }
      ++dim_iter;
   }

   // Need to convert X, Y, Z to geographic point (if necessary).
   ossimGpt pos;
   m_geometry->convertPos(dpt3d, pos);
   point->setPosition(pos);
}

void ossimPdalReader::establishMinMax()
{
   rewind();

   PointViewSet::iterator pvs_iter = m_pvs.begin();
   if ((pvs_iter == m_pvs.end()) || (*pvs_iter)->empty())
      return;

   if (!m_minRecord.valid())
   {
      m_minRecord = new ossimPointRecord(getFieldCode());
      m_maxRecord = new ossimPointRecord(getFieldCode());
   }

   // Latch first point:
   m_currentPV = *pvs_iter;
   parsePoint(m_minRecord.get());
   *m_maxRecord = *m_minRecord;
   ossimGpt minPos (m_minRecord->getPosition());
   ossimGpt maxPos (minPos);

   // Set up loop over all point view sets:
   ossimRefPtr<ossimPointRecord> opr = new ossimPointRecord(m_minRecord->getFieldCode());
   while (pvs_iter != m_pvs.end())
   {
      // Set up loop over points in each PointView:
      m_currentPV = *pvs_iter;
      m_currentPvOffset = 0;

      // Now loop over all points in PV to latch min/max:
      PointViewIter pv_iter = m_currentPV->begin();
      while (pv_iter != m_currentPV->end())
      {
         parsePoint(opr.get());

         if (opr->getPosition().lat < minPos.lat)
            minPos.lat = opr->getPosition().lat;
         if (opr->getPosition().lon < minPos.lon)
            minPos.lon = opr->getPosition().lon;
         if (opr->getPosition().hgt < minPos.hgt)
            minPos.hgt = opr->getPosition().hgt;

         if (opr->getPosition().lat > maxPos.lat)
            maxPos.lat = opr->getPosition().lat;
         if (opr->getPosition().lon > maxPos.lon)
            maxPos.lon = opr->getPosition().lon;
         if (opr->getPosition().hgt > maxPos.hgt)
            maxPos.hgt = opr->getPosition().hgt;

         std::map<ossimPointRecord::FIELD_CODES, ossim_float32>::const_iterator field =
               opr->getFieldMap().begin();
         while (field != opr->getFieldMap().end())
         {
            if (field->second < m_minRecord->getField(field->first))
               m_minRecord->setField(field->first, field->second);
            else if (field->second > m_maxRecord->getField(field->first))
               m_maxRecord->setField(field->first, field->second);
            ++field;
         }
         ++pv_iter;
         ++m_currentPvOffset;
      }
      ++pvs_iter;
   }

   m_minRecord->setPosition(minPos);
   m_maxRecord->setPosition(maxPos);

   // Latch overall min and max color band to avoid color distortion when normalizing:
   const ossimPointRecord::FIELD_CODES R = ossimPointRecord::Red;
   const ossimPointRecord::FIELD_CODES G = ossimPointRecord::Green;
   const ossimPointRecord::FIELD_CODES B = ossimPointRecord::Blue;
   if (m_minRecord->hasFields(R|G|B))
   {
      ossim_float32 r = m_minRecord->getField(R);
      ossim_float32 g = m_minRecord->getField(G);
      ossim_float32 b = m_minRecord->getField(B);
      float c = std::min(r, std::min(g, b));
      m_minRecord->setField(R, c);
      m_minRecord->setField(G, c);
      m_minRecord->setField(B, c);

      r = m_maxRecord->getField(R);
      g = m_maxRecord->getField(G);
      b = m_maxRecord->getField(B);
      c = std::max(r, std::max(g, b));
      m_maxRecord->setField(R, c);
      m_maxRecord->setField(G, c);
      m_maxRecord->setField(B, c);
   }

   rewind();
}

ossim_uint32 ossimPdalReader::getNumPoints() const
{
   ossim_uint32 numPoints = 0;

   PointViewSet::iterator pvs_iter = m_pvs.begin();
   while (pvs_iter != m_pvs.end())
   {
      numPoints += (*pvs_iter)->size();
      ++pvs_iter;
   }

   return numPoints;
}

void ossimPdalReader::establishAvailableFields()
{
   m_availableFields = 0;
   if (!m_currentPV)
      return;

   if (m_currentPV->hasDim(Id::Enum::Intensity))
      m_availableFields |= ossimPointRecord::Intensity;
   if (m_currentPV->hasDim(Id::Enum::ReturnNumber))
      m_availableFields |= ossimPointRecord::ReturnNumber;
   if (m_currentPV->hasDim(Id::Enum::NumberOfReturns))
      m_availableFields |= ossimPointRecord::NumberOfReturns;
   if (m_currentPV->hasDim(Id::Enum::Red))
      m_availableFields |= ossimPointRecord::Red;
   if (m_currentPV->hasDim(Id::Enum::Green))
      m_availableFields |= ossimPointRecord::Green;
   if (m_currentPV->hasDim(Id::Enum::Blue))
      m_availableFields |= ossimPointRecord::Blue;
   if (m_currentPV->hasDim(Id::Enum::GpsTime))
      m_availableFields |= ossimPointRecord::GpsTime;
   if (m_currentPV->hasDim(Id::Enum::Infrared))
      m_availableFields |= ossimPointRecord::Infrared;
}



//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id$

#ifndef ossimPdalReader_HEADER
#define ossimPdalReader_HEADER 1

#include <ossim/point_cloud/ossimPointCloudHandler.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <pdal/pdal.hpp>

class ossimPointRecord;
#define USE_FULL_POINT_CLOUD_BUFFERING

typedef std::shared_ptr<pdal::PointTable> PointTablePtr;

class OSSIM_PLUGINS_DLL ossimPdalReader : public ossimPointCloudHandler
{
public:
   /** default constructor */
   ossimPdalReader();

   /** virtual destructor */
   virtual ~ossimPdalReader();

   virtual void rewind() const;

   /**
    * Returns number of points in the data file.
    */
   virtual ossim_uint32 getNumPoints() const;

   virtual void close() { m_currentPV = 0; m_pointTable = 0; }

   /**
    * Fetches the data fields ids available from this source, OR'd together for testing against
    * specific field (@see ossimPointRecord::FIELD_CODES). e.g. (though should use hasField()),
    *
    * bool hasIntensity = getFields() & ossimPointRecord::Intensity;
    */
   virtual ossim_uint32 getFieldCode() const { return m_availableFields; }

   /**
    * Sets the data fields ID of interest for this source, and all input sources connected to this.
    * This is an OR'd mash-up of @see ossimPointRecord::FIELD_CODES
    */
   virtual void setFieldCode (ossim_uint32 fieldCode) { m_availableFields = fieldCode; }

protected:
   /** The current point ID (pid) gets updated. The block is added to -- pre-existing points remain
    *  or get overwritten if pid matches existing. */
   void parsePointView(ossimPointBlock& block, ossim_uint32 maxNumPoints = 0xFFFFFFFF) const;

   /** Need to pass allocated ossimPointRecord with field code set to desired fields. The pointNum
    * is the point index into the point view */
   void parsePoint(ossimPointRecord* precord) const;

   /** Computes min and max records using points in the current PointViewSet */
   virtual void establishMinMax();

   virtual void establishAvailableFields();

   mutable pdal::PointViewSet m_pvs;
   mutable pdal::PointViewPtr m_currentPV;
   mutable ossim_uint32 m_currentPvOffset;
   mutable PointTablePtr m_pointTable;
   pdal::Stage* m_pdalPipe;
   mutable pdal::Options m_pdalOptions;
   ossim_uint32 m_availableFields;

   TYPE_DATA
};

#endif /* #ifndef ossimPdalReader_HEADER */

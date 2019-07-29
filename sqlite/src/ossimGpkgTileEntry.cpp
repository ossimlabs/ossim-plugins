//----------------------------------------------------------------------------
//
// File: ossimGpkgTileEntry.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for GeoPackage tile entry.
//
// Holds a gpkg_tile_matrix_set and a vector of gpkg_tile_matrix.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgTileEntry.h"

#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimString.h>

#include <ossim/projection/ossimEpsgProjectionFactory.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimGoogleProjection.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimProjection.h>

#include <algorithm> /* for std::sort */
#include <iomanip>
#include <ostream>

static bool tileMatrixSort( const ossimGpkgTileMatrixRecord& i,
                            const ossimGpkgTileMatrixRecord& j )
{
   // This is backwards, want the highest zoom level should be at lowest index.
   return ( i.m_zoom_level > j.m_zoom_level );
}

static bool tileMatrixExtentSort(
   const ossimGpkgNsgTileMatrixExtentRecord& i,
   const ossimGpkgNsgTileMatrixExtentRecord& j )
{
   // This is backwards, want the highest zoom level should be at lowest index.
   return ( i.m_zoom_level > j.m_zoom_level );
}

ossimGpkgTileEntry::ossimGpkgTileEntry()
   :
   m_srs(),
   m_tileMatrixSet(),
   m_tileMatrix(0),
   m_tileMatrixExtents(0)
{
}

ossimGpkgTileEntry::ossimGpkgTileEntry(const ossimGpkgTileEntry& obj)
   :
   m_srs(obj.m_srs),
   m_tileMatrixSet(obj.m_tileMatrixSet),
   m_tileMatrix(obj.m_tileMatrix),
   m_tileMatrixExtents(obj.m_tileMatrixExtents)
{  
}

const ossimGpkgTileEntry& ossimGpkgTileEntry::operator=(const ossimGpkgTileEntry& obj)
{
   if ( this != &obj )
   {
      m_srs = obj.m_srs;
      m_tileMatrixSet = obj.m_tileMatrixSet;
      m_tileMatrix = obj.m_tileMatrix;
      m_tileMatrixExtents = obj.m_tileMatrixExtents;
   }
   return *this;
}

ossimGpkgTileEntry::~ossimGpkgTileEntry()
{
}

void ossimGpkgTileEntry::setTileMatrixSet(const ossimGpkgTileMatrixSetRecord& set)
{
   m_tileMatrixSet = set;
}

const ossimGpkgTileMatrixSetRecord& ossimGpkgTileEntry::getTileMatrixSet() const
{
   return m_tileMatrixSet;
}

void ossimGpkgTileEntry::setSrs( const ossimGpkgSpatialRefSysRecord& srs )
{
   m_srs = srs;
}

const ossimGpkgSpatialRefSysRecord& ossimGpkgTileEntry::getSrs() const
{
   return m_srs;
}

void ossimGpkgTileEntry::addTileMatrix(const ossimGpkgTileMatrixRecord& level)
{
   m_tileMatrix.push_back( level );
}

const std::vector<ossimGpkgTileMatrixRecord>& ossimGpkgTileEntry::getTileMatrix() const
{
   return m_tileMatrix;
}

void ossimGpkgTileEntry::addTileMatrixExtent(const ossimGpkgNsgTileMatrixExtentRecord& record)
{
   m_tileMatrixExtents.push_back( record );
}

const std::vector<ossimGpkgNsgTileMatrixExtentRecord>&
ossimGpkgTileEntry::getTileMatrixExtent() const
{
   return m_tileMatrixExtents;
}

void ossimGpkgTileEntry::sortTileMatrix()
{
   std::sort(m_tileMatrix.begin(), m_tileMatrix.end(), tileMatrixSort);
}

void ossimGpkgTileEntry::sortTileMatrixExtents()
{
   std::sort(m_tileMatrixExtents.begin(),
             m_tileMatrixExtents.end(),
             tileMatrixExtentSort);
}

void ossimGpkgTileEntry::saveState( ossimKeywordlist& kwl,
                                    const std::string& prefix ) const
{
   m_srs.saveState( kwl, prefix );
   m_tileMatrixSet.saveState( kwl, prefix );
   std::string myPrefix = prefix;
   myPrefix += "gpkg_tile_matrix";
   for ( ossim_uint32 i = 0; i < (ossim_uint32)m_tileMatrix.size(); ++i )
   {
      std::string p = myPrefix;
      p += ossimString::toString(i).string();
      p += ".";
      m_tileMatrix[i].saveState( kwl, p );
   }
}

std::ostream& ossimGpkgTileEntry::print(std::ostream& out) const
{
   ossimKeywordlist kwl;
   saveState(kwl, std::string(""));
   out << kwl;
   return out;
}

std::ostream& ossimGpkgTileEntry::printValidate(std::ostream& out) const
{
   m_tileMatrixSet.print( out );
   
   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();  

   // Set the new precision capturing old.
   std::streamsize oldPrecision = out.precision(15);

   ossim_float64 w = m_tileMatrixSet.getWidth();
   ossim_float64 h =  m_tileMatrixSet.getHeight();

   out << std::setiosflags(std::ios::fixed)
       << "gpkg_tile_matrix_set.width:                " << w << "\n"
       << "gpkg_tile_matrix_set.height:               " << h << "\n";
   
      
   for ( ossim_uint32 i = 0; i < (ossim_uint32)m_tileMatrix.size(); ++i )
   {
      ossim_float64 computedX =
         w / m_tileMatrix[i].m_matrix_width / m_tileMatrix[i].m_tile_width;
      ossim_float64 computedY =
         h / m_tileMatrix[i].m_matrix_height / m_tileMatrix[i].m_tile_height;
       
      std::cout << "gpkg_tile_matrix[" << i << "].zoom_level:            "
           << m_tileMatrix[i].m_zoom_level
           << "\ngpkg_tile_matrix[" << i << "].pixel_x_size:          "
           << m_tileMatrix[i].m_pixel_x_size
           << "\ngpkg_tile_matrix[" << i << "].pixel_x_size_computed: "
           << computedX
           << "\ngpkg_tile_matrix[" << i << "].pixel_x_size_delta:    "
           << m_tileMatrix[i].m_pixel_x_size - computedX
           << "\ngpkg_tile_matrix[" << i << "].pixel_y_size:          "
           << m_tileMatrix[i].m_pixel_y_size
           << "\ngpkg_tile_matrix[" << i << "].pixel_y_size_computed: "
           << computedY
           << "\ngpkg_tile_matrix[" << i << "].pixel_y_size_delta:    "
           << m_tileMatrix[i].m_pixel_y_size - computedY
           << "\n";
   }

   // Reset flags and precision.
   out.setf(f);
   out.precision(oldPrecision);

   return out;
}
   
std::ostream& operator<<(std::ostream& out,
                         const ossimGpkgTileEntry& obj)
{
   return obj.print( out );
}

ossim_uint32 ossimGpkgTileEntry::getNumberOfLines( ossim_uint32 resLevel ) const
{
   ossim_uint32 result = 0;

   if ( resLevel < m_tileMatrix.size() )
   {
      // m_tileMatrixExtents may or may not be there.
      if ( ( resLevel < m_tileMatrixExtents.size() ) &&
           ( m_tileMatrixExtents[resLevel].m_zoom_level == m_tileMatrix[resLevel].m_zoom_level ) )
      {
         result = m_tileMatrixExtents[resLevel].m_max_row -
            m_tileMatrixExtents[resLevel].m_min_row + 1;
      }
      else
      {
         ossim_float64 lines =
            ( m_tileMatrixSet.m_max_y - m_tileMatrixSet.m_min_y ) /
            m_tileMatrix[resLevel].m_pixel_y_size;
         if ( lines > 0.0 )
         {
            result = (ossim_uint32)(lines + 0.5);
         }
      }
   }
   
   return result;
}

ossim_uint32 ossimGpkgTileEntry::getNumberOfSamples( ossim_uint32 resLevel ) const
{
   ossim_uint32 result = 0;

   if ( resLevel < m_tileMatrix.size() )
   {
      // m_tileMatrixExtents may or may not be there.
      if ( ( resLevel < m_tileMatrixExtents.size() ) &&
           ( m_tileMatrixExtents[resLevel].m_zoom_level == m_tileMatrix[resLevel].m_zoom_level ) )
      {
         result = m_tileMatrixExtents[resLevel].m_max_column -
            m_tileMatrixExtents[resLevel].m_min_column + 1;
      }
      else
      {    
         ossim_float64 samples =
            ( m_tileMatrixSet.m_max_x - m_tileMatrixSet.m_min_x ) /
            m_tileMatrix[resLevel].m_pixel_x_size;
         if ( samples > 0.0 )
         {
            result = (ossim_uint32)(samples + 0.5);
         }
      }
   }
   
   return result;
}

void ossimGpkgTileEntry::getSubImageOffset( ossim_uint32 resLevel, ossimIpt& offset ) const
{
   // m_tileMatrixExtents may or may not be there.
   if ( ( resLevel < m_tileMatrix.size() ) &&
        ( resLevel < m_tileMatrixExtents.size() ) &&
        ( m_tileMatrixExtents[resLevel].m_zoom_level == m_tileMatrix[resLevel].m_zoom_level ) )
   {
      offset.x = m_tileMatrixExtents[resLevel].m_min_column;
      offset.y = m_tileMatrixExtents[resLevel].m_min_row;
   }
   else
   {
      offset.x = 0;
      offset.y = 0;
   }
}

void ossimGpkgTileEntry::getTiePoint( ossimDpt& tie ) const
{
   // m_tileMatrixExtents may or may not be there.
   if ( m_tileMatrix.size() && m_tileMatrixExtents.size() &&
        ( m_tileMatrixExtents[0].m_zoom_level == m_tileMatrix[0].m_zoom_level ) )
   {
      tie.x = m_tileMatrixExtents[0].m_min_x;
      tie.y = m_tileMatrixExtents[0].m_max_y;
   }
   else
   {
      tie.x = m_tileMatrixSet.m_min_x;
      tie.y = m_tileMatrixSet.m_max_y;
   }
}

void ossimGpkgTileEntry::getGsd( ossim_uint32 index, ossimDpt& gsd ) const
{
   if ( index < m_tileMatrix.size() )
   {
      gsd.x = m_tileMatrix[index].m_pixel_x_size;
      gsd.y = m_tileMatrix[index].m_pixel_y_size;
   }
   else
   {
      gsd.makeNan();
   }
}

void ossimGpkgTileEntry::getZoomLevels( std::vector<ossim_int32>& zoomLevels ) const
{
   zoomLevels.clear();
   std::vector<ossimGpkgTileMatrixRecord>::const_iterator i = m_tileMatrix.begin();
   while ( i != m_tileMatrix.end() )
   {
      zoomLevels.push_back( (*i).m_zoom_level );
      ++i;
   }
}

void ossimGpkgTileEntry::getZoomLevelMatrixSizes(
   std::vector<ossimIpt>& zoomLevelMatrixSizes ) const
{
   zoomLevelMatrixSizes.clear();
   std::vector<ossimGpkgTileMatrixRecord>::const_iterator i = m_tileMatrix.begin();
   while ( i != m_tileMatrix.end() )
   {
      zoomLevelMatrixSizes.push_back( ossimIpt((*i).m_matrix_width, (*i).m_matrix_height) );
      ++i;
   }
}

ossimRefPtr<ossimMapProjection> ossimGpkgTileEntry::getNewMapProjection() const
{
   ossimRefPtr<ossimMapProjection> mapProj = 0;

   if ( m_tileMatrix.size() )
   {
      // Must have code, and scale to continue:
      if ( m_srs.m_organization_coordsys_id &&
           m_tileMatrix[0].m_pixel_x_size &&
           m_tileMatrix[0].m_pixel_y_size )
      {
         std::string org = ossimString(m_srs.m_organization).upcase().string();
         
         if ( org == "EPSG" )
         {
            ossim_uint32 code = (ossim_uint32)m_srs.m_organization_coordsys_id;
            
            ossimDpt gsd;
            getGsd( 0, gsd );
            
            // Avoid factory call for two most common projections.
            if ( code == 4326 )
            {
               // Geographic, WGS 84
               
               //---
               // ossimEquDistCylProjection uses the origin_latitude for meters per pixel
               // (gsd) computation. Compute to achieve the proper horizontal sccaling.
               //---
               ossimGpt origin(0.0, 0.0);
               ossim_float64 tmpDbl = ossim::acosd(gsd.y/gsd.x);
               if ( !ossim::isnan(tmpDbl) )
               {
                  origin.lat = tmpDbl;
               }
               mapProj = new ossimEquDistCylProjection(ossimEllipsoid(), origin);
            }
            else if ( ( code == 3857 ) || ( code == 900913) )
            {
               mapProj = new ossimGoogleProjection();
            }
            else
            {
               ossimString name = "EPSG:";
               name += ossimString::toString(code);
               ossimRefPtr<ossimProjection> proj =
                  ossimEpsgProjectionFactory::instance()->createProjection(name);
               if ( proj.valid() )
               {
                  mapProj = dynamic_cast<ossimMapProjection*>( proj.get() );
               }
            }
            
            if ( mapProj.valid() )
            {
               //---
               // Set the tie and scale.  NOTE the tie is center of the upper left
               // pixel; hence, the half pixel shif.
               //---
               ossimDpt tie;
               getTiePoint( tie );
               
               if ( mapProj->isGeographic() )
               {
                  mapProj->setDecimalDegreesPerPixel(gsd);
                  
                  // Tie latitude, longitude:
                  ossimGpt tiePoint( tie.y, tie.x );
                  ossimDpt half_pixel_shift = gsd * 0.5;
                  tiePoint.lat -= half_pixel_shift.lat;
                  tiePoint.lon += half_pixel_shift.lon;
                  
                  mapProj->setUlTiePoints(tiePoint);
               }
               else
               {
                  mapProj->setMetersPerPixel(gsd);
                  
                  // Tie Easting Northing:
                  ossimDpt half_pixel_shift = gsd * 0.5;
                  tie.y -= half_pixel_shift.y;
                  tie.x += half_pixel_shift.x;
            
                  mapProj->setUlTiePoints(tie);
               }
            }
      
         } // Matches: if ( org == "epsg" )
         
      } // Matches: if ( m_srs.m_organization_coordsys_id && ...
      
   } // Matches: if ( m_tileMatrix.size() )

   return mapProj;
   
} // End: ossimGpkgTileEntry::getNewMapProjection()

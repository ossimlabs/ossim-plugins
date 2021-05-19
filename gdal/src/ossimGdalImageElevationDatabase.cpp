//----------------------------------------------------------------------------
//
// File: ossimGdalImageElevationDatabase.cpp
// 
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Scott Bortman
//
// Description:  See class desciption in header file.
// 
//----------------------------------------------------------------------------
// $Id$

#include <ossimShpElevIndex.h>

#include <ossimGdalImageElevationDatabase.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/elevation/ossimImageElevationHandler.h>
#include <cmath>
#include "ogrsf_frmts.h"


static ossimTrace traceDebug(ossimString("ossimGdalImageElevationDatabase:debug"));

RTTI_DEF1(ossimGdalImageElevationDatabase, "ossimGdalImageElevationDatabase", ossimElevationCellDatabase);

using namespace std;

ossimGdalImageElevationDatabase::ossimGdalImageElevationDatabase()
   :
   ossimElevationCellDatabase(),
   m_poDataSet(NULL),
   m_lastMapKey(0),
   m_lastAccessedId(0)
{
}

// Protected destructor as this is derived from ossimRefenced.
ossimGdalImageElevationDatabase::~ossimGdalImageElevationDatabase()
{
   closeShapefile();
}

bool ossimGdalImageElevationDatabase::open(const ossimString& connectionString)
{
// std:cout << "ossimGdalImageElevationDatabase open: " << connectionString << std::endl;   
   static const char M[] = "ossimGdalImageElevationDatabase::open";

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " entered...\n"
         << "\nConnection string: " << connectionString << "\n";
   }                   
   
   bool result = false;

   close();

   if ( connectionString.size() )
   {
      m_connectionString = connectionString.c_str();

//      if ( openShapefile() )
      if ( ossimShpElevIndex::getInstance(m_connectionString) != NULL )
      {
         result = true;
      }
      else
      {
         m_connectionString.clear();
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " result=" << (result?"true\n":"false\n");
   }

   return result;
}

void ossimGdalImageElevationDatabase::close()
{
   m_meanSpacing = 0.0;
   m_geoid = 0;
   m_connectionString.clear();
}

double ossimGdalImageElevationDatabase::getHeightAboveMSL(const ossimGpt& gpt)
{
// std::cout << "ossimGdalImageElevationDatabase::getHeightAboveMSL" <<  gpt << endl;
   double h = ossim::nan();

   if(isSourceEnabled())
   {
      ossimRefPtr<ossimElevCellHandler> handler = getOrCreateCellHandler(gpt);
      if(handler.valid())
      {
         h = handler->getHeightAboveMSL(gpt); // still need to shift

         // Save the elev source's post spacing as the database's mean spacing:
         m_meanSpacing = handler->getMeanSpacingMeters();
      }
   }

   return h;
}

double ossimGdalImageElevationDatabase::getHeightAboveEllipsoid(const ossimGpt& gpt)
{
   double h = getHeightAboveMSL(gpt);

   if(!ossim::isnan(h))
   {
      h += getOffsetFromEllipsoid(gpt);
   }

   return h;
}

ossimRefPtr<ossimElevCellHandler> ossimGdalImageElevationDatabase::createCell(const ossimGpt& gpt)
{
   // cout << "********************" << std::endl;
   // cout << "Searching for point " << gpt << std::endl;
   // cout << searchShapefile(gpt.lond(), gpt.latd()) << std::endl;
   // cout << "********************" << std::endl;

   ossimRefPtr<ossimElevCellHandler> result = 0;
   
   // Need to disable elevation while loading the DEM image to prevent recursion:
   disableSource();

   //std:string filename = searchShapefile(gpt.lond(), gpt.latd());
   std:string filename = ossimShpElevIndex::getInstance(m_connectionString)->searchShapefile(gpt.lond(), gpt.latd());

   if ( filename.size() > 1 )  
   {
      ossimRefPtr<ossimImageElevationHandler> h = new ossimImageElevationHandler();
      ossimFilename baseDir(m_connectionString);
      ossimFilename fullPath(baseDir.path() + filename );

      if ( h->open( fullPath ) )
      {
         result = h.get();
      } 
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimImageElevationDatabase::createCell WARN:\nCould not open: "
            <<  fullPath << std::endl;
      }
   }

   enableSource();

   return result;
}

ossimRefPtr<ossimElevCellHandler> ossimGdalImageElevationDatabase::getOrCreateCellHandler(
   const ossimGpt& gpt)
{
   ossimRefPtr<ossimElevCellHandler> result = 0;
   
   // Note: Must do mutex lock / unlock around any cach map access.
   m_cacheMapMutex.lock();

   if ( m_cacheMap.size() )
   {
      //---
      // Look in existing map for handler.
      //
      // Note: Cannot key off of id from gpt as cells can be any arbituary dimensions.
      //---

      CellMap::iterator lastAccessedCellIter = m_cacheMap.find(m_lastAccessedId);
      CellMap::iterator iter = lastAccessedCellIter;
        
      // Check from last accessed to end.
      while ( iter != m_cacheMap.end() )
      {
         if ( iter->second->m_handler->pointHasCoverage(gpt) )
         {
            result = iter->second->m_handler.get();
            break;
         }
         ++iter;
      }
     
      if ( result.valid() == false )
      {
         iter = m_cacheMap.begin();
              
         // Beginning to last accessed.
         while ( iter != lastAccessedCellIter)
         {
            if ( iter->second->m_handler->pointHasCoverage(gpt) )
            {
               result = iter->second->m_handler.get();
               break;
            }
            ++iter;
         }
      }

      if ( result.valid() )
      {
         m_lastAccessedId  = iter->second->m_id;
         iter->second->updateTimestamp();
      }
   }
   m_cacheMapMutex.unlock();
  
   if ( result.valid() == false )
   {
      // Not in m_cacheMap.  Create a new cell for point if we have coverage.
      result = createCell(gpt);

      if(result.valid())
      {
         std::lock_guard<std::mutex> lock(m_cacheMapMutex);

         //---
         // Add the cell to map.
         // NOTE: ossimImageElevationDatabase::createCell sets m_lastAccessedId to that of
         // the entries map key.
         //---
         m_cacheMap.insert(std::make_pair(m_lastAccessedId,
                                          new CellInfo(m_lastAccessedId, result.get())));

         ++m_lastMapKey;

         // Check the map size and purge cells if needed.
         if(m_cacheMap.size() > m_maxOpenCells)
         {
            flushCacheToMinOpenCells();
         }
      }
   }

   return result;
}

bool ossimGdalImageElevationDatabase::pointHasCoverage(const ossimGpt& gpt) const
{
   // std::cout << "********************" << std::endl;
   // std::cout << "* pointHasCoverage - Searching for point " << gpt << std::endl;
   // std::cout << "********************" << std::endl;

   //---
   // NOTE:
   //
   // The bounding rect is the North up rectangle.  So if the underlying image projection is not
   // a geographic projection and there is a rotation this could return false positives.  Inside
   // the ossimImageElevationDatabase::createCell there is a call to
   // ossimImageElevationHandler::pointHasCoverage which does a real check from the
   // ossimImageGeometry of the image.
   //---
   bool result = false;

   // std:string filename = searchShapefile(gpt.lond(), gpt.latd());
   std:string filename = ossimShpElevIndex::getInstance(m_connectionString)->searchShapefile(gpt.lond(), gpt.latd());

   result = filename.size() > 1;

   return result;
}


void ossimGdalImageElevationDatabase::getBoundingRect(ossimGrect& rect) const
{
   // The bounding rect is the North up rectangle.  So if the underlying image projection is not
   // a geographic projection and there is a rotation this will include null coverage area.
   // rect.makeNan();

   // OGRLayer  *poLayer = m_poDataSet->GetLayer( 0 );
   // OGREnvelope *poEnvelope = new OGREnvelope();
   // OGRErr status = poLayer->GetExtent(poEnvelope);

   // ossimGrect newRect(poEnvelope->MaxY, poEnvelope->MinX, poEnvelope->MinY, poEnvelope->MaxX);

   // rect.expandToInclude(newRect);

   // delete poEnvelope;
   ossimShpElevIndex::getInstance(m_connectionString)->getBoundingRect(rect);
}


bool ossimGdalImageElevationDatabase::getAccuracyInfo(ossimElevationAccuracyInfo& info, const ossimGpt& gpt) const
{
   if(pointHasCoverage(gpt))
   {
     info.m_surfaceName = "Image Elevation";
   }

   return false;
}

bool ossimGdalImageElevationDatabase::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   static const char M[] = "ossimImageElevationDatabase::loadState";

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " entered..." << "\nkwl:\n" << kwl << "\n";
   }

   bool result = false;
   const char* lookup = kwl.find(prefix, "type");

   if ( lookup )
   {
      std::string type = lookup;
      if ( ( type == "image_directory_shx" ) || ( type == "ossimGdalImageElevationDatabase" ) )
      {
         result = ossimElevationCellDatabase::loadState(kwl, prefix);

         if ( result )
         {
            //openShapefile();
            ossimShpElevIndex::getInstance(m_connectionString);
         }
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " result=" << (result?"true\n":"false\n");
   }

   // std::cout << "ossimGdalImageElevationDatabase::loadState " << result << std::endl;

   return result;
}

bool ossimGdalImageElevationDatabase::saveState(ossimKeywordlist& kwl, const char* prefix) const
{
   return ossimElevationCellDatabase::saveState(kwl, prefix);
}

// Hidden from use:
ossimGdalImageElevationDatabase::ossimGdalImageElevationDatabase(const ossimGdalImageElevationDatabase& copy)
: ossimElevationCellDatabase(copy)
{
   m_lastMapKey = copy.m_lastMapKey;
   m_lastAccessedId = copy.m_lastAccessedId;
}

// Private container class:
ossimGdalImageElevationDatabase::ossimGdalImageElevationFileEntry::ossimGdalImageElevationFileEntry()
   : m_file(),
     m_rect(),
     m_loadedFlag(false)
{
   m_rect.makeNan();
}

// Private container class:
ossimGdalImageElevationDatabase::ossimGdalImageElevationFileEntry::ossimGdalImageElevationFileEntry(
   const ossimFilename& file)
   : m_file(file),
     m_rect(),
     m_loadedFlag(false)
{
   m_rect.makeNan();
}

ossimGdalImageElevationDatabase::ossimGdalImageElevationFileEntry::ossimGdalImageElevationFileEntry
(const ossimGdalImageElevationFileEntry& copy_this)
   : m_file(copy_this.m_file),
     m_rect(copy_this.m_rect),
     m_loadedFlag(copy_this.m_loadedFlag)
{
}

std::ostream& ossimGdalImageElevationDatabase::print(ostream& out) const
{
   ossimKeywordlist kwl;

   saveState(kwl);

   out << "\nossimImageElevationDatabase @ "<< (ossim_uint64) this << "\n"
         << kwl <<ends;

   return out;
}


bool ossimGdalImageElevationDatabase::openShapefile() 
{
   bool status = false;

   if ( m_connectionString.size() > 0 ) 
   {
      m_poDataSet = static_cast<GDALDataset*>(GDALOpenEx( m_connectionString, GDAL_OF_VECTOR, NULL, NULL, NULL ));

      if( m_poDataSet == NULL )
      {
         std:cerr << "ERROR:  Cannot open shapefile: " << m_connectionString.c_str() << std::endl;
      }
      else
      {
         status = true;
      }
   }

   return status;
}

void ossimGdalImageElevationDatabase::closeShapefile()
{
   GDALClose( m_poDataSet );
}

std::string ossimGdalImageElevationDatabase::searchShapefile(double lon, double lat) const
{
   // OGRLayer  *poLayer = m_poDataSet->GetLayerByName( "gegd_dems" );
   OGRLayer  *poLayer = m_poDataSet->GetLayer( 0 );

//   OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();

   OGRFeature *poFeature;
   OGRPoint* ogrPoint = new OGRPoint(lon, lat);

   poLayer->ResetReading();
   poLayer->SetSpatialFilter(ogrPoint);

   std::string filename; 

   while( (poFeature = poLayer->GetNextFeature()) != NULL )
   {
      filename = poFeature->GetFieldAsString(poFeature->GetFieldIndex("filename"));
/*         
      for( int iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
      {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn( iField );

            switch( poFieldDefn->GetType() )
            {
               case OFTInteger:
                  printf( "%d,", poFeature->GetFieldAsInteger( iField ) );
                  break;
               case OFTInteger64:
                  printf( CPL_FRMT_GIB ",", poFeature->GetFieldAsInteger64( iField ) );
                  break;
               case OFTReal:
                  printf( "%.3f,", poFeature->GetFieldAsDouble(iField) );
                  break;
               case OFTString:
                  printf( "%s,", poFeature->GetFieldAsString(iField) );
                  break;
               default:
                  printf( "%s,", poFeature->GetFieldAsString(iField) );
                  break;
            }
      }

      OGRGeometry *poGeometry = poFeature->GetGeometryRef();

      if( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon )
      {
            OGRPolygon *poPolygon = (OGRPolygon *) poGeometry;

            printf( "%s\n", poPolygon->exportToWkt().c_str() );
      }
      else
      {
            printf( "no polygon geometry\n" );
      }
*/         
      OGRFeature::DestroyFeature( poFeature );
   }

   delete ogrPoint;

   return filename;
}
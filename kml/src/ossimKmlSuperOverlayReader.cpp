//*******************************************************************
//
// License:  See top level LICENSE.txt file.
//
// Author:  Mingjie Su, Harsh Govind
//
// Description:
//
// Contains class implementation for the class "ossimKmlSuperOverlayReader".
//
//*******************************************************************
//  $Id: ossimKmlSuperOverlayReader.cpp 2178 2011-02-17 18:38:30Z ming.su $

#include "ossimKmlSuperOverlayReader.h"

//Std Includes
#include <cstring> /* strdup */
#include <iostream>
#include <sstream>
#include <string>

//Minizip Includes
#include <minizip/unzip.h>

//Ossim Includes
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimXmlDocument.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimEquDistCylProjection.h>

RTTI_DEF1(ossimKmlSuperOverlayReader, "ossimKmlSuperOverlayReader", ossimImageHandler);

static ossimTrace traceDebug("ossimKmlSuperOverlayReader:debug");

//*******************************************************************
// Public Constructor:
//*******************************************************************
ossimKmlSuperOverlayReader::ossimKmlSuperOverlayReader()
   :ossimImageHandler(),
   m_xmlDocument(0),
   theImageBound()
{
}

//*******************************************************************
// Destructor:
//*******************************************************************
ossimKmlSuperOverlayReader::~ossimKmlSuperOverlayReader()
{
   close();
}

//*******************************************************************
// Public Method:
//*******************************************************************
bool ossimKmlSuperOverlayReader::open()
{
   // const char* MODULE = "ossimKmlSuperOverlayReader::open";
   bool result = false;
 
   if(isOpen())
   {
      close();
   }

   if (theImageFile.ext().downcase() == "kmz")
   {
      result = getTopLevelKmlFileInfo();
      if (result == false)
      {
         return result;
      }
   }
   else
   {
      m_xmlDocument = new ossimXmlDocument(theImageFile);
   }

   if ( m_xmlDocument.valid() )
   {
      ossimRefPtr<ossimXmlNode> rootNode = m_xmlDocument->getRoot(); 
      if (rootNode.valid())
      {
         std::vector<ossimRefPtr<ossimXmlNode> > nodes = rootNode->getChildNodes();
         result = isKmlSuperOverlayFile(nodes);

         if (result == false) //let gdal kml handle vector kml file
         {
            return result; 
         }

         ossim_float64 north = 0.0;
         ossim_float64 south = 0.0;
         ossim_float64 east = 0.0;
         ossim_float64 west = 0.0;
         result = getCoordinate(nodes, north, south, east, west);
         
         if (result == false)
         {
            return result;
         }

         if (!theImageGeometry.valid())
         {
            theImageGeometry = new ossimImageGeometry(0, createDefaultProj(north, south, east, west));
            if (theImageGeometry.valid())
            {
               result = true;

               theImageBound.makeNan();

               //if the projection is default or geographic, uses the bounding of the OGR Layer
               vector<ossimGpt> points;
               points.push_back(ossimGpt(north, west));
               points.push_back(ossimGpt(north, east));
               points.push_back(ossimGpt(south, east));
               points.push_back(ossimGpt(south, west));


               std::vector<ossimDpt> rectTmp;
               rectTmp.resize(points.size());
               for(std::vector<ossimGpt>::size_type index=0; index < points.size(); ++index)
               {
                  theImageGeometry->worldToLocal(points[(int)index], rectTmp[(int)index]);
               }

               ossimDrect rect = ossimDrect(rectTmp[0],
                  rectTmp[1],
                  rectTmp[2],
                  rectTmp[3]);

               theImageBound = rect;
            }
            else
            {
               result = false;
            }
         }
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = false;
   }

   return result;
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimKmlSuperOverlayReader::saveState(ossimKeywordlist& kwl,
                                         const char* prefix) const
{
   return ossimImageHandler::saveState(kwl, prefix);
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimKmlSuperOverlayReader::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   return ossimImageHandler::loadState(kwl, prefix);
}

ossimRefPtr<ossimImageGeometry> ossimKmlSuperOverlayReader::getInternalImageGeometry() const
{
   return theImageGeometry;
}

void ossimKmlSuperOverlayReader::close()
{
   if (theImageGeometry.valid())
   {
      theImageGeometry = 0;
   }
   if (m_xmlDocument.valid())
   {
      m_xmlDocument = 0;
   }
}

ossimRefPtr<ossimImageData> ossimKmlSuperOverlayReader::getTile(
   const ossimIrect& /* tileRect */, ossim_uint32 /* resLevel */)
{
   return 0;
}

ossim_uint32 ossimKmlSuperOverlayReader::getNumberOfInputBands() const
{
   return 0;
}

ossim_uint32 ossimKmlSuperOverlayReader::getNumberOfOutputBands() const
{
   return 0;
}

ossim_uint32 ossimKmlSuperOverlayReader::getImageTileHeight() const
{
   return 0;
}

ossim_uint32 ossimKmlSuperOverlayReader::getImageTileWidth() const
{
   return 0;
}

ossim_uint32 ossimKmlSuperOverlayReader::getNumberOfLines(ossim_uint32 /* reduced_res_level */ ) const
{
   return theImageBound.height();
}

ossim_uint32 ossimKmlSuperOverlayReader::getNumberOfSamples(ossim_uint32 /* reduced_res_level */ ) const
{ 
   return theImageBound.width();
}

ossimIrect ossimKmlSuperOverlayReader::getImageRectangle(ossim_uint32 /* reduced_res_level */ ) const
{
   return theImageBound;
}

ossimRefPtr<ossimImageGeometry> ossimKmlSuperOverlayReader::getImageGeometry()
{
   return theImageGeometry;
}

ossimScalarType ossimKmlSuperOverlayReader::getOutputScalarType() const
{
   return OSSIM_SCALAR_UNKNOWN;
}

bool ossimKmlSuperOverlayReader::isOpen()const
{
   if (theImageFile.empty())
   {
      return false;
   }
   return true;
}

bool ossimKmlSuperOverlayReader::isKmlSuperOverlayFile(std::vector<ossimRefPtr<ossimXmlNode> > nodes)
{
   for (ossim_uint32 i = 0; i < nodes.size(); i++)
   {
      if (nodes[i]->getTag().downcase() == "placemark") //means vector kml file with point, polygon, or lines
      {
         return false;
      }

      if (nodes[i]->getTag().downcase() == "region" || nodes[i]->getTag().downcase() == "networkLink") //means super overlay kml file
      {
         return true;
      }
   }
   for (ossim_uint32 i = 0; i < nodes.size(); i++)
   {
      return isKmlSuperOverlayFile(nodes[i]->getChildNodes());
   }
   return false;
}

ossimRefPtr<ossimXmlNode> ossimKmlSuperOverlayReader::findLatLonNode(std::vector<ossimRefPtr<ossimXmlNode> >nodes)
{
   ossimRefPtr<ossimXmlNode> latlonNode = 0;
   for (ossim_uint32 i = 0; i < nodes.size(); i++)
   {
      if (nodes[i]->getTag().downcase() == "latlonaltbox")
      {
         return nodes[i];
      }
   }
   for (ossim_uint32 j = 0; j < nodes.size(); j++)
   {
      latlonNode = findLatLonNode(nodes[j]->getChildNodes());
      if (latlonNode.valid())
      {
         return latlonNode;
      }
   }
   return latlonNode;
}

bool ossimKmlSuperOverlayReader::getCoordinate(std::vector<ossimRefPtr<ossimXmlNode> > nodes,
                                               ossim_float64& north,
                                               ossim_float64& south,
                                               ossim_float64& east,
                                               ossim_float64& west)
{
   ossimRefPtr<ossimXmlNode> latlonNode = findLatLonNode(nodes);
   if (!latlonNode.valid())
   {
      return false;
   }

   std::vector<ossimRefPtr<ossimXmlNode> > latlonNodes = latlonNode->getChildNodes();
   if (latlonNodes.size() != 4)
   {
      return false;
   }
   for (ossim_uint32 i = 0; i < latlonNodes.size(); i++)
   {
      ossimString str = latlonNodes[i]->getTag().downcase();
      if (str == "north")
      {
         north = latlonNodes[i]->getText().toFloat64();
      }
      else if (str == "south")
      {
         south = latlonNodes[i]->getText().toFloat64();
      }
      else if (str == "east")
      {
         east = latlonNodes[i]->getText().toFloat64();
      }
      else if (str == "west")
      {
         west = latlonNodes[i]->getText().toFloat64();
      }
   }
   return true;
}

ossimMapProjection* ossimKmlSuperOverlayReader::createDefaultProj(ossim_float64 north, 
                                                                  ossim_float64 south, 
                                                                  ossim_float64 east, 
                                                                  ossim_float64 west)
{
   ossimEquDistCylProjection* proj = new ossimEquDistCylProjection;
   
   ossim_float64 centerLat = (south + north) / 2.0;
   ossim_float64 centerLon = (west + east) / 2.0;
   ossim_float64 deltaLat  = north - south;
   
   // Scale that gives 1024 pixel in the latitude direction.
   ossim_float64 scaleLat = deltaLat / 1024.0;
   ossim_float64 scaleLon = scaleLat*ossim::cosd(std::fabs(centerLat)); 
   ossimGpt origin(centerLat, centerLon, 0.0);
   ossimDpt scale(scaleLon, scaleLat);
   
   // Set the origin.
   proj->setOrigin(origin);
   
  // Set the tie point.
   proj->setUlGpt( ossimGpt(north, west));
   
   // Set the scale.  Note this will handle computing meters per pixel.
   proj->setDecimalDegreesPerPixel(scale);
   
   return proj;
}

bool ossimKmlSuperOverlayReader::getTopLevelKmlFileInfo()
{
   theImageFile.rename(ossimString(theImageFile.noExtension() + ".zip"));
   ossimFilename tmpZipfile = ossimString(theImageFile.noExtension() + ".zip");
   unzFile unzipfile = unzOpen(tmpZipfile.c_str());
   if (!unzipfile)
   {
      ossimSetError(getClassName(),
         ossimErrorCodes::OSSIM_ERROR,
         "Unable to open target zip file..");

      tmpZipfile.rename(ossimString(tmpZipfile.noExtension() + ".kmz"));//if open fails, rename the file back to .kmz
      return false;
   }

   std::string kmzStringBuffer;

   if (unzGoToFirstFile(unzipfile) == UNZ_OK)
   {
      if (unzOpenCurrentFile(unzipfile) == UNZ_OK)
      {
         unsigned char buffer[4096] = {0};
         int bytesRead = 0;
         int read = 1;
         while (read > 0)
         {
            read = unzReadCurrentFile(unzipfile, buffer, 4096);
            bytesRead += read;
         }
         if ( bytesRead )
         {
            for (int i = 0; i < bytesRead; ++i)
            {
               // Copy to string casting to char.
               kmzStringBuffer.push_back( static_cast<char>(buffer[i]) );
            }
         }
      }
      else
      {
         tmpZipfile.rename(ossimString(tmpZipfile.noExtension() + ".kmz"));//if read fails, rename the file back to .kmz
         unzCloseCurrentFile(unzipfile);
         unzClose(unzipfile);
         return false;
      }
   }
   else
   {
      tmpZipfile.rename(ossimString(tmpZipfile.noExtension() + ".kmz"));
      unzClose(unzipfile);
      return false;
   }
   unzCloseCurrentFile(unzipfile);
   unzClose(unzipfile);

   std::istringstream in(kmzStringBuffer);
   if ( in.good() )
   {
      m_xmlDocument = new ossimXmlDocument;
      m_xmlDocument->read( in );
   }
   else
   {
      tmpZipfile.rename(ossimString(tmpZipfile.noExtension() + ".kmz"));
      return false;
   }

   tmpZipfile.rename(ossimString(tmpZipfile.noExtension() + ".kmz"));

   return true;
}




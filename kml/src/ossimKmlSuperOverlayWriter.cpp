//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su, Harsh Govind
//
// Description: OSSIM KmlSuperOverlayWriter writer class definition.
//
//----------------------------------------------------------------------------
// $Id: ossimKmlSuperOverlayWriter.cpp 2178 2011-02-17 18:38:30Z ming.su $

#include "ossimKmlSuperOverlayWriter.h"

//OSSIM Includes
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimVisitor.h> /* for ossimViewInterfaceVisitor */
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>

//STD Includes
#include <cmath>
#include <cstring> /* strdup */
#include <iostream>
#include <sstream>


RTTI_DEF1(ossimKmlSuperOverlayWriter, "ossimKmlSuperOverlayWriter", ossimImageFileWriter)

   //---
   // For trace debugging (to enable at runtime do:
   // your_app -T "ossimKmlSuperOverlayWriter:debug" your_app_args
   //---
   static ossimTrace traceDebug("ossimKmlSuperOverlayWriter:debug");

ossimKmlSuperOverlayWriter::ossimKmlSuperOverlayWriter()
   : ossimImageFileWriter(),
   m_imageWriter(0),
   m_mapProjection(0),
   m_isKmz(false),
   m_isPngFormat(false)
{
   // Set the output image type in the base class.
   setOutputImageType(getShortName());
}

ossimKmlSuperOverlayWriter::~ossimKmlSuperOverlayWriter()
{
   close();
}

ossimString ossimKmlSuperOverlayWriter::getShortName() const
{
   return ossimString("ossim_kmlsuperoverlay");
}

ossimString ossimKmlSuperOverlayWriter::getLongName() const
{
   return ossimString("ossim kmlsuperoverlay writer");
}

ossimString ossimKmlSuperOverlayWriter::getClassName() const
{
   return ossimString("ossimKmlSuperOverlayWriter");
}

bool ossimKmlSuperOverlayWriter::writeFile()
{
   // This method is called from ossimImageFileWriter::execute().
   bool result = false;

   if( theInputConnection.valid() &&
      (getErrorStatus() == ossimErrorCodes::OSSIM_OK) )
   {
      theInputConnection->setAreaOfInterest(theAreaOfInterest);
      theInputConnection->setToStartOfSequence();

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

bool ossimKmlSuperOverlayWriter::writeStream()
{
   static const char MODULE[] = "ossimKmlSuperOverlayWriter::writeStream";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..." << std::endl;
   }

   ossim_int32 bands = theInputConnection->getNumberOfOutputBands();
   if (bands != 1 && bands != 3)
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " Range Error:"
         << "\nInvalid number of input bands!  Must be one or three."
         << "\nInput bands = " << bands
         << "\nReturning from method." << std::endl;

      return false;
   }

   if (theFilename.ext().downcase() == "kmz")
   {
      m_isKmz = true;
   }

   ossimString fileExt = ".jpg";
   ossimString ext = "jpg";
   if (m_isPngFormat)
   {
      fileExt = ".png";
      ext = "png";
   }
   m_imageWriter = ossimImageWriterFactoryRegistry::instance()->createWriterFromExtension(ext);
   if (!m_imageWriter.valid())
   {
      return false;
   }
   if (m_isPngFormat)//add alpha channel for png
   {
      ossimKeywordlist kwl;
      kwl.add("add_alpha_channel", "true", true);
      m_imageWriter->loadState(kwl);
   }
   m_imageWriter->connectMyInputTo(theInputConnection.get());
   
   ossimString outDir = theFilename.path();
   if (outDir.empty())
      outDir = ".";

   if (theAreaOfInterest.hasNans())
   {
      return false;
   }

   ossim_int32 xsize = theAreaOfInterest.width();
   ossim_int32 ysize = theAreaOfInterest.height();

   ossimGpt ulg;
   ossimGpt llg;
   ossimGpt lrg;
   ossimGpt urg;
   ossimDpt scale;
   ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
   ossimRefPtr<ossimMapProjection> projDup = 0; 
   if (geom.valid())
   {
      geom->localToWorld(theAreaOfInterest.ul(), ulg);
      geom->localToWorld(theAreaOfInterest.ll(), llg);
      geom->localToWorld(theAreaOfInterest.lr(), lrg);
      geom->localToWorld(theAreaOfInterest.ur(), urg);

      ossimRefPtr<ossimProjection> proj = geom->getProjection();
      m_mapProjection = PTR_CAST(ossimMapProjection, proj.get());
      if (m_mapProjection.valid())
      {
         //make a copy of map projection for lat, lon conversion of the previous zoom in child kml since m_mapProjection set the scale 
         //for the current zoom
         projDup = PTR_CAST(ossimMapProjection, m_mapProjection->dup());

         if (m_mapProjection->isGeographic())
         {
            // Get the scale.
            scale.x =  m_mapProjection->getDecimalDegreesPerPixel().x;
            scale.y =  m_mapProjection->getDecimalDegreesPerPixel().y;
         }
         else
         {
            // Get the scale.
            scale.x =  m_mapProjection->getMetersPerPixel().x;
            scale.y =  m_mapProjection->getMetersPerPixel().y;
         }
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }

   //Zoom levels of the pyramid.
   ossim_int32 maxzoom;
   ossim_int32 tilexsize;
   ossim_int32 tileysize;
   // Let the longer side determine the max zoom level and x/y tilesizes.
   if ( xsize >= ysize )
   {
      ossim_float64 dtilexsize = xsize;
      while (dtilexsize > 400) //calculate x tile size
      {
         dtilexsize = dtilexsize/2;
      }

      maxzoom   = static_cast<ossim_int32>(log( (ossim_float64)xsize / dtilexsize ) / log(2.0));
      tilexsize = (ossim_int32)dtilexsize;
      tileysize = (ossim_int32)( (ossim_float64)(dtilexsize * ysize) / xsize );
   }
   else
   {
      ossim_float64 dtileysize = ysize;
      while (dtileysize > 400) //calculate y tile size
      {
         dtileysize = dtileysize/2;
      }

      maxzoom   = static_cast<ossim_int32>(log( (ossim_float64)ysize / dtileysize ) / log(2.0));
      tileysize = (ossim_int32)dtileysize;
      tilexsize = (ossim_int32)( (ossim_float64)(dtileysize * xsize) / ysize );
   }

   std::vector<ossim_float64> zoomScaleX;
   std::vector<ossim_float64> zoomScaleY;
   for (int zoom = 0; zoom < maxzoom + 1; zoom++)
   {
      zoomScaleX.push_back(scale.x * pow(2.0, (maxzoom - zoom)));
      zoomScaleY.push_back(scale.y * pow(2.0, (maxzoom - zoom)));
   }

   ossimString tmpFileName; 
   std::vector<ossimString> dirVector;
   std::vector<ossimString> fileVector;
   bool isKmlGenerated = false;
   if (m_isKmz)
   {
      tmpFileName = ossimString(outDir + "/" + "tmp.kml");
      isKmlGenerated = generateRootKml(tmpFileName.c_str(), ulg.lat, lrg.lat, lrg.lon, ulg.lon, (ossim_int32)tilexsize);
      fileVector.push_back(tmpFileName);
   }
   else
   {
      isKmlGenerated = generateRootKml(theFilename, ulg.lat, lrg.lat, lrg.lon, ulg.lon, (ossim_int32)tilexsize);
   }
   if (!isKmlGenerated)
   {
      return false;
   }

   for (ossim_int32 zoom = maxzoom; zoom >= 0; --zoom)
   {
      ossim_int32 rmaxxsize = static_cast<ossim_int32>(pow(2.0, (maxzoom-zoom)) * tilexsize);
      ossim_int32 rmaxysize = static_cast<ossim_int32>(pow(2.0, (maxzoom-zoom)) * tileysize);

      ossim_int32 xloop = (ossim_int32)xsize/rmaxxsize;
      ossim_int32 yloop = (ossim_int32)ysize/rmaxysize;

      xloop = xloop>0 ? xloop : 1;
      yloop = yloop>0 ? yloop : 1;

      //set scale for the current zoom
      if (m_mapProjection.valid())
      {
         if (m_mapProjection->isGeographic())
         {
            m_mapProjection->setDecimalDegreesPerPixel(ossimDpt(zoomScaleX[zoom],zoomScaleY[zoom]));
         }
         else
         {
            m_mapProjection->setMetersPerPixel(ossimDpt(zoomScaleX[zoom],zoomScaleY[zoom]));
         }
      }

      // Set all views and send property event out for combiner if needed.
      propagateViewChange();

      //set the scale for the previous zoom in child kml for lat, lon conversion
      if (zoom < maxzoom)
      {
         if (projDup.valid())
         {
            if (projDup->isGeographic())
            {
               projDup->setDecimalDegreesPerPixel(ossimDpt(zoomScaleX[zoom+1],zoomScaleY[zoom+1]));
            }
            else
            {
               projDup->setMetersPerPixel(ossimDpt(zoomScaleX[zoom+1],zoomScaleY[zoom+1]));
            }
         }
      }

      for (ossim_int32 ix = 0; ix < xloop; ix++)
      {
         ossim_int32 rxsize = (ossim_int32)(rmaxxsize);
         // ossim_int32 rx = (ossim_int32)(ix * rmaxxsize);

         for (ossim_int32 iy = 0; iy < yloop; iy++)
         {
            ossim_int32 rysize = (ossim_int32)(rmaxysize);
            // ossim_int32 ry = (ossim_int32)(ysize - (iy * rmaxysize)) - rysize;

            ossim_int32 dxsize = (ossim_int32)(rxsize/rmaxxsize * tilexsize);
            ossim_int32 dysize = (ossim_int32)(rysize/rmaxysize * tileysize);

            std::stringstream zoomStr;
            std::stringstream ixStr;
            std::stringstream iyStr;

            zoomStr << zoom;
            ixStr << ix;
            iyStr << iy;

            ossimFilename zoomDir = ossimString(outDir + "/" + ossimString(zoomStr.str()));
            zoomDir.createDirectory();

            zoomDir = ossimString(zoomDir + "/") + ossimString::toString(ix);
            zoomDir.createDirectory();

            if (m_isKmz)
            {
               dirVector.push_back(zoomDir);
            }

            ossimString filename = ossimString(zoomDir + "/" + ossimString::toString(yloop-iy-1) + fileExt);
            if (m_isKmz)
            {
               fileVector.push_back(filename);
            }

            generateTile(filename, ix, iy, dxsize, dysize);

            ossimString childKmlfile = ossimString(zoomDir + "/" + ossimString::toString(yloop-iy-1) + ".kml");
            if (m_isKmz)
            {
               fileVector.push_back(childKmlfile);
            }

            isKmlGenerated = generateChildKml(childKmlfile, zoom, yloop, ix, iy, dxsize, dysize, xsize, ysize, maxzoom, projDup, fileExt);
            if (!isKmlGenerated)
            {
               return false;
            }
         }
      }
   }


   if (m_isKmz)
   {
      if (zipWithMinizip(fileVector, outDir, theFilename) == false)
      {
         return false;
      }

      //remove sub-directories and the files under those directories
      for (ossim_uint32 i = 0; i < dirVector.size(); ++i)
      {
         ossimFilename zoomDir = dirVector[i];

         ossimDirectory dir;
         std::vector<ossimFilename> filesInDir;
         if(dir.open(zoomDir))
         {
            ossimFilename file;
            //if (dir.getFirst(file, ossimDirectory::OSSIM_DIR_DEFAULT))
            if (dir.getFirst(file, ossimDirectory::OSSIM_DIR_DEFAULT))
            {
               filesInDir.push_back(file);
            }
            do
            {
               filesInDir.push_back(file);
            }while(dir.getNext(file));
         }

         for(ossim_uint32 j = 0; j < filesInDir.size(); ++j)
         {
            ossimFilename fi = filesInDir[j];
            if (!fi.empty())
            {
               fi.remove();
            }
         }
         if (zoomDir.exists())
         {
            ossimString cmd = "rmdir \"";
            cmd += zoomDir + "\"";
            std::system(cmd.c_str());
         }
      }

      //remove the top directories
      for (int zoom = maxzoom; zoom >= 0; --zoom)
      {
         std::stringstream zoomStr;
         zoomStr << zoom;
         ossimFilename zoomZipDir = ossimString(outDir + "/" + ossimString(zoomStr.str()));
         if (zoomZipDir.exists())
         {
            ossimString cmd = "rmdir \"";
            cmd += zoomZipDir + "\"";
            std::system(cmd.c_str());
         }
      }
      if (ossimFilename(tmpFileName).exists())
      {
         ossimFilename(tmpFileName).remove();
      }
   }

   return true;
}

bool ossimKmlSuperOverlayWriter::saveState(ossimKeywordlist& kwl,
   const char* prefix)const
{
   return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimKmlSuperOverlayWriter::loadState(const ossimKeywordlist& kwl,
   const char* prefix)
{
   return ossimImageFileWriter::loadState(kwl, prefix);
}

bool ossimKmlSuperOverlayWriter::isOpen() const
{
   if (theFilename.size() > 0)
   {
      return true;
   }
   return false;
}

bool ossimKmlSuperOverlayWriter::open()
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
         << "ossimKmlSuperOverlayWriter::open()\n"
         << "File " << theFilename << (result ? " opened" : " not opened")
         << std::endl;
   }

   return result;
}

void ossimKmlSuperOverlayWriter::close()
{
   if (m_imageWriter.valid())
   {
      m_imageWriter = 0;
   }
   if (m_mapProjection.valid())
   {
      m_mapProjection = 0;
   }
}

void ossimKmlSuperOverlayWriter::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back(getShortName());
}

ossimString ossimKmlSuperOverlayWriter::getExtension() const
{
   if (!theFilename.empty())
   {
      return theFilename.ext();
   }
   return ossimString("kml");
}

bool ossimKmlSuperOverlayWriter::hasImageType(const ossimString& imageType) const
{
   bool result = false;
   if ( (imageType == getShortName()) ||
      (imageType == "image/kmlsuperoverlay") )
   {
      result = true;
   }
   return result;
}

void ossimKmlSuperOverlayWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if (property.get())
   {
      if(property->getName().downcase() == "format")
      {
         ossimString format = property->valueToString();
         if (format.downcase() == "png")
         {
            m_isPngFormat = true;
         }
      }
      ossimImageFileWriter::setProperty(property);
   }
}

ossimRefPtr<ossimProperty> ossimKmlSuperOverlayWriter::getProperty(
   const ossimString& name)const
{
   return ossimImageFileWriter::getProperty(name);
}

void ossimKmlSuperOverlayWriter::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   ossimImageFileWriter::getPropertyNames(propertyNames);
}

void ossimKmlSuperOverlayWriter::generateTile(ossimString filename, 
   ossim_int32 ix, 
   ossim_int32 iy, 
   ossim_int32 dxsize, 
   ossim_int32 dysize)
{
   ossimFilename file = filename;
   ossimIpt ul(dxsize*ix, dysize*(iy+1));
   ossimIpt ur(dxsize*(ix+1), dysize*(iy+1));
   ossimIpt lr(dxsize*(ix+1), dysize*iy);
   ossimIpt ll(dxsize*ix, dysize*iy);
   ossimIrect rect(ul, ur, lr, ll);

   m_imageWriter->setFilename(file);
   m_imageWriter->setAreaOfInterest(rect);
   m_imageWriter->execute();
}

bool ossimKmlSuperOverlayWriter::generateRootKml(ossimString filename, 
   ossim_float64 north, 
   ossim_float64 south, 
   ossim_float64 east, 
   ossim_float64 west, 
   ossim_int32 tilesize)
{
   FILE* fp = fopen(filename.c_str(), "wb");
   if (fp == NULL)
   {
      return false;
   }
   ossim_int32 minlodpixels = tilesize/2;

   ossimString tmpfilename = theFilename.fileNoExtension();
   // If we haven't writen any features yet, output the layer's schema
   fprintf(fp, "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n");
   fprintf(fp, "\t<Document>\n");
   fprintf(fp, "\t\t<name>%s</name>\n", tmpfilename.c_str());
   fprintf(fp, "\t\t<description></description>\n");
   fprintf(fp, "\t\t<Style>\n");
   fprintf(fp, "\t\t\t<ListStyle id=\"hideChildren\">\n");
   fprintf(fp, "\t\t\t\t<listItemType>checkHideChildren</listItemType>\n");
   fprintf(fp, "\t\t\t</ListStyle>\n");
   fprintf(fp, "\t\t</Style>\n");
   fprintf(fp, "\t\t<Region>\n \t\t<LatLonAltBox>\n");
   fprintf(fp, "\t\t\t\t<north>%f</north>\n", north);
   fprintf(fp, "\t\t\t\t<south>%f</south>\n", south);
   fprintf(fp, "\t\t\t\t<east>%f</east>\n", east);
   fprintf(fp, "\t\t\t\t<west>%f</west>\n", west);
   fprintf(fp, "\t\t\t</LatLonAltBox>\n");
   fprintf(fp, "\t\t</Region>\n");
   fprintf(fp, "\t\t<NetworkLink>\n");
   fprintf(fp, "\t\t\t<open>1</open>\n");
   fprintf(fp, "\t\t\t<Region>\n");
   fprintf(fp, "\t\t\t\t<Lod>\n");
   fprintf(fp, "\t\t\t\t\t<minLodPixels>%d</minLodPixels>\n", minlodpixels);
   fprintf(fp, "\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>\n");
   fprintf(fp, "\t\t\t\t</Lod>\n");
   fprintf(fp, "\t\t\t\t<LatLonAltBox>\n");
   fprintf(fp, "\t\t\t\t\t<north>%f</north>\n", north);
   fprintf(fp, "\t\t\t\t\t<south>%f</south>\n", south);
   fprintf(fp, "\t\t\t\t\t<east>%f</east>\n", east);
   fprintf(fp, "\t\t\t\t\t<west>%f</west>\n", west);
   fprintf(fp, "\t\t\t\t</LatLonAltBox>\n");
   fprintf(fp, "\t\t\t</Region>\n");
   fprintf(fp, "\t\t\t<Link>\n");
   fprintf(fp, "\t\t\t\t<href>0/0/0.kml</href>\n");
   fprintf(fp, "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>\n");
   fprintf(fp, "\t\t\t</Link>\n");
   fprintf(fp, "\t\t</NetworkLink>\n");
   fprintf(fp, "\t</Document>\n");
   fprintf(fp, "</kml>\n");

   fclose(fp);
   return true;
}

bool ossimKmlSuperOverlayWriter::generateChildKml(ossimString filename, 
   ossim_int32 zoom, 
   ossim_int32 yloop,
   ossim_int32 ix, 
   ossim_int32 iy, 
   ossim_int32 dxsize, 
   ossim_int32 dysize,
   ossim_int32 xsize,
   ossim_int32 ysize, 
   ossim_int32 maxzoom,
   ossimRefPtr<ossimMapProjection> proj,
   ossimString fileExt)
{
   ossim_int32 yIndex = yloop-iy-1;
   ossimIpt ul(dxsize*ix, dysize*(iy+1));
   ossimIpt ur(dxsize*(ix+1), dysize*(iy+1));
   ossimIpt lr(dxsize*(ix+1), dysize*iy);
   ossimIpt ll(dxsize*ix, dysize*iy);
   ossimIrect rect(ul, ur, lr, ll);

   ossim_float64 tnorth = 0.0;
   ossim_float64 tsouth = 0.0;
   ossim_float64 teast = 0.0;
   ossim_float64 twest = 0.0;

   ossim_float64 upperleftT = 0.0;
   ossim_float64 lowerleftT = 0.0;

   ossim_float64 rightbottomT = 0.0;
   ossim_float64 leftbottomT = 0.0;

   ossim_float64 lefttopT = 0.0;
   ossim_float64 righttopT = 0.0;

   ossim_float64 lowerrightT = 0.0;
   ossim_float64 upperrightT = 0.0;

   ossimGpt ulg;
   ossimGpt llg;
   ossimGpt lrg;
   ossimGpt urg;

   if (m_mapProjection.valid())
   {
      m_mapProjection->lineSampleToWorld(rect.ul(), ulg);
      m_mapProjection->lineSampleToWorld(rect.ll(), llg);
      m_mapProjection->lineSampleToWorld(rect.lr(), lrg);
      m_mapProjection->lineSampleToWorld(rect.ur(), urg);

      twest = llg.lon;
      tsouth = llg.lat;
      teast = urg.lon;
      tnorth = urg.lat;

      upperleftT = ulg.lon;
      lefttopT = ulg.lat;

      lowerleftT = llg.lon;
      leftbottomT = llg.lat;

      lowerrightT = lrg.lon;
      rightbottomT = lrg.lat;

      upperrightT = urg.lon; 
      righttopT = urg.lat;
   }

   std::vector<ossim_int32> xchildren;
   std::vector<ossim_int32> ychildern;
   std::vector<ossim_int32> ychildernIndex;

   ossim_int32 maxLodPix = -1;
   if ( zoom < maxzoom )
   {
      ossim_float64 zareasize = pow(2.0, (maxzoom - zoom - 1))*dxsize;
      ossim_float64 zareasize1 = pow(2.0, (maxzoom - zoom - 1))*dysize;
      xchildren.push_back(ix*2);
      ossim_int32 tmp = ix*2 + 1;
      ossim_int32 tmp1 = (ossim_int32)ceil(xsize/zareasize);
      if (tmp < tmp1)
      {
         xchildren.push_back(ix*2+1);
      }

      ychildern.push_back(iy*2);
      tmp = iy*2 + 1;
      tmp1 = (ossim_int32)ceil(ysize/zareasize1);
      if (tmp < tmp1)
      {
         ychildern.push_back(iy*2+1);
      }    

      ychildernIndex.push_back(yIndex*2);
      tmp = yIndex*2 + 1;
      tmp1 = (ossim_int32)ceil(ysize/zareasize1);
      if (tmp < tmp1)
      {
         ychildernIndex.push_back(yIndex*2+1);
      }    
      maxLodPix = 2048;
   }

   FILE* fp = fopen(filename.c_str(), "wb");
   if (fp == NULL)
   {
      return false;
   }

   fprintf(fp, "<kml xmlns=\"http://earth.google.com/kml/2.1\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n");
   fprintf(fp, "\t<Document>\n");
   fprintf(fp, "\t\t<name>%d/%d/%d.kml</name>\n", zoom, ix, yIndex);
   fprintf(fp, "\t\t<Style>\n");
   fprintf(fp, "\t\t\t<ListStyle id=\"hideChildren\">\n");
   fprintf(fp, "\t\t\t\t<listItemType>checkHideChildren</listItemType>\n");
   fprintf(fp, "\t\t\t</ListStyle>\n");
   fprintf(fp, "\t\t</Style>\n");
   fprintf(fp, "\t\t<Region>\n");
   fprintf(fp, "\t\t\t<Lod>\n");
   fprintf(fp, "\t\t\t\t<minLodPixels>%d</minLodPixels>\n", 128);
   fprintf(fp, "\t\t\t\t<maxLodPixels>%d</maxLodPixels>\n", maxLodPix);
   fprintf(fp, "\t\t\t</Lod>\n");
   fprintf(fp, "\t\t\t<LatLonAltBox>\n");
   fprintf(fp, "\t\t\t\t<north>%f</north>\n", tnorth);
   fprintf(fp, "\t\t\t\t<south>%f</south>\n", tsouth);
   fprintf(fp, "\t\t\t\t<east>%f</east>\n", teast);
   fprintf(fp, "\t\t\t\t<west>%f</west>\n", twest);
   fprintf(fp, "\t\t\t</LatLonAltBox>\n");
   fprintf(fp, "\t\t</Region>\n");
   fprintf(fp, "\t\t<GroundOverlay>\n");
   fprintf(fp, "\t\t\t<drawOrder>%d</drawOrder>\n", zoom);
   fprintf(fp, "\t\t\t<Icon>\n");
   fprintf(fp, "\t\t\t\t<href>%d%s</href>\n", yIndex, fileExt.c_str());
   fprintf(fp, "\t\t\t</Icon>\n");
   fprintf(fp, "\t\t\t<gx:LatLonQuad>\n");
   fprintf(fp, "\t\t\t\t<coordinates>\n");
   fprintf(fp, "\t\t\t\t\t%f, %f, 0\n", lowerleftT, leftbottomT);
   fprintf(fp, "\t\t\t\t\t%f, %f, 0\n", lowerrightT, rightbottomT);
   fprintf(fp, "\t\t\t\t\t%f, %f, 0\n", upperrightT, righttopT);
   fprintf(fp, "\t\t\t\t\t%f, %f, 0\n", upperleftT, lefttopT);
   fprintf(fp, "\t\t\t\t</coordinates>\n");
   fprintf(fp, "\t\t\t</gx:LatLonQuad>\n");
   fprintf(fp, "\t\t</GroundOverlay>\n");

   //generate lat, lon info and links for the previous zoom of kml
   for (ossim_uint32 i = 0; i < xchildren.size(); i++)
   {
      int cx = xchildren[i];
      for (ossim_uint32 j = 0; j < ychildern.size(); j++)
      {
         ossim_int32 cy = ychildern[j];
         ossim_int32 cyIndex = ychildernIndex[ychildernIndex.size()-j-1];

         ossimIpt ulc(dxsize*cx, dysize*(cy+1));
         ossimIpt urc(dxsize*(cx+1), dysize*(cy+1));
         ossimIpt lrc(dxsize*(cx+1), dysize*cy);
         ossimIpt llc(dxsize*cx, dysize*cy);
         ossimIrect rectc(ulc, urc, lrc, llc);

         ossim_float64 cnorth = 0.0;
         ossim_float64 csouth = 0.0;
         ossim_float64 ceast = 0.0;
         ossim_float64 cwest = 0.0;

         if (proj.valid())
         {
            ulg.makeNan();
            lrg.makeNan();
            proj->lineSampleToWorld(rectc.ll(), llg);
            proj->lineSampleToWorld(rectc.ur(), urg);

            cwest = llg.lon;
            csouth = llg.lat;

            ceast = urg.lon; 
            cnorth = urg.lat;
         }

         fprintf(fp, "\t\t<NetworkLink>\n");
         fprintf(fp, "\t\t\t<name>%d/%d/%d%s</name>\n", zoom+1, cx, cyIndex, fileExt.c_str());
         fprintf(fp, "\t\t\t<Region>\n");
         fprintf(fp, "\t\t\t\t<Lod>\n");
         fprintf(fp, "\t\t\t\t\t<minLodPixels>128</minLodPixels>\n");
         fprintf(fp, "\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>\n");
         fprintf(fp, "\t\t\t\t</Lod>\n");
         fprintf(fp, "\t\t\t\t<LatLonAltBox>\n");
         fprintf(fp, "\t\t\t\t\t<north>%f</north>\n", cnorth);
         fprintf(fp, "\t\t\t\t\t<south>%f</south>\n", csouth);
         fprintf(fp, "\t\t\t\t\t<east>%f</east>\n", ceast);
         fprintf(fp, "\t\t\t\t\t<west>%f</west>\n", cwest);
         fprintf(fp, "\t\t\t\t</LatLonAltBox>\n");
         fprintf(fp, "\t\t\t</Region>\n");
         fprintf(fp, "\t\t\t<Link>\n");
         fprintf(fp, "\t\t\t\t<href>../../%d/%d/%d.kml</href>\n", zoom+1, cx, cyIndex);
         fprintf(fp, "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>\n");
         fprintf(fp, "\t\t\t\t<viewFormat/>\n");
         fprintf(fp, "\t\t\t</Link>\n");
         fprintf(fp, "\t\t</NetworkLink>\n");
      }
   }

   fprintf(fp, "\t</Document>\n");
   fprintf(fp, "</kml>\n");
   fclose(fp);
   return true;
}

bool ossimKmlSuperOverlayWriter::zipWithMinizip(std::vector<ossimString> srcFiles, ossimString srcDirectory, ossimString targetFile)
{
   zipFile zipfile = zipOpen(targetFile.c_str(), 0);
   if (!zipfile)
   {
      ossimSetError(getClassName(),
         ossimErrorCodes::OSSIM_ERROR,
         "Unable to open target zip file..");
      return false;
   }

   std::vector<ossimString>::iterator v1_Iter;
   for(v1_Iter = srcFiles.begin(); v1_Iter != srcFiles.end(); v1_Iter++)
   {
      ossimString fileRead = *v1_Iter;

      // Find relative path and open new file with zip file
      std::string relativeFileReadPath = fileRead;
      ossim_int32 remNumChars = srcDirectory.size();
      if(remNumChars > 0)
      {
         ossim_int32 f = fileRead.find(srcDirectory.string());
         if( f >= 0 )
         {
            relativeFileReadPath.erase(f, remNumChars + 1); // 1 added for backslash at the end
         }      
      }

      std::basic_string<char>::iterator iter1;
      for (iter1 = relativeFileReadPath.begin(); iter1 != relativeFileReadPath.end(); iter1++)
      {
         int f = relativeFileReadPath.find("\\");
         if (f >= 0)
         {
            relativeFileReadPath.replace(f, 1, "/");
         }
         else
         {
            break;
         }
      }
      if (zipOpenNewFileInZip(zipfile, relativeFileReadPath.c_str(), 0, 0, 0, 0, 0, 0, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK)
      {
         ossimSetError(getClassName(),
            ossimErrorCodes::OSSIM_ERROR,
            "Unable to create file within the zip file..");
         return false;
      }

      // Read source file and write to zip file
      std::fstream inFile (fileRead.c_str(), std::ios::binary | std::ios::in);
      if (!inFile.is_open())
      {
         ossimSetError(getClassName(),
            ossimErrorCodes::OSSIM_ERROR,
            "Could not open source file..");
         return false;
      }
      if (!inFile.good())
      {
         ossimSetError(getClassName(),
            ossimErrorCodes::OSSIM_ERROR,
            "Error reading source file..");
         return false;
      }

      // Read file in buffer
      std::string fileData;
      const unsigned int bufSize = 1024;
      char buf[bufSize];
      do 
      {
         inFile.read(buf, bufSize);
         fileData.append(buf, inFile.gcount());
      } while (!inFile.eof() && inFile.good());

      if (zipWriteInFileInZip(zipfile, static_cast<const void*>(fileData.data()), static_cast<unsigned int>(fileData.size())) != ZIP_OK )
      {
         ossimSetError(getClassName(),
            ossimErrorCodes::OSSIM_ERROR,
            "Could not write to file within zip file..");
         return false;
      }

      // Close one src file zipped completely
      if ( zipCloseFileInZip(zipfile) != ZIP_OK )
      {
         ossimSetError(getClassName(),
            ossimErrorCodes::OSSIM_ERROR,
            "Could not close file written within zip file..");
         return false;
      }
   }

   zipClose(zipfile, 0);
   return true;
}

void ossimKmlSuperOverlayWriter::propagateViewChange()
{
   if ( theInputConnection.valid() && m_mapProjection.valid() )
   {
      //---
      // Send a view interface visitor to the input connection.  This will find all view
      // interfaces, set the view, and send a property event to output so that the combiner,
      // if present, can reinitialize.
      //---
      ossimViewInterfaceVisitor vv( m_mapProjection.get(),
                                    (ossimViewInterfaceVisitor::VISIT_INPUTS|
                                     ossimViewInterfaceVisitor::VISIT_CHILDREN) );
      theInputConnection->accept(vv);
   }
}

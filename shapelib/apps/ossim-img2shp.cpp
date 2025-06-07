//---
//
// License: MIT
//
// Author: George Stewart
// 
// Description: This program creates an ESRI shapefile containing
//              either the coordinates of the bounding box of an
//              image or the actual coordinates of the image's
//              corners.
//---
// $Id$

#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeyword.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/init/ossimInit.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <iostream>
#include <shapefil.h>

void finalize(int exit_code)
{
   ossimInit::instance()->finalize();
   exit(exit_code);
}

bool processEntry( ossimImageHandler* ih, ossim_uint32 entry, const ossimFilename& shapeFile )
{
   static const char MODULE[] = "img2shp:processEntry(...)";
   
   bool result = false;

   if ( ih )
   {
      result = true;
      if ( ih->getCurrentEntry() != entry )
      {
         result = ih->setCurrentEntry(entry);
      }

      if ( result )
      {
         // SHPCreate(...) puts the dot.shp extension on.
         ossimFilename entryShapeFile = shapeFile;
         if ( ih->getNumberOfEntries() > 1 )
         {
            entryShapeFile = entryShapeFile.noExtension();
            entryShapeFile.string() += std::string("_e") + ossimString::toString(entry).string();
            entryShapeFile.setExtension( ossimString(".shp") );
         }
      
         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if ( geom.valid() )
         {
            ossimPolyArea2d polyArea;
            if ( geom->isMapProjected() )
            {
               geom->calculatePolyBounds(polyArea, 1);
            }
            else // Sensor model:
            {
               geom->calculatePolyBounds(polyArea, 25);
            }
            
            polyArea.toMultiPolygon();
            if( polyArea.isEmpty() == false )
            {
               std::vector<ossimPolygon> polyList;
               if ( polyArea.getVisiblePolygons( polyList ) == true )
               {
                  ossim_int32 numberOfPolygons = (ossim_int32)polyList.size();

                  //---
                  // Create the shape file handle.
                  // Data that crosses the international date line will have
                  // two polygons.
                  //---
                  int nSHPType = ( numberOfPolygons == 1 ) ? SHPT_POLYGON: SHPT_POLYGONM;
                  SHPHandle hSHP = SHPCreate( entryShapeFile.noExtension().c_str(), nSHPType);
                  
                  if( hSHP )
                  {
                     // Get the total number of vertices:
                     std::vector<int> panParts(numberOfPolygons, 0);
                     std::vector<int> panPartType(numberOfPolygons);
            
                     ossim_int32 numberOfVerticesTotal = 0;
                     ossim_int32 polyIndex = 0;
                     for ( ;  polyIndex < numberOfPolygons; ++polyIndex )
                     {
                        // Start of ring for this polygon.
                        panParts[polyIndex] = numberOfVerticesTotal; 
                        panPartType[polyIndex] = SHPP_RING;
                        numberOfVerticesTotal += (ossim_int32)polyList[polyIndex].
                           getNumberOfVertices();
                     }
                     
                     std::vector<double> padfX(numberOfVerticesTotal);
                     std::vector<double> padfY(numberOfVerticesTotal);
                     std::vector<double> padfZ(numberOfVerticesTotal, 0.0);
                     
                     ossim_int32 verticeIndex = 0;
                     for ( polyIndex = 0; polyIndex < numberOfPolygons; ++polyIndex )
                     {
                        for (const auto& pt : polyList[polyIndex].getVertexList() )
                        {
                           padfX[verticeIndex] = pt.x;
                           padfY[verticeIndex++] = pt.y;
                           
                           // We could stuff z with elevation...
                           // Not sure about m?
                        }
                     }
                     
                     SHPObject *psObject = SHPCreateObject(
                        nSHPType, -1, numberOfPolygons,
                        panParts.data(), panPartType.data(), numberOfVerticesTotal,
                        padfX.data(), padfY.data(), padfZ.data(), NULL );
                     SHPWriteObject( hSHP, -1, psObject );
                     SHPDestroyObject( psObject );
                     SHPClose( hSHP );

                     ossimNotify(ossimNotifyLevel_NOTICE)
                        << "Wrote file: " << entryShapeFile << std::endl;
                  }
                  else
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << MODULE << " ERROR:\n"
                        << "Could create shape file: " << entryShapeFile.c_str()
                        << std::endl;
                     result = false;
                  }

                  //---
                  // Build DBF:
                  // Not sure what to put in this?
                  //---
                  ossimFilename entryDbfFile = entryShapeFile.noExtension();
                  entryDbfFile.setExtension( ossimString("dbf") );
                  DBFHandle hDBF = DBFCreate( entryDbfFile.noExtension().c_str() );
                  if ( hDBF )
                  {
                     DBFFieldType eType = FTString;
                     std::string s = "OSSIM Generated Shapefile";
                     DBFAddField( hDBF, "creator", eType, (int)s.size()+1, 0 );
                     DBFWriteStringAttribute(hDBF, 0, 0, s.c_str());
                     DBFClose( hDBF );

                     ossimNotify(ossimNotifyLevel_NOTICE)
                        << "Wrote file: " << entryDbfFile << std::endl;
                  }
               }
            }
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << " ERROR:\n"
               << "Entry " << entry << " has no image geometry!" << std::endl;
            result = false;
         }
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " ERROR:\n"
            << "Could not open entry: " << entry << std::endl;
         result = false;
      }
   }

   return result;
}

void usage(ossimArgumentParser& ap)
{
   // Add global usage options.
   ossimInit::instance()->addOptions(ap);
   
   // Set app name.
   std::string appName = ap.getApplicationName();
   ap.getApplicationUsage()->setApplicationName( ossimString( appName ) );

   ap.getApplicationUsage()->setDescription(ap.getApplicationName()+" creates an ESRI shapefile from an image file.");
   
   ap.getApplicationUsage()->setCommandLineUsage(ap.getApplicationName()+" [options] <image_file>\n");
   
   ap.getApplicationUsage()->addCommandLineOption("-v or --vertices","Use valid vertices of image if present instead of image corners.");

   ap.getApplicationUsage()->
      addCommandLineOption("-o", "Write to file specified.  Cannot be same as image file.");

   ap.getApplicationUsage()->addCommandLineOption("-h or --help", "Shows help");
   
   ap.getApplicationUsage()->write(std::cout);
}

int main(int argc, char* argv[])
{
   static const char MODULE[] = "img2shp:main";

   ossimArgumentParser ap(&argc, argv);

   // Initialize ossim stuff, factories, plugin, etc.
   ossimInit::instance()->initialize(ap);

   // Check for usage/help option.
   if(ap.read("-h") || ap.read("--help"))
   {
      usage(ap);
      finalize(0);
   }
   
   ossimString tempString;
   ossimArgumentParser::ossimParameter stringParam(tempString);
 
   // Check for valid vertice option.
   bool useVertices = false;
   if (ap.read("-v") || ap.read("--vertices"))
   {
      useVertices = true;
   }

   // Check for optional output file name.
   ossimFilename shapeFile;
   if( ap.read("-o", stringParam) )
   {
      shapeFile = tempString.trim();
   }

   // Uncaught options.
   ap.reportRemainingOptionsAsUnrecognized();

   // Errors...
   if (ap.errors())
   {
      ap.writeErrorMessages(std::cout);
      finalize(1);
   }

   // Arg count should be 2 now, app_name and input_file.
   if(ap.argc() != 2)
   {
      usage(ap);
      finalize(1);
   }

   // Get the image file name.
   ossimFilename imageFile;
   imageFile = argv[ap.argc()-1];

   // Make sure is exists...
   if(!imageFile.exists())
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " ERROR: " << imageFile << " does not exist!" << std::endl;
      finalize(1);
   }

   // Check/create a shape file as needed.
   if (shapeFile.empty())
   {
      shapeFile = imageFile.noExtension();
      shapeFile += "_vertices.shp";
   }
   else if ( shapeFile.isDir() )
   {
      shapeFile = shapeFile.dirCat( imageFile.noExtension() );
      shapeFile += "_vertices.shp";
   }
   else if ( shapeFile.ext().empty() )
   {
      shapeFile.setExtension( ossimString("shp") );
   }

#if 0
   ossimNotify(ossimNotifyLevel_NOTICE)
      << "Input image file:        " << imageFile
      << "\nOutput shape file base:  " << shapeFile
      << "\nUse valid vertices flag: " << (useVertices?"true":"false")
      << std::endl;
#endif
   
   //---
   // Avoid geometry file collisions.  The shape file without extension
   // cannot be same as image file without extension.
   //---
   if (shapeFile.noExtension() == imageFile.noExtension())
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " ERROR:\n"
         << "Input and output file name bases cannot be the same!"
         << "\nExiting..."
         << std::endl;
      finalize(1);
   }

   ossimRefPtr<ossimImageHandler> ih =
      ossimImageHandlerRegistry::instance()->open(imageFile);
   if ( ih.valid() )
   {
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      for ( const auto& entry : entryList )
      {
         if ( processEntry( ih.get(), entry, shapeFile ) == false )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << " ERROR:\n"
               << "Could not process entry: " << entry
               << "\nExiting..." << std::endl;
            finalize(1);
         }
      }
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " ERROR:\n"
         << "Could not open file:  " << imageFile
         << "\nExiting..."
         << std::endl;
      finalize(1);
   }

   finalize(0);
   
} // End of main...

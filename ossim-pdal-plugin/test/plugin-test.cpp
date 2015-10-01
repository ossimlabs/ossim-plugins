//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id: plugin-test.cpp 23401 2015-06-25 15:00:31Z okramer $
#include "../src/ossimPdalFileReader.h"
#include "../src/ossimRialtoReader.h"
#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimKeywordNames.h>
#include <assert.h>
#include <ossim/point_cloud/ossimPointCloudImageHandler.h>
#include <ossim/imaging/ossimTiffWriter.h>
#include <pdal/pdal.hpp>
#include <pdal/util/Bounds.hpp>
#include <pdal/LasWriter.hpp>
#include <pdal/FauxReader.hpp>

#define TEST_READER false

using namespace pdal;

int usage(char* app_name)
{
   cout << "\nUsage: "<<app_name<<" <pdal|rialto|genlas> [filename]\n" << endl;
   return 1;
}

bool writeRaster(ossimPdalReader* reader, const char* test)
{
   ossimRefPtr<ossimPointCloudImageHandler> ih =  new ossimPointCloudImageHandler;
   ih->setCurrentEntry((ossim_uint32)ossimPointCloudImageHandler::HIGHEST);
   ih->setPointCloudHandler(reader);

   ossimDpt gsd;
   ih->getGSD(gsd, 0);
   ossimString gsdstr = ossimString::toString((gsd.x + gsd.y)/6.0);
   ossimRefPtr<ossimProperty> gsd_prop =
         new ossimStringProperty(ossimKeywordNames::METERS_PER_PIXEL_KW, gsdstr);
   ih->setProperty(gsd_prop);

   // Set up the writer:
   ossimRefPtr<ossimTiffWriter> tif_writer =  new ossimTiffWriter();
   tif_writer->setGeotiffFlag(true);

   ossimFilename outfile (test);
   outfile += "-OUTPUT.tif";
   tif_writer->setFilename(outfile);
   if (tif_writer.valid())
   {
      tif_writer->connectMyInputTo(0, ih.get());
      tif_writer->execute();
   }

   cout << "Output written to <"<<outfile<<">"<<endl;
   return true;
}


bool test_rialto(const ossimFilename& fname)
{
   cout << "Testing rialto with <"<<fname<<">"<<endl;

   ossimRefPtr<ossimRialtoReader> reader = new ossimRialtoReader;
   reader->open(fname);

   ossimGrect bounds;
   reader->getBounds(bounds);
   cout <<"bounds = "<<bounds<<endl;

   ossimPointBlock pointBlock;
   reader->getBlock(bounds, pointBlock);

   writeRaster(reader.get(), "rialto");

   return true;
}


bool test_pdal(const ossimFilename& fname)
{
   cout << "Testing pdal with <"<<fname<<">"<<endl;

   ossimRefPtr<ossimPdalFileReader> reader = new ossimPdalFileReader;
   reader->open(fname);

   ossimGrect bounds;
   reader->getBounds(bounds);
   cout <<"bounds = "<<bounds<<endl;

   writeRaster(reader.get(), "pdal");

   return 0;
}

bool genlas(const ossimFilename& fname)
{
   cout << "Generating file <"<<fname<<">"<<endl;

   FauxReader reader;
   Options roptions;
   BOX3D bbox(-0.001, -0.001, -100.0, 0.001, 0.001, 100.0);
   roptions.add("bounds", bbox);
   roptions.add("num_points", 11);
   roptions.add("mode", "ramp");
   reader.setOptions(roptions);

   LasWriter writer;
   Options woptions;
   woptions.add("filename", fname.string());
   woptions.add("a_srs", "EPSG:4326"); // causes core dump when ossimInit::initialize() called on startup
   woptions.add("scale_x", 0.0000001);
   woptions.add("scale_y", 0.0000001);
   writer.setOptions(woptions);
   writer.setInput(reader);

   PointTable wtable;
   writer.prepare(wtable);
   writer.execute(wtable);

   return true;
}

int main(int argc, char** argv)
{
   // TODO: Figure out why program core-dumps on exit when this line is included and genlas test is
   // run. Determined problem to be in GDAL's OGRSpatialReference::SetFromUserInput() called from
   // PDAL's SpatialReference::setFromUserInput() called when EPSG option is set:
   //ossimInit::instance()->initialize(argc, argv);

   if (argc == 1)
      return usage(argv[0]);

   ossimFilename fname;
   if (argc > 2)
      fname = argv[2];

   ossimString test_name (argv[1]);
   bool passed = false;
   if (test_name.downcase() == "rialto")
   {
      if (fname.empty())
         fname = "autzen.gpkg";
      passed = test_rialto(fname);
   }
   else if (test_name.downcase() == "pdal")
   {
      if (fname.empty())
         fname = "autzen.las";
      passed = test_pdal(fname);
   }
   else if (test_name.downcase() == "genlas")
   {
      if (fname.empty())
         fname = "ramp.las";
      passed = genlas(fname);
   }
   else
      return usage(argv[0]);

   if (passed)
      return 0;
   return 1;
}



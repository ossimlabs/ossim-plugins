//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimPotraceUtil.h"
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <ossim/base/ossimKeywordNames.h>
#include <sstream>

using namespace std;

const char* ossimPotraceUtil::DESCRIPTION =
      "Computes vector representation of input image.";

static const string TOLERANCE_KW = "tolerance";


ossimPotraceUtil::ossimPotraceUtil()
:  m_tolerance(0)
{
}

ossimPotraceUtil::~ossimPotraceUtil()
{
}

void ossimPotraceUtil::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options.
   ossimUtility::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " [options] <output-vector-file>";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--tolerance <float>",
         "tolerance +- deviation from threshold for marginal classifications. Defaults to 0.05.");
}

bool ossimPotraceUtil::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);
   string ts2;
   ossimArgumentParser::ossimParameter sp2(ts2);
   string ts3;
   ossimArgumentParser::ossimParameter sp3(ts3);

   if ( ap.read("--tolerance", sp1))
      m_kwl.addPair(TOLERANCE_KW, ts1);

   if (!ossimUtility::initialize(ap))
      return false;

   // Remaining argument is input and output file names:
   if (ap.argc() > 1)
   {
      m_kwl.add(ossimKeywordNames::IMAGE_FILE_KW, ap.argv()[1]);
      if (ap.argc() > 2)
         m_kwl.add(ossimKeywordNames::OUTPUT_FILE_KW, ap.argv()[2]);
   }

   return true;
}

void ossimPotraceUtil::initialize(const ossimKeywordlist& kwl)
{
   ossimString value;
   ostringstream xmsg;

   // Don't copy KWL if member KWL passed in:
   if (&kwl != &m_kwl)
   {
      // Start with clean options keyword list.
      m_kwl.clear();
      m_kwl.addList( kwl, true );
   }

   value = m_kwl.findKey(TOLERANCE_KW);
   if (!value.empty())
      m_tolerance = value.toDouble();


   m_inputRasterFname = m_kwl.findKey(ossimKeywordNames::IMAGE_FILE_KW);

   m_outputVectorFname = m_kwl.findKey(ossimKeywordNames::OUTPUT_FILE_KW);
   if (m_outputVectorFname.empty())
   {
      m_outputVectorFname = m_inputRasterFname;
      m_outputVectorFname.setExtension("geojson");
   }

   ossimUtility::initialize(kwl);

   m_bitmapFname = m_inputRasterFname;
   m_bitmapFname.setExtension("pbm");
}

bool ossimPotraceUtil::execute()
{
   // Open input image:
   m_inputHandler = ossimImageHandlerRegistry::instance()->open(m_inputRasterFname, 1, 0);
   if (!m_inputHandler.valid())
   {
      cout <<"ossimPotraceUtil:"<<__LINE__<<" Could not open input file <"<<m_inputRasterFname<<">"
            <<endl;
      return false;
   }
   // Vector coordinates are in image space. Need to convert to geographic lat/lon:
   ossimRefPtr<ossimImageGeometry> geom = m_inputHandler->getImageGeometry();
   if (!geom.valid())
   {
      cout <<"ossimPotraceUtil:"<<__LINE__<<" Encountered null image geometry!"<<endl;
      return false;
   }

   // Convert raster to bitmap:
   potrace_bitmap_t* potraceBitmap = convertToBitmap();
   if (!potraceBitmap)
      return false;

   // Perform vectorization:
   potrace_param_t* potraceParam = potrace_param_default();
   potraceParam->turdsize = 10;
   potraceParam->alphamax = 0;
   potrace_state_t* potraceOutput = potrace_trace(potraceParam, potraceBitmap);
   if (!potraceOutput)
   {
      cout <<"ossimPotraceUtil:"<<__LINE__<<" Null pointer returned from potrace_trace!"<<endl;
      return false;
   }

   potrace_path_t* potracePaths = potraceOutput->plist;
   potrace_path_t* path = potracePaths;
   ossimDpt imgPt;
   ossimGpt gndPt;
   while (path)
   {
      int numSegments = path->curve.n;
      for (int i=0; i<numSegments; ++i)
      {
         // Convert up 3 possible vertices per segment
         for (int v=0; v<3; ++v)
         {
            if ((v == 0) && (path->curve.tag[i] = POTRACE_CORNER))
               continue;

            imgPt.x = path->curve.c[i][v].x;
            imgPt.y = path->curve.c[i][v].y;
            geom->localToWorld(imgPt, gndPt);
            path->curve.c[i][v].x = gndPt.lon;
            path->curve.c[i][v].y = gndPt.lat;
         }
      }
      path = path->next;
   }

   // Write to output vector file:
   if (!writeGeoJSON(potracePaths))
      return false;

   // Release memory:
   potrace_state_free(potraceOutput);
   //free(potraceBitmap->map);
   delete potraceBitmap;
   free(potraceParam);

   return true;
}

void ossimPotraceUtil::getKwlTemplate(ossimKeywordlist& kwl)
{
   ossimUtility::getKwlTemplate(kwl);
   kwl.add(TOLERANCE_KW.c_str(), "TBR");
}

potrace_bitmap_t* ossimPotraceUtil::convertToBitmap()
{
   potrace_bitmap_t* potraceBitmap = new potrace_bitmap_t;

   // Determine output bitmap size to even word boundary:
   ossimIrect rect = m_inputHandler->getImageRectangle();
   potraceBitmap->w = rect.width();
   potraceBitmap->h = rect.height();
   int pixelsPerWord = 8 * sizeof(int*);
   potraceBitmap->dy = (int) ceil((double)rect.width()/pixelsPerWord);

   // Allocate the bitmap memory:
   unsigned long bufLength = potraceBitmap->dy*potraceBitmap->h;
   potraceBitmap->map = new potrace_word[bufLength];

   // Prepare to sequence over all input image tiles and fill the bitmap image:
   ossimRefPtr<ossimImageSourceSequencer> sequencer =
         new ossimImageSourceSequencer(m_inputHandler.get());
   ossimRefPtr<ossimImageData> tile = sequencer->getNextTile();
   double null_pix = tile->getNullPix(0);
   unsigned long offset=0;

   // Loop to fill bitmap buffer:
   while (tile.valid())
   {
      ossimIpt pt (tile->getOrigin());
      ossimIpt lastPt (pt.x + tile->getWidth() - 1, pt.y + tile->getHeight() - 1);

      // Nested loops over all pixels in input tile:
      for (; pt.y <= lastPt.y; ++pt.y)
      {
         offset = tile->getOrigin().x/pixelsPerWord + pt.y*potraceBitmap->dy;
         for (pt.x = tile->getOrigin().x; pt.x <= lastPt.x; )
         {
            // Loop to pack a word buffer with pixel on (non-null) or off bit in proper positions:
            unsigned long wordBuf = 0;
            for (int bitpos=pixelsPerWord-1; bitpos>=0; --bitpos)
            {
               unsigned long pixel = (tile->getPix(pt) != null_pix) ? 1 : 0;
               wordBuf |= pixel << bitpos;
               ++pt.x;
               if (pt.x > lastPt.x)
                  break;
            }
            potraceBitmap->map[offset++] = wordBuf;
         }
      }

      tile = sequencer->getNextTile();
   }

   return potraceBitmap;
}

bool ossimPotraceUtil::writeGeoJSON(potrace_path_t* vectorList)
{
   FILE* outFile = fopen(m_outputVectorFname.chars(), "w");
   if (!outFile)
   {
      cout <<"ossimPotraceUtil:"<<__LINE__<<" Could not open output file <"<<m_outputVectorFname
            <<"> for writing."<<endl;
      return false;
   }

   potrace_geojson(outFile, vectorList);

   return true;
}

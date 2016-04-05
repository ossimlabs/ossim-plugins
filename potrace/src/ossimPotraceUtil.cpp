//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimPotraceUtil.h"
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimException.h>
#include <sstream>

using namespace std;

const char* ossimPotraceUtil::DESCRIPTION =
      "Computes vector representation of input raster image.";

static const string MODE_KW = "mode";
static const string ALPHAMAX_KW = "alphamax";
static const string TURDSIZE_KW = "turdsize";

ossimPotraceUtil::ossimPotraceUtil()
:  m_mode (LINESTRING),
   m_alphamax (1.0),
   m_turdSize (4),
   m_outputToConsole(false),
   m_maskBitmap (0),
   m_productBitmap (0)
{
}

ossimPotraceUtil::~ossimPotraceUtil()
{
   delete m_productBitmap;
   if (m_maskBitmap)
      delete m_maskBitmap;
}

void ossimPotraceUtil::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options. Don't include ossimChipProcUtil options as not appropriate.
   ossimUtility::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " [options] <input-raster-file> [<output-vector-file>]";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--alphamax <float>",
         "set the corner threshold parameter. The default value is 1. The smaller this value, the "
         "more sharp corners will be produced. If this parameter is 0, then no smoothing will be "
         "performed and the output is a polygon. If this parameter is greater than 1.3, then all "
         "corners are suppressed and the output is completely smooth");
   au->addCommandLineOption("--mode polygon|linestring",
         "Specifies whether to represent foreground-background boundary as polygons or line-strings"
         ". Polygons are closed regions surrounding either null or non-null pixels. Most viewers "
         "will represent polygons as solid blobs. Line-strings only outline the boundary but do not"
         " maintain sense of \"insideness\"");
   au->addCommandLineOption("--mask <filename>",
         "Use the raster file provided as a mask to exclude any vertices found within 1 pixel of "
         "a null mask pixel. Implies linestring mode since polygons may not be closed. The mask "
         "should be a single-band image, but if multi-band, only band 0 will be referenced.");
   au->addCommandLineOption("--turdsize <int>", "suppress speckles of up to this many pixels.");
}

bool ossimPotraceUtil::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);

   if (!ossimUtility::initialize(ap))
      return false;

   if ( ap.read("--alphamax", sp1))
      m_kwl.addPair(ALPHAMAX_KW, ts1);

   if ( ap.read("--mode", sp1))
      m_kwl.addPair(MODE_KW, ts1);

   if ( ap.read("--mask", sp1))
   {
      // Mask handled as a second image source:
      ostringstream key;
      key<<ossimKeywordNames::IMAGE_FILE_KW<<"1";
      m_kwl.addPair( key.str(), ts1 );
   }

   if ( ap.read("--turdsize", sp1))
      m_kwl.addPair(TURDSIZE_KW, ts1);

   processRemainingArgs(ap);
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

   value = m_kwl.findKey(ALPHAMAX_KW);
   if (!value.empty())
      m_alphamax = value.toDouble();

   value = m_kwl.findKey(TURDSIZE_KW);
   if (!value.empty())
      m_turdSize = value.toInt();

   value = m_kwl.findKey(MODE_KW);
   if (value.contains("polygon"))
      m_mode = POLYGON;
   else if (value.contains("linestring"))
      m_mode = LINESTRING;
   else if (!value.empty())
   {
      xmsg <<"ossimPotraceUtil:"<<__LINE__<<" Unallowed mode requested: <"<<value<<">."
            <<endl;
      throw ossimException(xmsg.str());
   }

   ossimChipProcUtil::initialize(kwl);
}

void ossimPotraceUtil::initProcessingChain()
{
   // Nothing to do.
}

void ossimPotraceUtil::finalizeChain()
{
   // Do nothing and avoid ossimChipProcUtil from doing its standard stuff.
}

bool ossimPotraceUtil::execute()
{
   ostringstream xmsg;

   if (m_imgLayers.empty() || !m_geom.valid())
   {
      xmsg<<"ossimPotraceUtil:"<<__LINE__<<" Null input image list encountered! ";
      throw ossimException(xmsg.str());
   }

   // Generate bitmap for input image:
   m_productBitmap = convertToBitmap(m_imgLayers[0].get());

   // If there is a mask, generate a bitmap for it as well:
   m_maskBitmap = 0;
   if (m_imgLayers.size() == 2)
   {
      m_maskBitmap = convertToBitmap(m_imgLayers[1].get());
      m_mode = LINESTRING;
   }

   // Perform vectorization:
   potrace_param_t* potraceParam = potrace_param_default();
   potraceParam->turdsize = m_turdSize;
   potraceParam->alphamax = m_alphamax;
   potrace_state_t* potraceOutput = potrace_trace(potraceParam, m_productBitmap);
   if (!potraceOutput)
   {
      xmsg <<"ossimPotraceUtil:"<<__LINE__<<" Null pointer returned from potrace_trace!";
      throw ossimException(xmsg.str());
   }

   if (m_mode == LINESTRING)
      transformLineStrings(potraceOutput);
   else
      transformPolygons(potraceOutput);

   // Write to output vector file:
   if (!writeGeoJSON(potraceOutput->plist))
      return false;

   // Release memory:
   //potrace_state_free(potraceOutput);
   //free(potraceBitmap->map);
   free(potraceParam);

   return true;
}

void ossimPotraceUtil::transformLineStrings(potrace_state_t* potraceOutput)
{
   ossimIrect rect;
   m_geom->getBoundingRect(rect);
   rect.expand(ossimIpt(-1,-1));

   // Vector coordinates are in image space. Need geom to convert to geographic lat/lon:
   potrace_path_t* path = potraceOutput->plist;
   ossimDpt imgPt;
   ossimGpt gndPt;

   while (path)
   {
      int segment = 0;
      while (segment<path->curve.n)
      {
         // Convert up to 3 possible vertices per segment
         for (int v=0; v<3; ++v)
         {
            if ((v == 0) && (path->curve.tag[segment] = POTRACE_CORNER))
               continue;

            imgPt.x = path->curve.c[segment][v].x;
            imgPt.y = path->curve.c[segment][v].y;

            // Potrace reasonably assumes that active pixels on edge need to be bound by a polygon.
            // Override this behavior by avoiding consideration of edge pixels. This involves
            // editing the potrace paths returned by the trace algorithm, splitting the paths into
            // separate independent paths when they encounter an edge. Any segments lying near the
            // edge of the image rectangle will be removed from the path list.
            if ( !rect.pointWithin(imgPt) || pixelIsMasked(imgPt, m_maskBitmap))
            {
               // Need to ignore this segment and split the containing path into two separate paths:
               potrace_path_t* new_path = new potrace_path_t;
               new_path->area = 1;
               new_path->sign = 1;
               new_path->next = path->next;
               new_path->sibling = 0;
               new_path->childlist = 0;
               new_path->priv = 0;
               new_path->curve.n = path->curve.n - segment - 1;
               new_path->curve.c = new potrace_dpoint_t[new_path->curve.n][3];
               new_path->curve.tag = new int[new_path->curve.n];
               for (int new_seg=0; new_seg<new_path->curve.n; new_seg++)
               {
                  for (int u=0; u<3; ++u)
                  {
                     new_path->curve.c[new_seg][u].x = path->curve.c[new_seg+segment+1][u].x;
                     new_path->curve.c[new_seg][u].y = path->curve.c[new_seg+segment+1][u].y;
                  }
                  new_path->curve.tag[new_seg] = path->curve.tag[new_seg+segment+1];
               }
               if (segment == 0)
               {
                  if (path == potraceOutput->plist) // First in the trace list?
                     potraceOutput->plist = new_path;
                  delete path;
                  path = new_path;
                  segment = -1;
               }
               else
               {
                  path->curve.tag[segment-1] = POTRACE_ENDPOINT;
                  path->next = new_path; // Link in the new path
                  path->curve.n = segment;
               }

               break; // out of vertex loop
            }

            // Good segment, reproject to geographic:
            m_geom->localToWorld(imgPt, gndPt);
            path->curve.c[segment][v].x = gndPt.lon;
            path->curve.c[segment][v].y = gndPt.lat;
         }
         ++segment;
      }
      path = path->next;
   }
}

void ossimPotraceUtil::transformPolygons(potrace_state_t* potraceOutput)
{
   // Vector coordinates are in image space. Need geom to convert to geographic lat/lon:
   potrace_path_t* path = potraceOutput->plist;
   ossimDpt imgPt;
   ossimGpt gndPt;
   while (path)
   {
      for (int segment=0; segment<path->curve.n;)
      {
         // Convert up to 3 possible vertices per segment
         for (int v=0; v<3; ++v)
         {
            if ((v == 0) && (path->curve.tag[segment] = POTRACE_CORNER))
               continue;

            imgPt.x = path->curve.c[segment][v].x;
            imgPt.y = path->curve.c[segment][v].y;

            m_geom->localToWorld(imgPt, gndPt);
            path->curve.c[segment][v].x = gndPt.lon;
            path->curve.c[segment][v].y = gndPt.lat;
         }
         ++segment;
      }
      path = path->next;
   }
}

void ossimPotraceUtil::getKwlTemplate(ossimKeywordlist& kwl)
{
   ostringstream value;
   value << "polygon|linestring (optional, defaults to polygon)";
   kwl.addPair(MODE_KW, value.str());

   value.clear();
   value<<"<float> (optional, defaults to "<<m_alphamax<<")";
   kwl.addPair(ALPHAMAX_KW, value.str());

   value.clear();
   value<<"<int> (optional, defaults to "<<m_turdSize<<")";
   kwl.addPair(TURDSIZE_KW, value.str());

   kwl.add("image_file0", "<input-raster-file>");
   kwl.add("image_file1", "<mask-file> (optional)");
   kwl.add(ossimKeywordNames::OUTPUT_FILE_KW, "<output-vector-file>");
}

potrace_bitmap_t* ossimPotraceUtil::convertToBitmap(ossimImageSource* raster)
{
   potrace_bitmap_t* potraceBitmap = new potrace_bitmap_t;

   // Determine output bitmap size to even word boundary:
   ossimIrect rect;
   raster->getImageGeometry()->getBoundingRect(rect);
   potraceBitmap->w = rect.width();
   potraceBitmap->h = rect.height();
   int pixelsPerWord = 8 * sizeof(int*);
   potraceBitmap->dy = (int) ceil((double)rect.width()/pixelsPerWord);

   // Allocate the bitmap memory:
   unsigned long bufLength = potraceBitmap->dy*potraceBitmap->h;
   potraceBitmap->map = new potrace_word[bufLength];

   // Prepare to sequence over all input image tiles and fill the bitmap image:
   ossimRefPtr<ossimImageSourceSequencer> sequencer = new ossimImageSourceSequencer(raster);
   ossimRefPtr<ossimImageData> tile = sequencer->getNextTile();
   double null_pix = tile->getNullPix(0);
   unsigned long offset=0;
   ossimIpt pt_ul, pt, pt_lr;

   // Loop to fill bitmap buffer:
   while (tile.valid())
   {
      pt_ul = tile->getOrigin();
      pt_lr.x = pt_ul.x + tile->getWidth();
      pt_lr.y = pt_ul.y + tile->getHeight();
      if (pt_lr.x > rect.lr().x)
         pt_lr.x = rect.lr().x;
      if (pt_lr.y > rect.lr().y)
         pt_lr.y = rect.lr().y;

      // Nested loops over all pixels in input tile:
      for (pt.y=pt_ul.y; pt.y < pt_lr.y; ++pt.y)
      {
         offset = pt_ul.x/pixelsPerWord + pt.y*potraceBitmap->dy;
         for (pt.x=pt_ul.x; pt.x<pt_lr.x; )
         {
            // Loop to pack a word buffer with pixel on (non-null) or off bit in proper positions:
            unsigned long wordBuf = 0;
            for (int bitpos=pixelsPerWord-1; bitpos>=0; --bitpos)
            {
               unsigned long pixel = (tile->getPix(pt) != null_pix) ? 1 : 0;
               wordBuf |= pixel << bitpos;
               ++pt.x;
               if (pt.x >= pt_lr.x)
                  break;
            }
            potraceBitmap->map[offset++] = wordBuf;
         }
      }

      tile = sequencer->getNextTile();
   }

#if 1
   FILE* pbm = fopen("TEMP.pbm", "w");
   potrace_writepbm(pbm, potraceBitmap);
#endif

   return potraceBitmap;
}

bool ossimPotraceUtil::pixelIsMasked(const ossimIpt& image_pt, potrace_bitmap_t* bitmap) const
{
   if (bitmap == 0)
      return false;

   int pixelsPerWord = 8 * sizeof(int*);
   unsigned long offset = image_pt.x/pixelsPerWord + image_pt.y*bitmap->dy;
   int shift = pixelsPerWord - (image_pt.x % pixelsPerWord) - 1;
   unsigned long wordBuf = bitmap->map[offset];
   unsigned long shifted = wordBuf >> shift;
   shifted = shifted & 1;
   if (shifted == 0)
      return true;
   return false;
}

bool ossimPotraceUtil::writeGeoJSON(potrace_path_t* vectorList)
{
   ostringstream xmsg;

   FILE* outFile = fopen(m_productFilename.chars(), "w");
   if (!outFile)
   {
      xmsg <<"ossimPotraceUtil:"<<__LINE__<<" Could not open output file <"<<m_productFilename
            <<"> for writing.";
      throw ossimException(xmsg.str());
   }

   potrace_geojson(outFile, vectorList, (int) (m_mode == POLYGON));
   fclose(outFile);

   if (m_outputToConsole && m_consoleStream)
   {
      ifstream vectorFile (m_productFilename.chars());
      if (vectorFile.fail())
      {
         xmsg <<"ossimPotraceUtil:"<<__LINE__<<" Error encountered opening temporary vector file at: "
               "<"<<m_productFilename<<">.";
         throw ossimException(xmsg.str());
      }

      *m_consoleStream << vectorFile.rdbuf();
      vectorFile.close();
   }

   return true;
}

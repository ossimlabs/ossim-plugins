//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimException.h>
#include <potrace/src/ossimPotraceTool.h>
#include <sstream>

using namespace std;

const char* ossimPotraceTool::DESCRIPTION =
      "Computes vector representation of input raster image.";

static const string MODE_KW = "mode";
static const string ALPHAMAX_KW = "alphamax";
static const string TURDSIZE_KW = "turdsize";

ossimPotraceTool::ossimPotraceTool()
:  m_mode (LINESTRING),
   m_alphamax (1.0),
   m_turdSize (4),
   m_outputToConsole(false),
   m_maskBitmap (0),
   m_productBitmap (0)
{
}

ossimPotraceTool::~ossimPotraceTool()
{
   delete m_productBitmap;
   if (m_maskBitmap)
      delete m_maskBitmap;
}

void ossimPotraceTool::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options. Don't include ossimChipProcUtil options as not appropriate.
   ossimTool::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " potrace [options] <input-raster-file> [<output-vector-file>]";
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

bool ossimPotraceTool::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);

   if (!ossimTool::initialize(ap))
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

void ossimPotraceTool::initialize(const ossimKeywordlist& kwl)
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
      xmsg <<"ossimPotraceTool:"<<__LINE__<<" Unallowed mode requested: <"<<value<<">."
            <<endl;
      throw ossimException(xmsg.str());
   }

   ossimChipProcTool::initialize(kwl);
}

void ossimPotraceTool::initProcessingChain()
{
   // Nothing to do.
}

void ossimPotraceTool::finalizeChain()
{
   // Do nothing and avoid ossimChipProcUtil from doing its standard stuff.
}

bool ossimPotraceTool::execute()
{
   ostringstream xmsg;

   if (m_imgLayers.empty() || !m_geom.valid())
   {
      xmsg<<"ossimPotraceTool:"<<__LINE__<<" Null input image list encountered! ";
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
      xmsg <<"ossimPotraceTool:"<<__LINE__<<" Null pointer returned from potrace_trace!";
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

void ossimPotraceTool::transformLineStrings(potrace_state_t* potraceOutput)
{
   // This is a fairly complex process because potrace assumes all paths are closed, i.e., the
   // last vertex of a path is the same as the first. For linestring mode, this is not always
   // true since the path may hit the border or masked region, in which case it needs to be
   // split into two or more paths.

   ossimIrect rect;
   m_geom->getBoundingRect(rect);
   rect.expand(ossimIpt(-1,-1));

   // Vector coordinates are in image space. Need geom to convert to geographic lat/lon:
   potrace_path_t* path = potraceOutput->plist;
   potrace_path_t* pathToDelete;
   ossimDpt imgPt;
   vector<Path*> originalPaths;
   vector<Path*> adjustedPaths;

   // Populate the std::vector with original list of potrace paths:
   while (path)
   {
      //cout << "\nProcessing path "<<(long)path<<endl;
      if (path->curve.n > 0)
      {
         // Start by converting all paths to C++ data structures:
         Path* p = new Path;
         for (int segment=0; segment<path->curve.n; ++segment)
         {
            //cout << "  Processing segment "<<segment;
            p->addPotraceCurve(path->curve, segment);
         }
         originalPaths.push_back(p);
      }
      potrace_path_t* delete_this_path = path;
      path = path->next;
      free(delete_this_path); // Don't need old path anymore
   }

   // All paths are now represented by line segments in convenient C++ vector structure. Now
   // need to filter out masked points, splitting up paths as they encounter the masked regions:
   for (size_t i=0; i<originalPaths.size(); ++i)
   {
      Path* original = originalPaths[i];
      bool outside = false;

      //cout << "\nProcessing originalPath["<<i<<"] with numVertices="<<original->vertices.size()<<endl;
      Path* adjusted = 0;
      for (size_t v=0; v<original->vertices.size(); ++v)
      {
         imgPt = original->vertices[v];
         if ( rect.pointWithin(imgPt) && !pixelIsMasked(imgPt, m_maskBitmap))
         {
            if (!adjusted)
            {
               adjusted = new Path;
               adjustedPaths.push_back(adjusted);
               //cout << "  Creating adjusted="<<(long)adjusted<<endl;
            }
            adjusted->vertices.push_back(imgPt);
            //cout << "  Adding to adjusted, imgPt:"<<imgPt<<endl;

            if (outside)
            {
               // Coming in from the outside, so not closed:
               adjusted->closed = false;
               outside = false;
            }
         }
         else if (!outside)
         {
            // Just went outside. Close this adjusted path and start a new one:
            //cout << "  Hit outside, end of adjusted:"<<(long)adjusted<<endl;
            if (adjusted)
               adjusted->closed = false;
            adjusted = 0;
            outside = true;
         }
      }
   }

   // The adjustedPaths list contains only vertices inside the ROI. Now need to move back into
   // potrace space to prepare potrace data structures for geojson output:
   ossimGpt gndPt;
   potrace_path_t* previous_path = 0;
   vector<Path*>::iterator path_iter = adjustedPaths.begin();
   while (path_iter != adjustedPaths.end())
   {
      // Don't bother with single-vertex "paths":
      Path* adjusted = *path_iter;
      if (adjusted->vertices.size() < 3)
      {
         ++path_iter;
         continue;
      }

      // Convert image point vertices to ground points and repopulate using potrace
      // datastructures:
      //cout << "\nProcessing adjustedPaths["<<i<<"] with numVertices="<<adjusted->vertices.size()<<endl;
      path = new potrace_path_t;
      //cout << "  Created potrace path:"<<(long)path<<endl;
      if (previous_path)
         previous_path->next = path;
      else
         potraceOutput->plist = path;

      int num_segments = (adjusted->vertices.size()+1)/2;
      path->curve.n = num_segments;
      path->curve.c = new potrace_dpoint_t[num_segments][3];
      path->curve.tag = new int[num_segments];
      path->area = 1;
      path->sign = 1;
      path->sibling = 0;
      path->childlist = 0;
      path->priv = 0;
      path->next = 0;

      // Loop to transfer the vertices to the potrace structure, transforming to lat, lon:
      int segment = 0;
      int adj_v = 0;
      while (adj_v < adjusted->vertices.size())
      {
         path->curve.tag[segment] = POTRACE_CORNER;

         // Transform:
         imgPt = adjusted->vertices[adj_v++];
         m_geom->localToWorld(imgPt, gndPt);
         path->curve.c[segment][1].x = gndPt.lon;
         path->curve.c[segment][1].y = gndPt.lat;
         //cout << "  Inserted ["<<adj_v-1<<"], imgPt:"<<imgPt<<" --> ("<<gndPt.lon<<", "<<gndPt.lat<<")"<<endl;

         if (adj_v == adjusted->vertices.size())
         {
            //cout << "  Inserted above as last"<<endl;

            path->curve.c[segment][2].x = gndPt.lon;
            path->curve.c[segment][2].y = gndPt.lat;
         }
         else
         {
            imgPt = adjusted->vertices[adj_v++];
            m_geom->localToWorld(imgPt, gndPt);
            path->curve.c[segment][2].x = gndPt.lon;
            path->curve.c[segment][2].y = gndPt.lat;
            //cout << "  Inserted ["<<adj_v-1<<"], imgPt:"<<imgPt<<" --> ("<<gndPt.lon<<", "<<gndPt.lat<<")"<<endl;
         }

         // Mark the last point as a line segment endpoint to avoid closure:
         if (adj_v == adjusted->vertices.size() && !adjusted->closed)
            path->curve.tag[segment] = POTRACE_ENDPOINT;

         //cout << "  Finished segment:"<<segment<<", tag="<<path->curve.tag[segment]<<endl;
         ++segment;
      }
      previous_path = path;

      // Don't need the adjusted path anymore:
      delete adjusted;
      adjusted = 0;
      ++path_iter;
   }
}

void ossimPotraceTool::transformPolygons(potrace_state_t* potraceOutput)
{
   // Vector coordinates are in image space. Need geom to convert to geographic lat/lon:
   potrace_path_t* path = potraceOutput->plist;
   ossimDpt imgPt;
   ossimGpt gndPt;
   while (path)
   {
      for (int segment=0; segment<path->curve.n;)
      {
         //cout << "\nWorking on path="<<(long)path<<" segment="<<segment<<", tag="<<path->curve.tag[segment]<<endl;
         // Convert up to 3 possible vertices per segment
         for (int v=0; v<3; ++v)
         {
            if ((v == 0) && (path->curve.tag[segment] == POTRACE_CORNER))
            {
               //cout << "Hit corner"<<endl;
               continue;
            }

            imgPt.x = path->curve.c[segment][v].x;
            imgPt.y = path->curve.c[segment][v].y;
            //cout << "imgPt["<<v<<"] = "<<imgPt<<endl;

            m_geom->localToWorld(imgPt, gndPt);
            path->curve.c[segment][v].x = gndPt.lon;
            path->curve.c[segment][v].y = gndPt.lat;
         }
         ++segment;
      }
      path = path->next;
   }
}

void ossimPotraceTool::getKwlTemplate(ossimKeywordlist& kwl)
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

potrace_bitmap_t* ossimPotraceTool::convertToBitmap(ossimImageSource* raster)
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

#if 0
   FILE* pbm = fopen("TEMP.pbm", "w");
   potrace_writepbm(pbm, potraceBitmap);
#endif

   return potraceBitmap;
}

bool ossimPotraceTool::pixelIsMasked(const ossimIpt& image_pt, potrace_bitmap_t* bitmap) const
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

bool ossimPotraceTool::writeGeoJSON(potrace_path_t* vectorList)
{
   ostringstream xmsg;

   FILE* outFile = fopen(m_productFilename.chars(), "w");
   if (!outFile)
   {
      xmsg <<"ossimPotraceTool:"<<__LINE__<<" Could not open output file <"<<m_productFilename
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
         xmsg <<"ossimPotraceTool:"<<__LINE__<<" Error encountered opening temporary vector file at: "
               "<"<<m_productFilename<<">.";
         throw ossimException(xmsg.str());
      }

      *m_consoleStream << vectorFile.rdbuf();
      vectorFile.close();
   }

   return true;
}

ossimPotraceTool::Path::~Path()
{
   vertices.clear();
}


void ossimPotraceTool::Path::addPotraceCurve(potrace_curve_t& curve, int segment)
{
   potrace_dpoint_t *c = curve.c[segment];
   if (curve.tag[segment] == POTRACE_CORNER)
   {
      // Just add next connected line segment
      ossimDpt v1 (c[1].x, c[1].y);
      ossimDpt v2 (c[2].x, c[2].y);
      vertices.push_back(v1);
      vertices.push_back(v2);
      //cout <<"    Added corners    v1:"<<v1<<endl;
      //cout <<"                     v2:"<<v2<<endl;
   }
   else
   {
      // Need to convert bezier curve representation to line segments:
      ossimDpt v0;
      ossimDpt v1 (c[0].x, c[0].y);
      ossimDpt v2 (c[1].x, c[1].y);
      ossimDpt v3 (c[2].x, c[2].y);

      // Need the first vertex from the previous segment if available:
      if (segment == 0)
         v0 = v1;
      else
      {
         c = curve.c[segment-1];
         v0.x = c[2].x;
         v0.y = c[2].y;
      }
      //cout <<"    Processing curve v0:"<<v0<<endl;
      //cout <<"                     v1:"<<v1<<endl;
      //cout <<"                     v2:"<<v2<<endl;
      //cout <<"                     v3:"<<v3<<endl;

      // Now loop to extract line segments from bezier curve:
      double step, t, s;
      ossimDpt vertex;
      step = 1.0 / 8.0;
      t = step;
      for (int i=0; i<8; i++)
      {
         s = 1.0 - t;
         vertex.x = s*s*s*v0.x + 3*(s*s*t)*v1.x + 3*(t*t*s)*v2.x + t*t*t*v3.x;
         vertex.y = s*s*s*v0.y + 3*(s*s*t)*v1.y + 3*(t*t*s)*v2.y + t*t*t*v3.y;
         vertices.push_back(vertex);
         t += step;
         //cout <<"    added vertex:"<<vertex<<endl;
      }
   }
}


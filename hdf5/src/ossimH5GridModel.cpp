//*****************************************************************************
// FILE: ossimH5GridModel.cc
//
// License:  See LICENSE.txt file in the top level directory.
//
// AUTHOR: David Burken
//
// Copied from Mingjie Su's ossimHdfGridModel.
//
// DESCRIPTION:
//   Contains implementation of class ossimH5GridModel. This is an
//   implementation of an interpolation sensor model. 
//
//   IMPORTANT: The lat/lon grid is for ground points on the ellipsoid.
//   The dLat/dHgt and dLon/dHgt partials therefore are used against
//   elevations relative to the ellipsoid.
//
//*****************************************************************************
//  $Id$

#include "ossimH5GridModel.h"
#include "ossimH5Util.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>

#include <hdf5.h>
#include <H5Cpp.h>

#include <sstream>

static ossimTrace traceDebug("ossimH5GridModel:debug");

static const std::string CROSSES_DATELINE_KW = "crosses_dateline";
static const std::string GROUND_POLYGON_KW   = "ground_polygon"; 
static const std::string WKT_FOOTPRINT_KW    = "wkt_footprint";

RTTI_DEF1(ossimH5GridModel, "ossimH5GridModel", ossimCoarseGridModel);

ossimH5GridModel::ossimH5GridModel()
   :
   ossimCoarseGridModel(),
   m_crossesDateline(false),
   m_boundGndPolygon()
{
   theLatGrid.setDomainType(ossimDblGrid::SAWTOOTH_90);
}

ossimH5GridModel::~ossimH5GridModel()
{
}

bool ossimH5GridModel::setGridNodes( H5::DataSet* latDataSet,
                                     H5::DataSet* lonDataSet,
                                     const ossimIrect& validRect )
{
   bool status = false;

   if ( latDataSet && lonDataSet )
   {
      m_crossesDateline = ossim_hdf5::crossesDateline( *lonDataSet, validRect );
      
      if ( m_crossesDateline )
      {
         theLonGrid.setDomainType(ossimDblGrid::CONTINUOUS);
      }
      else
      {
         // Add 360 to any negative numbers.
         theLonGrid.setDomainType(ossimDblGrid::WRAP_180);
      }

      // Get dataspace of the dataset.
      H5::DataSpace latDataSpace = latDataSet->getSpace();
      H5::DataSpace lonDataSpace = lonDataSet->getSpace();
      const ossim_int32 LAT_DIM_COUNT = latDataSpace.getSimpleExtentNdims();
      const ossim_int32 LON_DIM_COUNT = lonDataSpace.getSimpleExtentNdims();
      
      // Number of dimensions of the input dataspace:
      if ( ( LAT_DIM_COUNT == 2 ) && ( LON_DIM_COUNT == 2 ) )
      {
         const ossim_uint32 ROWS = validRect.height();
         const ossim_uint32 COLS = validRect.width();
         const ossim_uint32 GRID_SIZE = 4; // Only grab every 4th value.

         //---
         // Get the extents:
         // dimsOut[0] is height, dimsOut[1] is width:
         //---
         std::vector<hsize_t> latDimsOut(LAT_DIM_COUNT);
         latDataSpace.getSimpleExtentDims( &latDimsOut.front(), 0 );
         std::vector<hsize_t> lonDimsOut(LON_DIM_COUNT);
         lonDataSpace.getSimpleExtentDims( &lonDimsOut.front(), 0 );
         
         // Verify valid rect within our bounds:
         if ( (ROWS <= latDimsOut[0] ) && (ROWS <= lonDimsOut[0] ) &&
              (COLS <= latDimsOut[1] ) && (COLS <= lonDimsOut[1] ) )
         {
            //----
            // Initialize the ossimDblGrids:
            //---
            ossimDpt dspacing (GRID_SIZE, GRID_SIZE);
            
            ossim_uint32 gridRows = ROWS / GRID_SIZE + 1;
            ossim_uint32 gridCols = COLS / GRID_SIZE + 1;
            
            // Round up if size doesn't fall on end pixel.
            if ( ROWS % GRID_SIZE) ++gridRows;
            if ( COLS % GRID_SIZE) ++gridCols;

            ossimIpt gridSize (gridCols, gridRows);
            
            // The grid as used in base class, has UV-space always at 0,0 origin            
            ossimDpt gridOrigin(0.0,0.0);

            const ossim_float64 NULL_VALUE = -999.0;
            
            theLatGrid.setNullValue(ossim::nan());
            theLonGrid.setNullValue(ossim::nan());
            theLatGrid.initialize(gridSize, gridOrigin, dspacing);
            theLonGrid.initialize(gridSize, gridOrigin, dspacing);            
            
            std::vector<hsize_t> inputCount(LAT_DIM_COUNT);
            std::vector<hsize_t> inputOffset(LAT_DIM_COUNT);
            
            inputOffset[0] = 0; // row is set below.
            inputOffset[1] = validRect.ul().x; // col
            
            inputCount[0] = 1; // row
            inputCount[1] = (hsize_t)COLS; // col
            
            // Output dataspace dimensions. Reading a line at a time.
            const ossim_int32 OUT_DIM_COUNT = 3;
            std::vector<hsize_t> outputCount(OUT_DIM_COUNT);
            outputCount[0] = 1;    // band
            outputCount[1] = 1;    // row
            outputCount[2] = COLS; // col
            
            // Output dataspace offset.
            std::vector<hsize_t> outputOffset(OUT_DIM_COUNT);
            outputOffset[0] = 0;
            outputOffset[1] = 0;
            outputOffset[2] = 0;
            
            ossimScalarType scalar = ossim_hdf5::getScalarType( latDataSet );
            if ( scalar == OSSIM_FLOAT32 )
            {
               // Set the return status to true if we get here...
               status = true;
               
               // See if we need to swap bytes:
               ossimEndian* endian = 0;
               if ( ( ossim::byteOrder() != ossim_hdf5::getByteOrder( latDataSet ) ) )
               {
                  endian = new ossimEndian();
               }
               
               // Native type:
               H5::DataType latDataType = latDataSet->getDataType();
               H5::DataType lonDataType = lonDataSet->getDataType();

               // Output dataspace always the same, width of one line.
               H5::DataSpace bufferDataSpace( OUT_DIM_COUNT, &outputCount.front());
               bufferDataSpace.selectHyperslab( H5S_SELECT_SET,
                                                &outputCount.front(),
                                                &outputOffset.front() );

               //  Arrays to hold a single line of latitude longitude values.
               vector<ossim_float32> latValue(COLS);
               vector<ossim_float32> lonValue(COLS);
               hsize_t row = 0;

               // Line loop:
               for ( ossim_uint32 y = 0; y < gridRows; ++y )
               {
                  // row = line in image space
                  row = y*GRID_SIZE;

                  if ( row < ROWS )
                  {
                     inputOffset[0] = row + validRect.ul().y;

                     latDataSpace.selectHyperslab( H5S_SELECT_SET,
                                                   &inputCount.front(),
                                                   &inputOffset.front() );
                     lonDataSpace.selectHyperslab( H5S_SELECT_SET,
                                                   &inputCount.front(),
                                                   &inputOffset.front() );
                  
                     // Read data from file into the buffer.
                     latDataSet->read( &(latValue.front()), latDataType,
                                       bufferDataSpace, latDataSpace );
                     lonDataSet->read( &(lonValue.front()), lonDataType,
                                       bufferDataSpace, lonDataSpace );
                  
                     if ( endian )
                     {
                        // If the endian pointer is initialized(not zero) swap the bytes.
                        endian->swap( &(latValue.front()), COLS );
                        endian->swap( &(lonValue.front()), COLS );  
                     }
                  
                     // Sample loop:
                     hsize_t col = 0;
                     
                     for ( ossim_uint32 x = 0; x < gridCols; ++x )
                     {
                        ossim_float32 lat = ossim::nan();
                        ossim_float32 lon = ossim::nan();
                        
                        // col = sample in image space
                        col = x*GRID_SIZE;

                        if ( col < COLS )
                        {
                           if ( (latValue[col] > NULL_VALUE)&&(lonValue[col] > NULL_VALUE) )
                           {
                              lat = latValue[col];
                              lon = lonValue[col];
                              if ( m_crossesDateline )
                              {
                                 if ( lon < 0.0 ) lon += 360;
                              }
                           }
                           else // Nulls in grid!
                           {
                              std::string errMsg =
                                 "ossimH5GridModel::setGridNodes encountered nans!";
                              throw ossimException(errMsg); 
                           }
                        }
                        else // Last column is outside of image bounds.
                        {
                           // Get the last two latitude values:
                           ossim_float32 lat1 = theLatGrid.getNode( x-2, y );
                           ossim_float32 lat2 = theLatGrid.getNode( x-1, y );

                           // Get the last two longitude values
                           ossim_float32 lon1 = theLonGrid.getNode( x-2, y );
                           ossim_float32 lon2 = theLonGrid.getNode( x-1, y );
                           
                           if ( ( lat1 > NULL_VALUE ) && ( lat2 > NULL_VALUE ) )
                           {
                              // Delta between last two latitude grid values.
                              ossim_float32 latSpacing = lat2 - lat1;
                           
                              // Compute:
                              lat = lat2 + latSpacing;
                           }
                           else // Nulls in grid!
                           {
                              std::string errMsg =
                                 "ossimH5GridModel::setGridNodes encountered nans!";
                              throw ossimException(errMsg);
                           }

                           if ( ( lon1 > NULL_VALUE ) && ( lon2 > NULL_VALUE ) )
                           {
                              // Delta between last two longitude values.
                              ossim_float32 lonSpacing = lon2 - lon1;
                           
                              // Compute:
                              lon = lon2 + lonSpacing;

                              // Check for wrap:
                              if ( !m_crossesDateline && ( lon > 180 ) )
                              {
                                 lon -= 360.0;
                              }
                           }
                           else // Nulls in grid!
                           {
                              std::string errMsg =
                                 "ossimH5GridModel::setGridNodes encountered nans!";
                              throw ossimException(errMsg);
                           }

#if 0 /* Please leave for debug. (drb) */                        
                           cout << "lat1: " << lat1 << " lat2 " << lat2
                                << " lon1 " << lon1  << " lon2 " << lon2
                                << "\n";
#endif
                        }
                        
                        // Assign the latitude and longitude.
                        theLatGrid.setNode( x, y, lat );
                        theLonGrid.setNode( x, y, lon );
                        
#if 0 /* Please leave for debug. (drb) */ 
                        cout << "x,y,col,row,lat,lon:" << x << "," << y << ","
                             << col << "," << row << ","
                             << theLatGrid.getNode(x, y)
                             << "," << theLonGrid.getNode( x, y) << "\n";
#endif
                        
                     } // End sample loop.
                     
                  }
                  else // Row is outside of image bounds:
                  {
                     // Sample loop:
                     for ( ossim_uint32 x = 0; x < gridCols; ++x )
                     {
                        ossim_float32 lat        = ossim::nan();
                        ossim_float32 lon        = ossim::nan();
                        
                        ossim_float32 lat1 = theLatGrid.getNode( x, y-2 );
                        ossim_float32 lat2 = theLatGrid.getNode( x, y-1 );
                        ossim_float32 lon1 = theLonGrid.getNode( x, y-2 );
                        ossim_float32 lon2 = theLonGrid.getNode( x, y-1 );
                        
                        if ( ( lon1 > NULL_VALUE ) && ( lon2 > NULL_VALUE ) )
                        {
                           // Delta between last two longitude values.
                           ossim_float32 lonSpacing = lon2 - lon1;
                           
                           // Compute:
                           lon = lon2 + lonSpacing;
                           
                           // Check for wrap:
                           if ( !m_crossesDateline && ( lon > 180 ) )
                           {
                              lon -= 360.0;
                           }
                        }
                        else // Nulls in grid!
                        {
                           std::string errMsg =
                              "ossimH5GridModel::setGridNodes encountered nans!";
                           throw ossimException(errMsg);
                        }
                        
                        if ( ( lat1 > NULL_VALUE ) && ( lat2 > NULL_VALUE ) )
                        {
                           // Delta between last two latitude values.
                           ossim_float32 latSpacing = lat2 - lat1;

                           lat = lat2 + latSpacing;
                        }
                        else // Nulls in grid!
                        {
                           std::string errMsg =
                              "ossimH5GridModel::setGridNodes encountered nans!";
                           throw ossimException(errMsg);
                        }
                        
#if 0 /* Please leave for debug. (drb) */
                        hsize_t col = x*GRID_SIZE; // Sample in image space
                        cout << "lat1: " << lat1 << " lat2 " << lat2
                             << " lon1 " << lon1  << " lon2 " << lon2
                             << "\nx,y,col,row,lat,lon:" << x << "," << y << ","
                             << col << "," << row << "," << lat << "," << lon << "\n";
#endif
                        // Assign the latitude::
                        theLatGrid.setNode( x, y, lat );

                        // Assign the longitude.
                        theLonGrid.setNode( x, y, lon );
                     
                     } // End sample loop.
                  
                  } // Matches if ( row < imageRows ){...}else{
                  
               } // End line loop.

               latDataSpace.close();
               lonDataSpace.close();

               if ( status )
               {
                  theSeedFunction = ossim_hdf5::getBilinearProjection(
                     *latDataSet,*lonDataSet, validRect );
                  
                  // Bileaner projection to handle
                  ossimDrect imageRect(validRect);
                  initializeModelParams(imageRect);

                  // debugDump();
               }

               if ( endian )
               {
                  delete endian;
                  endian = 0;
               }
               
            } // Matches: if ( scalar == OSSIM_FLOAT32 )
            
         } // Matches: if ( (latDimsOut[0] == imageRows) ...
         
      } // Matches: if ( ( LAT_DIM_COUNT == 2 ) ...

   } // Matches: if ( latDataSet && lonDataSet

   return status;
   
} // End: bool ossimH5GridModel::setGridNodes( H5::DataSet* latDataSet, ... )

bool ossimH5GridModel::saveState(ossimKeywordlist& kwl, const char* prefix) const
{
   // Base save state.  This will save the "type" key to "ossimH5GridModel".
   bool status = ossimCoarseGridModel::saveState( kwl, prefix );
   
   if ( status )
   {
      std::string myPrefix = ( prefix ? prefix: "" );
      std::string value;

      // m_crossesDateline:
      value = ossimString::toString(m_crossesDateline).string();
      kwl.addPair( myPrefix,CROSSES_DATELINE_KW, value, true );

      // Computed wkt footprint:
      if ( getWktFootprint( value ) )
      {   
         kwl.addPair( myPrefix, WKT_FOOTPRINT_KW, value, true );
      }      

      // m_boundGndPolygon:
      if ( m_boundGndPolygon.getNumberOfVertices() )
      {
         std::string polyPrefix = myPrefix;
         polyPrefix += GROUND_POLYGON_KW;
         polyPrefix += ".";         
         m_boundGndPolygon.saveState( kwl, polyPrefix.c_str() );
      }

      //---
      // theSeedFunction: This is in the base class ossimSensorModel but is not
      // in the ossimSensorModel::saveState so do it here.
      //---
      if ( theSeedFunction.valid() )
      {
         ossimString seedPrefix = myPrefix;
         seedPrefix += "seed_projection.";
         status = theSeedFunction->saveState(kwl, seedPrefix.c_str());
      }
   }

   return status;
}     

bool ossimH5GridModel::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   bool result = false;
   
   std::string myPrefix = ( prefix ? prefix: "" );

   // Look for type key:
   std::string key = "type";
   std::string value = kwl.findKey( myPrefix, key );

   if ( value.size() )
   {
      // ossimHdfGridModel included for backward compatibility.
      if ( ( value == "ossimH5GridModel" ) || ( value == "ossimHdfGridModel" ) )
      {
         //---
         // theSeedFunction: This is in the base class ossimSensorModel but is not
         // in the ossimSensorModel::loadState so do it here.
         //---
         std::string seedPrefix = myPrefix;
         seedPrefix += "seed_projection.";
         value = kwl.findKey( seedPrefix, key );
         if ( value.size() )
         {
            // Only do expensive factory call if key is found...
            theSeedFunction = ossimProjectionFactoryRegistry::instance()->
               createProjection(kwl, seedPrefix.c_str());         
         }
         
         // m_crossesDateline:
         value = kwl.findKey( myPrefix, CROSSES_DATELINE_KW );
         if ( value.size() )
         {
            m_crossesDateline = ossimString(value).toBool();
         }

         // m_boundGndPolygon:
         std::string polyPrefix = myPrefix;
         polyPrefix += GROUND_POLYGON_KW;
         polyPrefix += ".";
         m_boundGndPolygon.clear();
         m_boundGndPolygon.loadState( kwl, polyPrefix.c_str() );
      
         // Make a copy of kwl so we can change type.
         ossimKeywordlist myKwl = kwl;
         value = "ossimCoarseGridModel";
         myKwl.addPair( myPrefix, key, value, true );

         // Load the state of base class.
         result = ossimCoarseGridModel::loadState( myKwl, prefix );
      }
   }
   
   return result;
   
} // End: ossimH5GridModel::loadState( ... )

//---------------------------------------------------------------------------**
//  METHOD: ossimSensorModel::worldToLineSample()
//  
//  Performs forward projection of ground point to image space.
//  
//---------------------------------------------------------------------------**
void ossimH5GridModel::worldToLineSample(const ossimGpt& worldPoint,
                                         ossimDpt&       ip) const
{
   static const double PIXEL_THRESHOLD    = .1; // acceptable pixel error
   static const int    MAX_NUM_ITERATIONS = 20;

   if(worldPoint.isLatNan() || worldPoint.isLonNan())
   {
      ip.makeNan();
      return;
   }
      
   //---
   // First check if the world point is inside bounding rectangle:
   //---
   int iters = 0;
   ossimDpt wdp (worldPoint);
   if ( m_crossesDateline && (wdp.x < 0.0) )
   {
      // Longitude points stored between 0 and 360.
      wdp.x += 360.0;
   }
   
   if( (m_boundGndPolygon.getNumberOfVertices() > 0) &&
       (!m_boundGndPolygon.hasNans()) )
   {
      if (!(m_boundGndPolygon.pointWithin(wdp)))
      {
         // if(theSeedFunction.valid())
         // {
         //    theSeedFunction->worldToLineSample(worldPoint, ip);
         // }
         // else if(!theExtrapolateGroundFlag) // if I am not already in the extrapolation routine
         // {
         //    ip = extrapolate(worldPoint);
         // }
      ip.makeNan();
         return;
      } 

   }

   //---
   // Substitute zero for null elevation if present:
   //---
   double height = worldPoint.hgt;
   if ( ossim::isnan(height) )
   {
      height = 0.0;
   }

   //---
   // Utilize iterative scheme for arriving at image point. Begin with guess
   // at image center:
   //---
   if(theSeedFunction.valid())
   {
      theSeedFunction->worldToLineSample(worldPoint, ip);
   }
   else
   {
      ip.u = theRefImgPt.u;
      ip.v = theRefImgPt.v;
   }
   
   ossimDpt ip_du;
   ossimDpt ip_dv;

   ossimGpt gp, gp_du, gp_dv;
   double dlat_du, dlat_dv, dlon_du, dlon_dv;
   double delta_lat, delta_lon, delta_u, delta_v;
   double inverse_norm;
   bool done = false;
   //---
   // Begin iterations:
   //---
   do
   {
      //---
      // establish perturbed image points about the guessed point:
      //---
      ip_du.u = ip.u + 1.0;
      ip_du.v = ip.v;
      ip_dv.u = ip.u;
      ip_dv.v = ip.v + 1.0;
      
      //---
      // Compute numerical partials at current guessed point:
      //---
      lineSampleHeightToWorld(ip,    height, gp);
      lineSampleHeightToWorld(ip_du, height, gp_du);
      lineSampleHeightToWorld(ip_dv, height, gp_dv);
      
      if(gp.isLatNan() || gp.isLonNan())
      {
         gp = ossimCoarseGridModel::extrapolate(ip);
      }
      if(gp_du.isLatNan() || gp_du.isLonNan())
      {
         gp_du = ossimCoarseGridModel::extrapolate(ip_du);
      }
      if(gp_dv.isLatNan() || gp_dv.isLonNan())
      {
         gp_dv = ossimCoarseGridModel::extrapolate(ip_dv);
         
      }

      if ( m_crossesDateline )
      {
         // Longitude set to between 0 and 360.
         if ( gp.lon < 0.0 )    gp.lon    += 360.0;
         if ( gp_du.lon < 0.0 ) gp_du.lon += 360.0;
         if ( gp_dv.lon < 0.0 ) gp_dv.lon += 360.0;
      }
      
      dlat_du = gp_du.lat - gp.lat; //e
      dlon_du = gp_du.lon - gp.lon; //g
      dlat_dv = gp_dv.lat - gp.lat; //f
      dlon_dv = gp_dv.lon - gp.lon; //h
      
      //---
      // Test for convergence:
      // Use the wdp as it was corrected if there was a date line cross.
      //
      delta_lat = wdp.lat - gp.lat;
      delta_lon = wdp.lon - gp.lon;

      // Compute linearized estimate of image point given gp delta:
      inverse_norm = dlat_dv*dlon_du - dlat_du*dlon_dv; // fg-eh
      
      if (!ossim::almostEqual(inverse_norm, 0.0, DBL_EPSILON))
      {
         delta_u = (-dlon_dv*delta_lat + dlat_dv*delta_lon)/inverse_norm;
         delta_v = ( dlon_du*delta_lat - dlat_du*delta_lon)/inverse_norm;
         ip.u += delta_u;
         ip.v += delta_v;
      }
      else
      {
         delta_u = 0;
         delta_v = 0;
      }

#if 0
      cout << "gp:    " << gp
           << "\ngp_du: " << gp_du
           << "\ngp_dv: " << gp_dv
           << "\ndelta_lat: " << delta_lat
           << "\ndelta_lon: " << delta_lon     
           << "\ndelta_u: " << delta_u
           << "\ndelta_v: " << delta_v
           << endl;
#endif      
      
      done = ((fabs(delta_u) < PIXEL_THRESHOLD)&&
              (fabs(delta_v) < PIXEL_THRESHOLD));
      ++iters;

   } while ( (!done) && (iters < MAX_NUM_ITERATIONS));

   //---
   // Note that this error mesage appears only if max count was reached while
   // iterating. A linear (no iteration) solution would finish with iters =
   // MAX_NUM_ITERATIONS + 1:
   //---
#if 0 /* please leave for debug: */
   if (iters >= MAX_NUM_ITERATIONS)
   {
      std::cout << "MAX ITERATION!!!" << std::endl;
      std::cout << "delta_u = "   << delta_u
                << "\ndelta_v = " << delta_v << "\n";
   }
   else
   {
      std::cout << "ITERS === " << iters << std::endl;
   }
   std::cout << "iters = " << iters << "\n";
#endif   

   //---
   // The image point computed this way corresponds to full image space.
   // Apply image offset in the case this is a sub-image rectangle:
   //---
   ip -= theSubImageOffset;
   
} // End: worldToLineSample( ... )

void ossimH5GridModel::initializeModelParams(ossimIrect imageBounds)
{
   theLatGrid.enableExtrapolation();
   theLonGrid.enableExtrapolation();
   theHeightEnabledFlag = false;

   // NOTE: it is assumed that the grid size and spacing is the same for ALL grids:
   ossimIpt gridSize (theLatGrid.size());
   ossimDpt spacing  (theLatGrid.spacing());
   ossimDpt v[4];
   v[0].lat = theLatGrid.getNode(0,0);
   v[0].lon = theLonGrid.getNode(0,0);
   v[1].lat = theLatGrid.getNode(gridSize.x-1, 0);
   v[1].lon = theLonGrid.getNode(gridSize.x-1, 0);
   v[2].lat = theLatGrid.getNode(gridSize.x-1, gridSize.y-1);
   v[2].lon = theLonGrid.getNode(gridSize.x-1, gridSize.y-1);
   v[3].lat = theLatGrid.getNode(0, gridSize.y-1);
   v[3].lon = theLonGrid.getNode(0, gridSize.y-1);

   if ( m_crossesDateline )
   {
      // Longitude values between 0 and 360.
      m_boundGndPolygon = ossimPolygon(4, v);
   }
   
   // Guaranty longitude values are -180 to 180
   for (int i=0; i<4; ++i)
   {
      if (v[i].lon > 180.0) v[i].lon -= 360.0;
   }

   theBoundGndPolygon = ossimPolygon(4, v);

   if ( !m_crossesDateline )
   {
      // Longitude values between -180 and 180.
      m_boundGndPolygon = theBoundGndPolygon;
   }
   
   theImageSize  = ossimDpt(imageBounds.width(), imageBounds.height());
   theRefImgPt   = imageBounds.midPoint();
   theRefGndPt.lat = theLatGrid(theRefImgPt);
   theRefGndPt.lon = theLonGrid(theRefImgPt);
   
   ossimDpt ref_ip_dx (theRefImgPt.x+1.0, theRefImgPt.y    );
   ossimDpt ref_ip_dy (theRefImgPt.x    , theRefImgPt.y+1.0);
   ossimGpt ref_gp_dx (theLatGrid(ref_ip_dx), theLonGrid(ref_ip_dx));
   ossimGpt ref_gp_dy (theLatGrid(ref_ip_dy), theLonGrid(ref_ip_dy));

   theGSD.x   = theRefGndPt.distanceTo(ref_gp_dx);
   theGSD.y   = theRefGndPt.distanceTo(ref_gp_dy);

   theMeanGSD = (theGSD.line + theGSD.samp)/2.0;
   theImageClipRect  = imageBounds;

   // Image is clipped to valid rect so no sub image offset.
   theSubImageOffset = ossimDpt(0.0, 0.0); //imageBounds.ul();

   theRefGndPt.limitLonTo180();

   // debugDump();
   
} // End: initializeModelParams


ossimDpt ossimH5GridModel::extrapolate( const ossimGpt& gpt ) const
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         <<  "DEBUG ossimH5GridModel::extrapolate: entering... " << std::endl;
   }

   theExtrapolateGroundFlag = true;
   double height = 0.0;

   //---
   // If ground point supplied has NaN components, return now with a NaN point.
   //---
   if ( (ossim::isnan(gpt.lat)) || (ossim::isnan(gpt.lon)) )
   {
      theExtrapolateGroundFlag = false;
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG ossimH5GridModel::extrapolate: returning..." << std::endl;
      }
      return ossimDpt(ossim::nan(), ossim::nan());
   }
   
   if(ossim::isnan(gpt.hgt) == false)
   {
      height = gpt.hgt;
   }
   
   if(theSeedFunction.valid())
   {
      ossimDpt ipt;
      
      theSeedFunction->worldToLineSample(gpt, ipt);
      
      theExtrapolateGroundFlag = false;
      return ipt;
   }
   
   //---
   // Determine which edge is intersected by the radial, and establish
   // intersection:
   //---
   ossimDpt edgePt (gpt);
   ossimDpt image_center (theRefGndPt);
   if ( m_crossesDateline )
   {
      // Longitude points between 0 and 360.
      if (edgePt.lon < 0.0 ) edgePt.lon += 360.0;
      if ( image_center.lon < 0.0 ) image_center.lon += 360.0;
   }
   
   m_boundGndPolygon.clipLineSegment(image_center, edgePt);

   //---
   // Compute an epsilon perturbation in the direction away from edgePt for
   // later computing directional derivative:
   //---
   const double  DEG_PER_MTR =  8.983152841e-06; // Equator WGS-84...
   double epsilon = theMeanGSD*DEG_PER_MTR; //degrees (latitude) per pixel
   ossimDpt deltaPt (edgePt-image_center);
   ossimDpt epsilonPt (deltaPt*epsilon/deltaPt.length());
   edgePt -= epsilonPt;
   ossimDpt edgePt_prime (edgePt - epsilonPt);
       
   //---
   // Establish image point corresponding to edge point and edgePt+epsilon:
   //---
   ossimGpt edgeGP       (edgePt.lat,       edgePt.lon,       height);//gpt.hgt);
   ossimGpt edgeGP_prime (edgePt_prime.lat, edgePt_prime.lon, height);//gpt.hgt);
   if ( m_crossesDateline )
   {
      // Put longitude back between -180 and 180.
      edgeGP.limitLonTo180();
      edgeGP_prime.limitLonTo180();
   }

   worldToLineSample(edgeGP, edgePt);
   worldToLineSample(edgeGP_prime, edgePt_prime);

   //---
   // Compute approximate directional derivatives of line and sample along
   // radial at the edge:
   //---
   double dsamp_drad = (edgePt.samp - edgePt_prime.samp)/epsilon;
   double dline_drad = (edgePt.line - edgePt_prime.line)/epsilon;

   //---
   // Now extrapolate to point of interest:
   //---
   double delta = (ossimDpt(gpt) - ossimDpt(edgeGP)).length();

   
   ossimDpt extrapolated_ip (edgePt.samp + delta*dsamp_drad,
                             edgePt.line + delta*dline_drad);

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "DEBUG ossimH5GridModel::extrapolate: returning..." << std::endl;  
   }
   
   theExtrapolateGroundFlag = false;
   return extrapolated_ip;
   
} // ossimH5GridModel::extrapolate

bool ossimH5GridModel::getWktFootprint( std::string& s ) const
{
   bool status = true;

   // Loop until complete or a "nan" is hit.
   while ( 1 )
   {
      ossim_float32 lat = 0.0;
      ossim_float32 lon = 0.0;

      std::ostringstream os;

      os <<  setprecision(10) << "MULTIPOLYGON(((";
      const ossim_int32 STEP = 128;
      
      ossimIrect rect( 0, 0, theImageSize.x-1, theImageSize.y-1 );  

      ossim_int32 x = 0;
      ossim_int32 y = 0;
      
      // Walk the top line:
      while ( x < theImageSize.x )
      {
         lat = theLatGrid( x, y );
         lon = theLonGrid( x, y );

         if ( ossim::isnan(lat) || ossim::isnan(lon) )
         {
            status = false;
            break;
         }
         
         os << lon << " " << lat << ",";
         
         if ( x != rect.ur().x )
         {
            x = ossim::min<ossim_int32>( x+STEP, rect.ur().x );
         }
         else
         {
            break; // End of top line walk.
         }
      }

      if ( !status ) break; // Found "nan" so get out.
      
      // Walk the right edge:
      y = ossim::min<ossim_int32>( y+STEP, rect.lr().y );
      while ( y < theImageSize.y )
      {
         lat = theLatGrid( x, y );
         lon = theLonGrid( x, y );

         if ( ossim::isnan(lat) || ossim::isnan(lon) )
         {
            status = false;
            break;
         }

         os << lon << " " << lat << ",";
         
         if ( y != rect.lr().y )
         {
            y = ossim::min<ossim_int32>( y+STEP, rect.lr().y );
         }
         else
         {
            break;
         }
      }

      if ( !status ) break; // Found "nan" so get out.
      
      // Walk the bottom line from right to left:
      x = ossim::max<ossim_int32>( rect.lr().x-STEP, 0 );
      while ( x >= 0 )
      {
         lat = theLatGrid( x, y );
         lon = theLonGrid( x, y );
         
         if ( ossim::isnan(lat) || ossim::isnan(lon) )
         {
            status = false;
            break;
         }

         os << lon << " " << lat << ",";
         
         if ( x != 0 )
         {
            x = ossim::max<ossim_int32>( x-STEP, 0 );
         }
         else
         {
            break;
         }
      }

      if ( !status ) break; // Found "nan" so get out.
      
      // Walk the left edge from bottom to top:
      y = ossim::max<ossim_int32>( y-STEP, 0 );
      while ( y >= 0 )
      {
         lat = theLatGrid( x, y );
         lon = theLonGrid( x, y );

         if ( ossim::isnan(lat) || ossim::isnan(lon) )
         {
            status = false;
            break;
         }

         if ( ( x == 0 ) && ( y == 0 ) )
         {
            os << lon << " " << lat; // Last point:
         }
         else
         {
            os << lon << " " << lat << ",";
         }
         
         if ( y != 0 )
         {
            y = ossim::max<ossim_int32>( y-STEP, 0 );
         }
         else
         {
            break;
         }
      }

      if ( !status ) break; // Found "nan" so get out.
      
      os << ")))";
      s = os.str();

      // Trailing break from while ( FOREVER ) loop:
      break;
      
   } // Matches: while ( 1 )
   
   return status;
}

void ossimH5GridModel::debugDump()
{
   ossimIpt step (theImageSize/200);
   int margin = 0;
   double lat, lon;
   ossimDpt pt (0,0);
   ofstream fslat ("lat_grid.dat");
   ofstream fslon ("lon_grid.dat");
   fslat<< setprecision(10) <<endl;
   fslon<< setprecision(10) <<endl;
   for (pt.v = -margin*step.v; pt.v < theImageSize.v+margin*step.v; pt.v += step.y)
   {
      for (pt.u = -margin*step.u; pt.u < theImageSize.u+margin*step.u ; pt.u += step.u)
      {
         lat = theLatGrid(pt.u, pt.v);
         lon = theLonGrid(pt.u, pt.v);
         fslat << pt.u << " " << pt.v << " " << lat << endl;
         fslon << pt.u << " " << pt.v << " " << lon << endl;
      }
   }
   fslat.close();
   fslon.close();
}

//*****************************************************************************
// FILE: ossimH5GridModel.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// AUTHOR: David Burken
//
// Copied from Mingjie Su's ossimHdfGridModel.
//
// DESCRIPTION:
//   Contains declaration of class ossimHdfGridModel. This is an
//   implementation of an interpolation sensor model.
//
//*****************************************************************************
//  $Id$

#ifndef ossimH5GridModel_HEADER
#define ossimH5GridModel_HEADER 1
#include <ossim/base/ossimPolygon.h>
#include <ossim/projection/ossimCoarseGridModel.h>

#include <ossim/plugin/ossimPluginConstants.h>

#include <string>

// Forward class declarations:
class ossimDpt;
class ossimGpt;
namespace H5
{
   class DataSet;
   class H5File;
}

/******************************************************************************
 *
 * CLASS:  ossimH5GridModel
 *
 *****************************************************************************/
class OSSIM_PLUGINS_DLL ossimH5GridModel : public ossimCoarseGridModel
{
public:

   /** @brief default constructor. */
   ossimH5GridModel();
   
   /** @brief virtual destructor */
   virtual ~ossimH5GridModel();

   /**
    * @brief Initializes the latitude and longitude ossimDblGrids from file.
    * @param latDataSet H5::DataSet* to layer,
    *    e.g. /All_Data/VIIRS-DNB-GEO_All/Latitude
    * @param lonDataSet H5::DataSet* to layer,
    *    e.g. /All_Data/VIIRS-DNB-GEO_All/Longitude
    * @param validRect Valid rectangle of image.
    * @return true on success, false on error.
    *
    * This can throw an ossimException on nans.
    */
   bool setGridNodes( H5::DataSet* latDataSet,
                      H5::DataSet* lonDataSet,
                      const ossimIrect& validRect );
   
   /**
    * @brief saveState Saves state of object to a keyword list.
    * @param kwl Initialized by this.
    * @param prefix E.g. "image0.geometry.projection."
    * @return true on success, false on error.
    */
   virtual bool saveState(ossimKeywordlist& kwl, const char* prefix=0) const;

   /**
    * @brief loadState Loads state of object from a keyword list.
    * @param kwl Keyword list to initialize from.
    * @param prefix E.g. "image0.geometry.projection."
    * @return true on success, false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix=0);

   /**
    * METHOD: extrapolate()
    * Extrapolates solutions for points outside of the image. The second
    * version accepts a height value -- if left at the default, the elevation
    * will be looked up via theElevation object.
    */
   virtual void  worldToLineSample(const ossimGpt& world_point,
                                   ossimDpt&       image_point) const;

protected:

   /**
    * @brief Initializes base class data members after grids have been assigned.
    *
    * Overrides base class method to handle date line cross.
    */
   void initializeModelParams(ossimIrect imageBounds);

   /**
    * @brief extrapolate()
    * Extrapolates solutions for points outside of the image.
    *
    * Overrides base class method to handle date line cross.
    */
   virtual ossimDpt extrapolate( const ossimGpt& gp ) const;
   
private:
   
   bool getWktFootprint( std::string& s ) const;
   
   void debugDump();

   bool m_crossesDateline;

   //---
   // This polygon differs from base "theBoundGndPolygon" in that if the
   // scene crosses the dateline the longitude values are stored between
   // 0 and 360 degress as opposed to -180 to 180.
   //---
   ossimPolygon m_boundGndPolygon;
   
   TYPE_DATA
};

#endif /* Matches: #ifndef ossimH5GridModel_HEADER */

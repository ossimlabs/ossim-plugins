//----------------------------------------------------------------------------
//
// File: ossimGdalImageElevationDatabase.h
// 
// License:  MIT
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Scott Bortman
//
// Description: See description for class below.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGdalImageElevationDatabase_HEADER
#define ossimGdalImageElevationDatabase_HEADER 1

#include <ossim/elevation/ossimElevationCellDatabase.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimGrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimRtti.h>
#include <map>

class ossimString;
class GDALDataset;

/**
 * @class ossimGdalImageElevationDatabase
 *
 * Elevation source used for working with generic images opened by an
 * ossimImageHandler. This class is typically utilized through the
 * ossimElevManager.
 */
class OSSIM_DLL ossimGdalImageElevationDatabase :
   public ossimElevationCellDatabase
{
public:

   /** default constructor */
   ossimGdalImageElevationDatabase();

   /**
    * @brief Open a connection to a database.
    *
    * @param connectionString File or directory to open.  In most cases this
    * will point to a directory containing DEMs. Satisfies pure virtual
    * ossimElevationDatabase::open().
    *
    * @return true on success, false on error.
    */   
   virtual bool open(const ossimString& connectionString);

   /** @brief close method. Unreferences all data. */
   virtual void close();

   virtual ossimObject* dup() const
   {
      ossimGdalImageElevationDatabase* duped = new ossimGdalImageElevationDatabase;
      duped->open(m_connectionString);
      return duped;
   }

   /**
    * @brief Maps elevation data for region to a grid.
    *
    * This uses connectionString passed to open method as starting point.
    */
   void mapRegion(const ossimGrect& region);

   /**
    * @brief Get height above MSL for point.
    *
    * Satisfies pure virtual ossimElevSource::getHeightAboveMSL().
    * 
    * @return Height above MSL.
    */
   virtual double getHeightAboveMSL(const ossimGpt& gpt);

   /**
    * @brief Get height above ellipsoid for point.
    *
    * Satisfies pure virtual ossimElevSource::getHeightAboveMSL().
    * 
    * @return Height above MSL.
    */
   virtual double getHeightAboveEllipsoid(const ossimGpt&);
   
   /**
    * Satisfies pure virtual ossimElevSource::pointHasCoverage
    * 
    * @return true if database has coverage for point.
    */
   virtual bool pointHasCoverage(const ossimGpt& gpt) const;


   virtual bool getAccuracyInfo(ossimElevationAccuracyInfo& info, const ossimGpt& gpt) const;


   /**
    * Statisfies pure virtual ossimElevSource::getAccuracyLE90.
    * @return The vertical accuracy (90% confidence) in the
    * region of gpt:
    */
   //virtual double getAccuracyLE90(const ossimGpt& gpt) const;
   
   /**
    * Statisfies pure virtual ossimElevSource::getAccuracyCE90.
    * @return The horizontal accuracy (90% confidence) in the
    * region of gpt.
    */
   //virtual double getAccuracyCE90(const ossimGpt& gpt) const;

   /** @brief Initialize from keyword list. */
   virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix=0);

   /** @brief Save the state to a keyword list. */
   virtual bool saveState(ossimKeywordlist& kwl, const char* prefix=0)const;

   /**
    * @brief Gets the bounding rectangle/coverage of elevation.
    *
    * @param rect Rectangle to initialize.
    */
   void getBoundingRect(ossimGrect& rect) const;
   
   virtual std::ostream& print(std::ostream& out) const;

protected:
   /**
    * @Brief Protected destructor.
    *
    * This class is derived from ossimReferenced so users should always use
    * ossimRefPtr<ossimImageElevationDatabase> to hold instance.
    */
   virtual ~ossimGdalImageElevationDatabase();

   virtual ossimRefPtr<ossimElevCellHandler> createCell(const ossimGpt& gpt);

   // virtual ossim_uint64 createId(const ossimGpt& pt) const;

   /**
    * @brief Gets cell for point.
    *
    * This override ossimElevationCellDatabase::getOrCreateCellHandler as we cannot use
    * the createId as our cells could be of any size.
    */
   virtual ossimRefPtr<ossimElevCellHandler> getOrCreateCellHandler(const ossimGpt& gpt);

   /**
    * @brief Removes an entry from the m_cacheMap.
    */
   virtual void remove(ossim_uint64 id);

private:

   // Private container to hold bounding rect, nominal GSD, and image handler.
   struct ossimGdalImageElevationFileEntry
   {
      /** @brief default constructor */
      ossimGdalImageElevationFileEntry();

      /** @brief Constructor that takes a file name. */
      ossimGdalImageElevationFileEntry(const ossimFilename& file);

      ossimGdalImageElevationFileEntry(const ossimGdalImageElevationFileEntry& copy_this);

      /** file name */
      ossimFilename m_file;

      /** Bounding rectangle in decimal degrees. */
      ossimGrect m_rect;
      ossimDpt m_nominalGSD; // post spacing at center

      /** True if in ossimElevationCellDatabase::m_cacheMap. */
      bool m_loadedFlag;
   };  

   /** Hidden from use copy constructor */
   ossimGdalImageElevationDatabase(const ossimGdalImageElevationDatabase& copy_this);
   
   GDALDataset *m_poDataSet;

   bool openShapefile();   
   void closeShapefile();
   std::string searchShapefile(double lon, double lat) const;

   ossim_uint64       m_lastMapKey;
   ossim_uint64       m_lastAccessedId;

   TYPE_DATA 
};

inline void ossimGdalImageElevationDatabase::remove(ossim_uint64 id)
{
   ossimElevationCellDatabase::remove(id);   
}

#endif /* ossimGdalImageElevationDatabase_HEADER */

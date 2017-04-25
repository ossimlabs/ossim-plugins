//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimTiePoint_HEADER
#define ossimTiePoint_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandler.h>

class ossimTiePoint
{
public:
   struct Covariance
   {
      double vXX;
      double vYY;
      double vXY;
   };

   ossimTiePoint();

   virtual ~ossimTiePoint();

   unsigned int getTiePointID() const { return m_tiePointID; }

   const ossimDpt& getImagePoint(unsigned int index) const;

   const ossimString& getImageID(unsigned int index) const;

   const ossimDpt& getCovariance(unsigned int index) const;

private:
   unsigned int             m_tiePointID;
   std::vector<ossimString> m_imageIDs;    //> List of images containing common feature
   std::vector<ossimDpt>    m_imagePoints; //> List of image point measurements for common feature
   std::vector<Covariance>  m_covariances; //> List of measurement covariances corresponding to image points vector
   double                   m_gsd; //> image scale (meters/pixel) at which matching was performed

   static unsigned int s_lastID; //> Auto-increment with each new tiepoint created
};

#endif /* #ifndef ossimTiePoint_HEADER */

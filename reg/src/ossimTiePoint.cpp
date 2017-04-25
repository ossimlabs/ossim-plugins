//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimTiePoint.h"

unsigned int ossimTiePoint::s_lastID = 0;

ossimTiePoint::ossimTiePoint()
:  m_tiePointID (++s_lastID),
   m_gsd (0.0)
{

}

ossimTiePoint::~ossimTiePoint()
{

}

void ossimTiePoint::getImagePoint(unsigned int index, ossimDpt& imagePoint) const
{

}

void ossimTiePoint::getImageID(unsigned int index, ossimString& imageID) const
{

}

void ossimTiePoint::getCovariance(unsigned int index, ossimTiePoint::Covariance& covariance) const
{

}


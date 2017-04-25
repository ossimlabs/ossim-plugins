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

const ossimDpt& ossimTiePoint::getImagePoint(unsigned int index) const
{

}

const ossimString& ossimTiePoint::getImageID(unsigned int index) const
{

}

const ossimDpt& ossimTiePoint::getCovariance(unsigned int index) const
{

}


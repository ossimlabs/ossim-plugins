/*****************************************************************************
*                                                                            *
*                                 O S S I M                                  *
*            Open Source, Geospatial Image Processing Project                *
*          License: MIT, see LICENSE at the top-level directory              *
*                                                                            *
*****************************************************************************/

#include "ossimViirsHandler.h"

static const ossimString VIIRS_DATASET ("/All_Data/VIIRS-DNB-SDR_All/Radiance");

ossimViirsHandler::ossimViirsHandler()
{
   addRenderableSetName(VIIRS_DATASET);
}

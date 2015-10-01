//*******************************************************************
// License:  See top level LICENSE.txt file.
//
// Author: Garrett Potts
// 
// Description: This class extends the stl's string class.
//
//********************************************************************
// $Id: ossimShapeFile.cpp 22453 2013-10-23 18:12:16Z gpotts $
//

#include <iomanip>
#include <ossimShapeFile.h>

RTTI_DEF1(ossimShapeFile, "ossimShapeFile", ossimObject);

using namespace std;
std::ostream& operator<<(std::ostream& out, const ossimShapeObject& rhs)
{
   if(rhs.isLoaded())
   {
      double minx, miny, minz, minm;
      double maxx, maxy, maxz, maxm;
      ossim_uint32 i = 0;
      
      rhs.getBounds(minx, miny, minz, minm,
                    maxx, maxy, maxz, maxm);
      
      out << std::setw(15) << setiosflags(std::ios::left)<<"type:"<<rhs.getTypeByName() << std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"id:"<<rhs.getId()<<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"minx:"<<minx <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"miny:"<<miny <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"minz:"<<minz <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"minm:"<<minm <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"maxx:"<<maxx <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"maxy:"<<maxy <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"maxz:"<<maxz <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"maxm:"<<maxm <<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"parts:"<<rhs.getNumberOfParts()<<std::endl
          << std::setw(15) << setiosflags(std::ios::left)<<"vertices:"<<rhs.getNumberOfVertices();

      if(rhs.getNumberOfParts())
      {
         out << std::endl;
         for(i = 0; i < rhs.getNumberOfParts()-1; ++i)
         {
            ossimString s1 = "part start ";
            ossimString s2 = "part type ";
            s1 += (ossimString::toString(i+1)+":");
            s2 += (ossimString::toString(i+1)+":");
            
            out << std::setw(15) << setiosflags(std::ios::left) << s1.c_str() << rhs.theShape->panPartStart[i]<<std::endl;
            out << std::setw(15) << setiosflags(std::ios::left) << s2.c_str() <<  SHPPartTypeName(rhs.theShape->panPartType[i])<<std::endl;
         }
         ossimString s1 = "part start ";
         ossimString s2 = "part type ";
         
         s1 += (ossimString::toString(rhs.getNumberOfParts())+":");
         out << std::setw(15) << setiosflags(std::ios::left) << s1.c_str() <<  rhs.theShape->panPartStart[i]<<std::endl;
         out << std::setw(15) << setiosflags(std::ios::left) << s2.c_str() <<  SHPPartTypeName(rhs.theShape->panPartType[i]);
      }
      
      out << std::setw(0);
      
   }
   return out;   
}

ossimShapeObject::ossimShapeObject()
   :theShape((SHPObject*)NULL),
    theIndex(-1)
{
}

ossimShapeObject::~ossimShapeObject()
{
   if(theShape)
   {
      SHPDestroyObject(theShape);
      theShape = NULL;
   }
}

void ossimShapeObject::getBoundingRect(ossimDrect& result,
                                       ossimCoordSysOrientMode orient)const
{
   double minx, miny, maxx, maxy;
   
   if(theShape)
   {
      getBounds(minx, miny, maxx, maxy);
      
      if(orient == OSSIM_RIGHT_HANDED)
      {
         result = ossimDrect(minx, maxy, maxx, miny, orient);
      }
      else
      {
         result = ossimDrect(minx, miny, maxx, maxy, orient);         
      }
   }
   else
   {
      result = ossimDrect(0,0,0,0,orient);
      result.makeNan();
   }
}

void ossimShapeObject::setShape(SHPObject* obj)
{
   if(theShape)
   {
      SHPDestroyObject(theShape);
      theShape = NULL;            
   }
   
   theShape = obj;
}

bool ossimShapeObject::isLoaded()const
{
   return (theShape!=NULL);
}

long ossimShapeObject::getIndex()const
{
   return theIndex;
}

long ossimShapeObject::getId()const
{
   if(theShape)
   {
      return theShape->nShapeId;
   }
   
   return -1;
}

bool ossimShapeObject::loadShape(const ossimShapeFile& shapeFile,
                                 long shapeRecord)
{
   if(theShape)
   {
      SHPDestroyObject(theShape);
      theShape = NULL;            
   }
   if(shapeFile.isOpen())
   {
      theShape = SHPReadObject(shapeFile.getHandle(),
                               shapeRecord);
      theIndex = shapeRecord;
   }
   else
   {
      theIndex = -1;
   }
   return (theShape != (SHPObject*)NULL);
}

ossim_uint32 ossimShapeObject::getNumberOfParts()const
{
   if(theShape)
   {
      return theShape->nParts;
   }
   
   return 0;
}

ossim_uint32 ossimShapeObject::getNumberOfVertices()const
{
   if(theShape)
   {
      return theShape->nVertices;
   }
   
   return 0;
}

void ossimShapeObject::getBounds(
   double& minX, double& minY, double& minZ, double& minM,
   double& maxX, double& maxY, double& maxZ, double& maxM)const
{
   if(theShape)
   {
      minX = theShape->dfXMin;
      minY = theShape->dfYMin;
      minZ = theShape->dfZMin;
      minM = theShape->dfMMin;
      maxX = theShape->dfXMax;
      maxY = theShape->dfYMax;
      maxZ = theShape->dfZMax;
      maxM = theShape->dfMMax;
   }
   else
   {
      minX = minY = minZ = minM =
         maxX = maxY = maxZ = maxM = ossim::nan();
   }
}

void ossimShapeObject::getBounds(double& minX, double& minY,
                                 double& maxX, double& maxY)const
{
   if(theShape)
   {
      minX = theShape->dfXMin;
      minY = theShape->dfYMin;
      maxX = theShape->dfXMax;
      maxY = theShape->dfYMax;
   }
   else
   {
      minX = minY = 
         maxX = maxY = ossim::nan();
   }
}

ossimDrect ossimShapeObject::getBoundingRect(
   ossimCoordSysOrientMode orient)const
{
   ossimDrect result;
   
   getBoundingRect(result, orient);
   
   return result;
}
   
int ossimShapeObject::getType()const
{
   if(theShape)
   {
      return theShape->nSHPType;
   }
   return SHPT_NULL;
}

ossimString ossimShapeObject::getTypeByName()const
{
   if(theShape)
   {
      return ossimString(SHPTypeName(theShape->nSHPType));
   }
   return "unknown";
}

int ossimShapeObject::getPartType(ossim_uint32 partIndex)const
{
   if((partIndex > getNumberOfParts())||
      (!theShape))
   {
      return -1;
   }
   
   return theShape->panPartType[partIndex];
}

ossimString ossimShapeObject::getPartByName(ossim_uint32 partIndex)const
{
   if((partIndex > getNumberOfParts())||
      (!theShape))
   {
   }
   switch(theShape->panPartType[partIndex])
   {
      case SHPP_TRISTRIP:
      {
         ossimString("tristrip");
         break;
      }
      case SHPP_TRIFAN:
      {
         ossimString("trifan");
         break;
      }
      case SHPP_OUTERRING:
      {
         ossimString("outerring");
         break;
      }
      case SHPP_INNERRING:
      {
         ossimString("innerring");
         break;
      }
      case SHPP_RING:
      {
         ossimString("ring");
         break;
      }
   }
   return ossimString("unknown");
}

SHPObject* ossimShapeObject::getShapeObject()
{
   return theShape;
}

const SHPObject* ossimShapeObject::getShapeObject()const
{
   return theShape;
}
   
std::ostream& operator <<(std::ostream& out, const ossimShapeFile& rhs)
{
   rhs.print(out);
   
   return out;
}


ossimShapeFile::ossimShapeFile()
   :theHandle(NULL)
{
}

ossimShapeFile::~ossimShapeFile()
{
   close();
}

void ossimShapeFile::close()
{
   if(theHandle)
   {
      SHPClose(theHandle);
      theHandle = NULL;
   }
}

bool ossimShapeFile::open(const ossimFilename& file,
                          const ossimString& flags)
{
   if(isOpen()) close();
   
   theHandle = SHPOpen( file.c_str(),
                        flags.c_str());
   if(isOpen())
   {
      theFilename = file;
   }
   
   return (theHandle != NULL);
}

bool ossimShapeFile::isOpen()const
{
   return (theHandle!=NULL);
}

SHPHandle ossimShapeFile::getHandle()
{
   return theHandle;
}
   
const SHPHandle& ossimShapeFile::getHandle()const
{
   return theHandle;
}

long ossimShapeFile::getNumberOfShapes()const
{
   if(theHandle)
   {
      return theHandle->nRecords;
   }
   return 0;
}

void ossimShapeFile::getBounds(
   double& minX, double& minY, double& minZ, double& minM,
   double& maxX, double& maxY, double& maxZ, double& maxM)const
{
   if(theHandle)
   {
      minX = theHandle->adBoundsMin[0];
      minY = theHandle->adBoundsMin[1];
      minZ = theHandle->adBoundsMin[2];
      minM = theHandle->adBoundsMin[3];
      maxX = theHandle->adBoundsMax[0];
      maxY = theHandle->adBoundsMax[1];
      maxZ = theHandle->adBoundsMax[2];
      maxM = theHandle->adBoundsMax[3];
   }
   else
   {
      minX = minY = minZ = minM =
         maxX = maxY = maxZ = maxM = ossim::nan();
   }
}
   
void ossimShapeFile::getBounds(double& minX, double& minY,
                               double& maxX, double& maxY)const
{
   if(theHandle)
   {
      minX = theHandle->adBoundsMin[0];
      minY = theHandle->adBoundsMin[1];
      maxX = theHandle->adBoundsMax[0];
      maxY = theHandle->adBoundsMax[1];
   }
   else
   {
      minX = minY = maxX = maxY = ossim::nan();
   }
}

std::ostream& ossimShapeFile::print(std::ostream& out) const
{
   if(isOpen())
   {
      out << std::setw(15) << setiosflags(std::ios::left)<<"Shp filename:" << theFilename << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"Record count:" << theHandle->nRecords << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"File type:" << getShapeTypeString().c_str() << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"minx:" << theHandle->adBoundsMin[0] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"miny:" << theHandle->adBoundsMin[1] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"minz:" << theHandle->adBoundsMin[2] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"minm:" << theHandle->adBoundsMin[3] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"maxx:" << theHandle->adBoundsMax[0] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"maxy:" << theHandle->adBoundsMax[1] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"maxz:" << theHandle->adBoundsMax[2] << std::endl;
      out << std::setw(15) << setiosflags(std::ios::left)<<"maxm:" << theHandle->adBoundsMax[3] << std::endl;

      ossimShapeObject shape;
      
      if(theHandle->nRecords)
      {
         out << std::setw(30) << std::setfill('_') << "" << std::setfill(' ')<<std::endl;
      }
      for(int i=0; i < theHandle->nRecords; ++i)
      {
         if(shape.loadShape(*this, i))
         {
            out << shape << std::endl;
            out << std::setw(30) << std::setfill('_') << ""<<std::setfill(' ')<<std::endl;
         }
      }
   }
   return out;
}


ossimString ossimShapeFile::getShapeTypeString()const
{
   if(theHandle)
   {
      return SHPTypeName(theHandle->nShapeType);
   }
   return "";
}

void ossimShapeFile::getBoundingRect(ossimDrect& result,
                                     ossimCoordSysOrientMode orient)const
{
   double minx, miny, maxx, maxy;
   
   getBounds(minx, miny, maxx, maxy);
   if(orient == OSSIM_RIGHT_HANDED)
   {
      result = ossimDrect(minx, maxy, maxx, miny, orient);
   }
   else
   {
      result = ossimDrect(minx, miny, maxx, maxy, orient);         
   }
}

ossimDrect ossimShapeFile::getBoundingRect(
   ossimCoordSysOrientMode orient)const
{
   ossimDrect result;
   
   getBoundingRect(result, orient);
   
   return result;
}

const ossimFilename& ossimShapeFile::getFilename()const
{
   return theFilename;
}

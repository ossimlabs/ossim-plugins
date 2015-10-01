//*******************************************************************
//
// License:  See top level LICENSE.txt file.
// 
// Author: Garrett Potts
//
//*************************************************************************
// $Id: ossimShapeFile.h 19908 2011-08-05 19:57:34Z dburken $

#ifndef ossimShapeFile_HEADER
#define ossimShapeFile_HEADER 1

#include <iostream>

#include <shapefil.h>

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimRtti.h>
#include <ossim/base/ossimObject.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimDrect.h>


class ossimShapeFile;
class ossimAnnotationObject;

class ossimShapeObject
{
public:
   friend std::ostream& operator<<(std::ostream& out,
                                   const ossimShapeObject& rhs);
   
   ossimShapeObject();
      
   ~ossimShapeObject();

   void setShape(SHPObject* obj);
   bool isLoaded()const;
   long getIndex()const;
   long getId()const;

   bool loadShape(const ossimShapeFile& shapeFile, long shapeRecord);
   
   ossim_uint32 getNumberOfParts()const;

   ossim_uint32 getNumberOfVertices()const;
   
   void getBounds(double& minX, double& minY, double& minZ, double& minM,
                  double& maxX, double& maxY, double& maxZ, double& maxM)const;
   
   void getBounds(double& minX, double& minY,
                  double& maxX, double& maxY)const;
   
   void getBoundingRect(ossimDrect& result,
                        ossimCoordSysOrientMode orient = OSSIM_RIGHT_HANDED)const;
   
   ossimDrect getBoundingRect(ossimCoordSysOrientMode orient = OSSIM_RIGHT_HANDED)const;
   
   int getType()const;

   ossimString getTypeByName()const;

   int getPartType(ossim_uint32 partIndex)const;

   ossimString getPartByName(ossim_uint32 partIndex)const;

   SHPObject* getShapeObject();

   const SHPObject* getShapeObject()const;
   
protected:
   SHPObject* theShape;
   long       theIndex;
};

class OSSIM_PLUGINS_DLL ossimShapeFile : public ossimObject
{
public:
   friend std::ostream& operator <<(std::ostream& out,
                                    const ossimShapeFile& rhs);
   
   ossimShapeFile();
   virtual ~ossimShapeFile();
   
   virtual bool open(const ossimFilename& file,
                     const ossimString& flags=ossimString("rb"));
   
   virtual void close();

   bool isOpen()const;

   virtual SHPHandle getHandle();

   virtual const SHPHandle& getHandle()const;

   virtual std::ostream& print(std::ostream& out) const;

   virtual ossimString getShapeTypeString()const;
   
   
   virtual long getNumberOfShapes()const;

   void getBounds(double& minX, double& minY, double& minZ, double& minM,
                  double& maxX, double& maxY, double& maxZ, double& maxM)const;
   
   void getBounds(double& minX, double& minY,
                  double& maxX, double& maxY)const;
   
   void getBoundingRect(ossimDrect& result,
                        ossimCoordSysOrientMode orient = OSSIM_RIGHT_HANDED)const;
   
   ossimDrect getBoundingRect(ossimCoordSysOrientMode orient = OSSIM_RIGHT_HANDED)const;

   const ossimFilename& getFilename()const;

protected:
   SHPHandle	 theHandle;
   ossimFilename theFilename;

TYPE_DATA
};

#endif

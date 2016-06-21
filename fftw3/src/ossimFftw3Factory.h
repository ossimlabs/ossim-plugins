//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimFftw3Factory_HEADER
#define ossimFftw3Factory_HEADER
#include <ossim/imaging/ossimImageSourceFactoryBase.h>

class ossimFftw3Factory : public ossimImageSourceFactoryBase
{
public:
   virtual ~ossimFftw3Factory();
   static ossimFftw3Factory* instance();
   virtual ossimObject* createObject(const ossimString& name)const;
   virtual ossimObject* createObject(const ossimKeywordlist& kwl, const char* prefix=0)const;
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
protected:
   // Hide from use.
   ossimFftw3Factory();
   ossimFftw3Factory(const ossimFftw3Factory&);
   const ossimFftw3Factory& operator=(ossimFftw3Factory&);

   static ossimFftw3Factory* theInstance;
TYPE_DATA
};
#endif

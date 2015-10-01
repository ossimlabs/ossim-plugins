#ifndef ossimShapeDatabase_HEADER
#define ossimShapeDatabase_HEADER 1

#include <iostream>
#include <shapefil.h>

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimRtti.h>
#include <ossim/base/ossimObject.h>
#include <ossim/base/ossimFilename.h>

#include <ossim/base/ossimString.h>


class ossimShapeDatabaseField
{
public:
   ossimString fieldTypeAsString()const;
   
   ossimString  theName;
   int         theWidth;
   int         theDecimals;
   DBFFieldType theFieldType;
   ossimString  theValue;
};

class OSSIM_PLUGINS_DLL ossimShapeDatabaseRecord
{
   
public:
   bool getField(ossimShapeDatabaseField& result,
                 ossim_uint32 i);

   bool setField(const ossimShapeDatabaseField& field,
                 ossim_uint32 i);

   int getNumberOfFields()const;

   void setNumberOfFields(int n);

   ossim_int32 getFieldIdx(const ossimString& name,
                           bool caseInsensitive=true)const;
protected:
	std::vector<ossimShapeDatabaseField> theFieldArray;
};

class OSSIM_PLUGINS_DLL ossimShapeDatabase : public ossimObject
{
public:
   friend std::ostream& operator <<(std::ostream& out,
                                    const ossimShapeDatabase& rhs);
   
   ossimShapeDatabase();
   virtual ~ossimShapeDatabase();
   
   virtual bool open(const ossimFilename& file,
                     const ossimString& flags=ossimString("rb"));
   
   virtual void close();
   
   bool getRecord(ossimShapeDatabaseRecord& result);
   
   bool getRecord(ossimShapeDatabaseRecord& result,
                  int recordNumber);

   bool getNextRecord(ossimShapeDatabaseRecord& result);

   int getNumberOfRecords()const;
   
   bool isOpen()const;

   virtual DBFHandle getHandle();

   virtual const DBFHandle& getHandle()const;
   
   virtual std::ostream& print(std::ostream& out)const;

protected:
   DBFHandle theHandle;
   ossimFilename theFilename;

   int theRecordNumber;

TYPE_DATA
};

#endif

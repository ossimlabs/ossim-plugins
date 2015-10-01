
#include <iomanip>

#include <ossimShapeDatabase.h>
#include <ossimShapeFile.h>

RTTI_DEF1(ossimShapeDatabase, "ossimShapeDatabase", ossimObject);
using namespace std;

ossimString ossimShapeDatabaseField::fieldTypeAsString()const
{
   switch(theFieldType)
   {
   case FTString:
   {
      return "String";
   }
   case FTInteger:
   {
      return "Integer";
   }
   case FTDouble:
   {
      return "Double";
   }
   default:
   {
      return "Unknown";
   }
   };

   return "";
   
}

bool ossimShapeDatabaseRecord::getField(ossimShapeDatabaseField& result,
                                        ossim_uint32 i)
{
   if(i < theFieldArray.size())
   {
      result = theFieldArray[i];
      return true;
   }
   return false;
}

bool ossimShapeDatabaseRecord::setField(const ossimShapeDatabaseField& field,
                                        ossim_uint32 i)
{
   if(i < theFieldArray.size())
   {
      theFieldArray[i] = field;
      return true;
   }
   return false;
}

int ossimShapeDatabaseRecord::getNumberOfFields()const
{
   return theFieldArray.size();
}

void ossimShapeDatabaseRecord::setNumberOfFields(int n)
{
   if(n)
   {
      theFieldArray.resize(n);
   }
   else
   {
      theFieldArray.clear();
   }
}

ossim_int32 ossimShapeDatabaseRecord::getFieldIdx(const ossimString& name,
                                                  bool caseInsensitive)const
{
   ossimString searchString = name;
   if(caseInsensitive) searchString = searchString.downcase();
   ossim_int32 idx = 0;
   for(idx = 0; idx < (int)theFieldArray.size(); ++idx)
   {
      if(caseInsensitive)
      {
         if(ossimString(theFieldArray[idx].theName).downcase() == searchString)
         {
            return idx;
         }
      }
      else
      {
         if(theFieldArray[idx].theName == searchString)
         {
            return idx;
         }
      }
   }
   return -1;
}

std::ostream& operator <<(std::ostream& out, const ossimShapeDatabase& rhs)
{
   rhs.print(out);
   
   return out;
}

ossimShapeDatabase::ossimShapeDatabase()
   :theHandle(NULL),
    theFilename("")
{
   theRecordNumber = -1;
}

ossimShapeDatabase::~ossimShapeDatabase()
{
   close();
}

bool ossimShapeDatabase::open(const ossimFilename& file,
                              const ossimString& flags)
{
   if(isOpen()) close();

   theHandle = DBFOpen(file.c_str(), flags.c_str());
   if(theHandle)
   {
      theFilename = file;
      theRecordNumber = -1;
   }

   return (theHandle != NULL);
}

void ossimShapeDatabase::close()
{
   if(isOpen())
   {
      DBFClose(theHandle);
      theHandle = NULL;
      theRecordNumber = -1;
   }
}
int ossimShapeDatabase::getNumberOfRecords()const
{
   if(isOpen())
   {
      return theHandle->nRecords;
   }

   return 0;
}

bool ossimShapeDatabase::getRecord(ossimShapeDatabaseRecord& result)
{
   if(isOpen()&&( (theRecordNumber < theHandle->nRecords) ))
   {
      if(result.getNumberOfFields() != theHandle->nFields)
      {
         result.setNumberOfFields(theHandle->nFields);
      }
      
      char name[1024] = {'\0'};
      int width       = 0;
      int decimals    = 0;
      int iField      = 0;
      std::vector<int>         fieldWidths;
      
      for(iField = 0; iField < theHandle->nFields; ++iField)
      {   
         DBFFieldType fieldType = DBFGetFieldInfo(theHandle,
                                                  iField,
                                                  name,
                                                  &width,
                                                  &decimals);
         ossimShapeDatabaseField field;
         field.theName = name;
         field.theWidth = width;
         field.theDecimals = decimals;
         field.theFieldType = fieldType;
            
         ossimString key = "field";
         key+=ossimString::toString(iField+1);
         key+=(ossimString(".")+name+":");
         
         switch(fieldType)
         {
         case FTString:
         {
            field.theValue = DBFReadStringAttribute(theHandle, theRecordNumber, iField);
            break;
         }
         case FTInteger:
         {
            field.theValue = ossimString::toString(DBFReadIntegerAttribute(theHandle, theRecordNumber, iField));
            break;
         }
         case FTDouble:
         {
            field.theValue = ossimString::toString(DBFReadDoubleAttribute(theHandle, theRecordNumber, iField));
            break;
         }
		 case FTLogical:
		 {
			 break;
		 }
		 case FTInvalid:
		 {
			 break;
		 }
         }

         result.setField(field,
                         iField);
      }
      return true;
   }

   return false;
}

bool ossimShapeDatabase::getRecord(ossimShapeDatabaseRecord& result,
                                   int recordNumber)
{
   if(isOpen())
   {
      if(recordNumber < getNumberOfRecords())
      {
         theRecordNumber = recordNumber;
         return getRecord(result);
      }
   }
   
   return false;
}
   
bool ossimShapeDatabase::getNextRecord(ossimShapeDatabaseRecord& result)
{
   if(isOpen() && ((theRecordNumber+1) < getNumberOfRecords()))
   {
      ++theRecordNumber;
      return getRecord(result);
   }
   
   return false;
}

bool ossimShapeDatabase::isOpen()const
{
   return (theHandle!=NULL);
}

DBFHandle ossimShapeDatabase::getHandle()
{
   return theHandle;
}

const DBFHandle& ossimShapeDatabase::getHandle()const
{
   return theHandle;
}

std::ostream& ossimShapeDatabase::print(std::ostream& out)const
{
   if(isOpen())
   {
      out << std::setw(15)<<setiosflags(std::ios::left)
          <<"DB filename:" << theFilename << std::endl
          << std::setw(15)<<setiosflags(std::ios::left)
          <<"records:" << theHandle->nRecords << std::endl
          << std::setw(15)<<setiosflags(std::ios::left)
          <<"fields:" << theHandle->nFields << std::endl;

      char name[1024] = {'\0'};
      int width       = 0;
      int decimals    = 0;
      int iField      = 0;
      std::vector<int>         fieldWidths;

      
      for(iField = 0; iField < theHandle->nFields; ++iField)
      {
         DBFFieldType fieldType = DBFGetFieldInfo(theHandle,
                                                  iField,
                                                  name,
                                                  &width,
                                                  &decimals);
         ossimString s = "field " + ossimString::toString(iField+1) + " name:";
         switch(fieldType)
         {
         case FTString:
         case FTInteger:
         case FTDouble:
         {
            out << std::setw(15) << setiosflags(std::ios::left) << s.c_str() << name << std::endl;
            break;
         }
         default:
         {
            out << std::setw(15) << setiosflags(std::ios::left) << s.c_str() << "INVALID"<<std::endl;
            break;
         }
         }
      }
      for(int iShape = 0; iShape < theHandle->nRecords; ++iShape)
      {
         for(iField = 0; iField < theHandle->nFields; ++iField)
         {   
            DBFFieldType fieldType = DBFGetFieldInfo(theHandle,
                                                     iField,
                                                     name,
                                                     &width,
                                                     &decimals);
            
            ossimString key = "field";
            key+=ossimString::toString(iField+1);
            key+=(ossimString(".")+name+":");
            
            switch(fieldType)
            {
            case FTString:
            {
               
               out << std::setw(25) << setiosflags(std::ios::left) << key.c_str()
                   << DBFReadStringAttribute(theHandle, iShape, iField) <<std::endl;
               
               break;
            }
            case FTInteger:
            {
               out << std::setw(25) << setiosflags(std::ios::left) << key.c_str()
                   << DBFReadIntegerAttribute(theHandle, iShape, iField) << std::endl;
               
               break;
            }
            case FTDouble:
            {
               out << std::setw(25) << setiosflags(std::ios::left) << key.c_str()
                   << DBFReadDoubleAttribute(theHandle, iShape, iField) << std::endl;
               
               break;
            }
			case FTLogical:
			{
				break;
			}
			case FTInvalid:
			{
				break;
			}
            }
         }
         out << "_________________________________"<<std::endl;
      }
   }

   return out;
}

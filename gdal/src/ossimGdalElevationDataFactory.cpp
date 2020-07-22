#include "ossimGdalElevationDatabaseFactory.h"
#include <ossim/base/ossimKeywordNames.h>
#include "ossimGdalImageElevationDatabase.h"

ossimGdalElevationDatabaseFactory *ossimGdalElevationDatabaseFactory::m_instance = 0;
ossimGdalElevationDatabaseFactory *ossimGdalElevationDatabaseFactory::instance()
{
   if(!m_instance)
   {
      m_instance = new ossimGdalElevationDatabaseFactory();
   }
   
   return m_instance;
}

ossimElevationDatabase *ossimGdalElevationDatabaseFactory::createDatabase(const ossimString &typeName) const
{
   ossimElevationDatabase* result = nullptr;
   // std::cout << "ossimGdalElevationDatabaseFactory::createDatabase -------------------------" << typeName << "\n";
   if(typeName == "image_directory_shx")      
   {
      result = new ossimGdalImageElevationDatabase();
   }

   return result;
 }

 ossimElevationDatabase *ossimGdalElevationDatabaseFactory::createDatabase(const ossimKeywordlist &kwl,
                                                                           const char *prefix) const
 {
   //  std::cout << "ossimGdalElevationDatabaseFactory::createDatabase\n";
    ossimRefPtr<ossimElevationDatabase> result = 0;
    ossimString type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
    if (!type.empty())
    {
       result = createDatabase(type);
       if (result.valid())
       {
          if (!result->loadState(kwl, prefix))
          {
             result = 0;
          }
       }
    }

    return result.release();
}

ossimElevationDatabase *ossimGdalElevationDatabaseFactory::open(const ossimString &connectionString) const
{
   ossimRefPtr<ossimElevationDatabase> result = 0;

   // need a detector that will take your directory and check for a shape idx file 
   // in that connection string location

   // This method will only open individual image files for use as dems. It will not utilize the
   // file walker to search over directories:
   ossimFilename filename (connectionString);
   if (filename.isFile())
   {
      result = new ossimGdalImageElevationDatabase;
      if (!result->open(connectionString)) 
      {
         std::cerr << "ERROR: ossimGdalElevationDatabaseFactory::createDatabase failed\n";
      }
      else
      {
         std::cout << "ossimGdalElevationDatabaseFactory::createDatabase success\n";
      }
      
   }   

   return result.release();
}

void ossimGdalElevationDatabaseFactory::getTypeNameList(std::vector<ossimString> &typeList) const
{
   // Modify this for your new database handler
   typeList.push_back(STATIC_TYPE_NAME(ossimGdalImageElevationDatabase));
   typeList.push_back("image_directory_shx");
}

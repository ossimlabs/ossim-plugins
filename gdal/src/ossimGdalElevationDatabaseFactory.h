#ifndef ossimGdalElevationDatabaseFactory_HEADER
#define ossimGdalElevationDatabaseFactory_HEADER
#include <ossim/elevation/ossimElevationDatabaseFactoryBase.h>

/**
*
* Example Flat file with shape dx Keywordlist:
* @code
* elevation_manager.elevation_source1.type: image_directory_shx
* elevation_manager.elevation_source1.connection_string: /data/elevation/image_directory
* elevation_manager.elevation_source1.min_open_cells: 5
* elevation_manager.elevation_source1.max_open_cells: 25
* elevation_manager.elevation_source1.enabled: true
* elevation_manager.elevation_source1.memory_map_cells: true
* @endcode
*/
class OSSIM_DLL ossimGdalElevationDatabaseFactory : public ossimElevationDatabaseFactoryBase
{
public:
   ossimGdalElevationDatabaseFactory()
   {
      m_instance = this;
   }

   /**
   * @return a singleton instance of this factory
   */
   static ossimGdalElevationDatabaseFactory* instance();
   
   /**
   * @param typeName is the type name of the database you wish to
   *        create.
   * @return the elevation database or null otherwise
   */
   ossimElevationDatabase* createDatabase(const ossimString& typeName)const;

   /**
   * @param kwl kewyord list that has state information about how to
   *        create a database
   * @param prefix prefix key to use when loading the state information.
   * @return the elevation database or null otherwise
   */
   ossimElevationDatabase* createDatabase(const ossimKeywordlist& kwl,
                                          const char* prefix=0)const;
   /**
   * @param connectionString used to determine which database can support
   *        that connection.
   * @return the elevation database or null otherwise
   */
   virtual ossimElevationDatabase* open(const ossimString& connectionString)const;

   /**
   * @param typeList appends to the typeList all the type names this factory
   * supports
   */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

protected:
   static ossimGdalElevationDatabaseFactory* m_instance;
};
#endif

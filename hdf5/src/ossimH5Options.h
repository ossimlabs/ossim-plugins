#ifndef ossimH5Options_HEADER
#define ossimH5Options_HEADER
#include <ossim/base/ossimKeywordlist.h>
#include <vector>
class ossimH5Options
{
public:
   typedef std::vector<std::string> StringListType;

   static ossimH5Options* instance();

   /*!
    * METHOD: loadPreferences()
    * These methods clear the current preferences and load either the default
    * preferences file or the specified file. Returns TRUE if loaded properly:
    */
   bool loadOptions(const ossimKeywordlist& kwl, const char* prefix);

   ossim_uint32 getMaxRecursionLevel()const;

   bool isDatasetRenderable(const std::string& datasetName)const;
   bool isDatasetExcluded(const std::string& datasetName)const;
   const StringListType& getRenderableDataset()const;

protected:
   /*!
    * Override the compiler default constructors:
    */
   ossimH5Options();
   ossimH5Options(const ossimH5Options&) {}

   void operator = (const ossimH5Options&) const {}

   static void loadRenderableDatasetsFromString(StringListType& result, 
                                                const ossimString& datasets);
   static ossimH5Options* m_instance;

   ossim_uint32 m_maxRecursionLevel;
   StringListType m_renderableDatasets;
};

#endif

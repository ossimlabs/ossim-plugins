#include <ossimH5Options.h>
#include <ossim/base/ossimPreferences.h>
#include <algorithm>

const char* MAX_RECURSION_LEVEL_KW="max_recursion_level";
const char* RENDERABLE_DATASETS_KW="renderable_datasets";

ossimH5Options* ossimH5Options::m_instance=0;

ossimH5Options::ossimH5Options()
: m_maxRecursionLevel(8)
{
  m_renderableDatasets.push_back("/All_Data/VIIRS-DNB-SDR_All/Radiance");

  loadOptions(ossimPreferences::instance()->preferencesKWL(), "hdf5.options.");
}

ossimH5Options* ossimH5Options::instance()
{
   /*!
    * Simply return the instance if already created:
    */
   if (m_instance)
   {
      return m_instance;
   }

   /*!
    * Create the static instance of this class:
    */
   m_instance = new ossimH5Options();

   return m_instance;
}
 
ossim_uint32 ossimH5Options::getMaxRecursionLevel()const
{
  return m_maxRecursionLevel;
}  

bool ossimH5Options::isDatasetRenderable(const std::string& datasetName)const
{
  return std::binary_search (m_renderableDatasets.begin(), 
                                m_renderableDatasets.end(), 
                                datasetName);
}

const ossimH5Options::StringListType& ossimH5Options::getRenderableDataset()const
{
  return m_renderableDatasets;
}

void ossimH5Options::loadRenderableDatasetsFromString(StringListType& result, const ossimString& datasets)
{
  std::vector<ossimString> splitList;
  result.clear();
  datasets.split(splitList, ",");
  ossim_uint32 idx = 0;
  for(;idx < splitList.size();++idx)
  {
    ossimString tempString = splitList[idx].trim();
    if(!tempString.empty()) result.push_back(tempString);
  }

  std::sort(result.begin(), result.end());
}

bool ossimH5Options::loadOptions(const ossimKeywordlist& kwl, const char* prefix)
{
  bool result = true;

  const char* maxRecursionLevel = kwl.find(prefix, MAX_RECURSION_LEVEL_KW);
  if(maxRecursionLevel)
  {
     m_maxRecursionLevel = ossimString(maxRecursionLevel).toUInt32();
     if(m_maxRecursionLevel <= 0) m_maxRecursionLevel = 8;
  }
  const char* renderableDatasets = kwl.find(prefix, RENDERABLE_DATASETS_KW);
  if(renderableDatasets)
  {
    loadRenderableDatasetsFromString(m_renderableDatasets, ossimString(renderableDatasets));
  }

  return result;
}

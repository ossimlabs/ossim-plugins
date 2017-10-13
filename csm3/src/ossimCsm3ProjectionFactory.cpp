//*****************************************************************************
// License:  See top level LICENSE.txt file.
//
// DESCRIPTION:
//   Contains implementation of class ossimCsm3ProjectionFactory
//
//*****************************************************************************
//  $Id: ossimCsm3ProjectionFactory.cpp 1577 2015-06-05 18:47:18Z cchuah $

#include "ossimCsm3ProjectionFactory.h"
#include "ossimCsm3SensorModel.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/base/ossimTrace.h>

static ossimTrace traceDebug("ossimCsm3ProjectionFactory:debug");

ossimCsm3ProjectionFactory* ossimCsm3ProjectionFactory::theInstance = 0;

ossimCsm3ProjectionFactory* ossimCsm3ProjectionFactory::instance()
{
    if(!theInstance)
    {
        theInstance = new ossimCsm3ProjectionFactory;
    }

    return (ossimCsm3ProjectionFactory*) theInstance;
}

ossimCsm3ProjectionFactory::ossimCsm3ProjectionFactory()
{
   // Instantiate once to load CSM model dynamic libs
   ossimCsm3Loader csml;
}


ossimProjection* ossimCsm3ProjectionFactory::createProjection(const ossimFilename& filename,
                                                             ossim_uint32 entryIdx ) const
{
    string filenameStr = filename.string();
    ossimProjection* result = ossimCsm3Loader::getSensorModel(filenameStr, entryIdx);
    return result;
}

ossimProjection* ossimCsm3ProjectionFactory::createProjection(const ossimKeywordlist &keywordList,
                                                             const char *prefix) const
{
    const char *lookup = keywordList.find(prefix, ossimKeywordNames::TYPE_KW);
    ossimProjection* result = 0;
    if(lookup)
    {
        result = createProjection(ossimString(lookup));
        if(result)
        {
            result->loadState(keywordList); 	

        }
    }
    return result;
}

ossimProjection* ossimCsm3ProjectionFactory::createProjection(const ossimString &name) const
{
    ossimProjection* result = 0;
   
    if(name == STATIC_TYPE_NAME(ossimCsm3SensorModel))
    {
        result = new ossimCsm3SensorModel();
    }
   
    return result;
}

ossimObject* ossimCsm3ProjectionFactory::createObject(const ossimString& typeName)const
{
    return createProjection(typeName);
}

ossimObject* ossimCsm3ProjectionFactory::createObject(const ossimKeywordlist& kwl,
                                                     const char* prefix)const
{
    return createProjection(kwl, prefix);
}

void ossimCsm3ProjectionFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
    typeList.push_back(STATIC_TYPE_NAME(ossimCsm3SensorModel));
}

std::list<ossimString> ossimCsm3ProjectionFactory::getList()const
{
    std::list<ossimString> result;
    std::string pError;

	vector<string> pluginList;
	ossimCsm3Loader::getAvailablePluginNames(pluginList);
    for (int i=0; i<pluginList.size(); i++)
    {
        vector<string> sensorList;
        ossimCsm3Loader::getAvailableSensorModelNames( pluginList[i], sensorList );
        std::copy (sensorList.begin (), sensorList.end (), std::back_inserter (result));
    }

    return result;
}

//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Ogr Info object.
// 
//----------------------------------------------------------------------------
// $Id: ossimOgrInfo.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

#include <ossimOgrInfo.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordlist.h>

#include <ogr_api.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

// Static trace for debugging
static ossimTrace traceDebug("ossimOgrInfo:debug");
static ossimTrace traceDump("ossimOgrInfo:dump"); // This will dump offsets.

ossimString getKeyValue(ossimString metaPrefix,
                        ossimString prefix,
                        ossimString metaNameValue,
                        ossimKeywordlist& kwl)
{
  if (!metaNameValue.contains("{") && !metaNameValue.contains("}"))
  {
    std::vector<int> indexVector;
    ossimString name = metaNameValue.split(":")[0].downcase().trim().substitute(" ", "_", true);
    ossimString keyValue = ossimString(metaPrefix + prefix + name);
    std::vector<ossimString> allMatchKeys;
    kwl.findAllKeysThatMatch(allMatchKeys, keyValue);

    if (allMatchKeys.size() == 0)
    {
      return name;
    }
    else
    {
      for (ossim_uint32 i = 0; i < allMatchKeys.size(); i++)
      {
        ossimString keyMatchValue = allMatchKeys[i];
        ossimString intValue = keyMatchValue.after(keyValue);
        if (!intValue.empty())
        {
          indexVector.push_back(intValue.toInt());
        }
      }
      if (indexVector.size() == 0) //only found one entry, e.g vpf.cat.Coverage_name
      {
        const char* tmpValue = kwl.find(keyValue);
        ossimString metaValue = tmpValue;
        kwl.remove(keyValue);
        ossimString newPrefix = ossimString(prefix + name + ossimString::toString(0));
        kwl.add(metaPrefix, 
                newPrefix,
                metaValue,
                true);
        return ossimString(name + ossimString::toString(1));
      }
      else // e.g vpf.cat.Coverage_name0 found
      {
        double max = 0; 
        for (ossim_uint32 i = 0; i < indexVector.size(); i++)
        {
          if (max < indexVector.at(i)) 
          {
            max = indexVector.at(i);
          }
        }
        return ossimString(name + ossimString::toString(max+1));
      }
    }
  }
  return ossimString("");
}

ossimOgrInfo::ossimOgrInfo()
   : ossimInfoBase(),
     m_file(),
     m_ogrDatasource(0),
     m_ogrDriver(0)
{
}

ossimOgrInfo::~ossimOgrInfo()
{
  if (m_ogrDatasource != NULL)
  {
    OGRDataSource::DestroyDataSource(m_ogrDatasource);
    m_ogrDatasource = 0;
  }
}

bool ossimOgrInfo::open(const ossimFilename& file)
{
   if ( m_ogrDatasource )
   {
      OGRDataSource::DestroyDataSource(m_ogrDatasource);
      m_ogrDatasource = 0;
   }

   // Below interface removed in gdal.
   // m_file = file;
   // m_ogrDatasource = OGRSFDriverRegistrar::Open(file.c_str(), false, &m_ogrDriver);

   m_ogrDatasource = (OGRDataSource*) OGROpen( file.c_str(), false, NULL );
   if ( m_ogrDatasource )
   {
      m_ogrDriver = (OGRSFDriver*) m_ogrDatasource->GetDriver();
      if ( m_ogrDriver )
      {
         m_file = file;
      }

      if ( !m_ogrDriver ) 
      {
         OGRDataSource::DestroyDataSource( m_ogrDatasource );
         m_ogrDatasource = 0;
      }
   }

   return ( m_ogrDatasource ? true : false );
}

void ossimOgrInfo::parseMetadata(ossimString metaData, 
                                 ossimKeywordlist& kwl, 
                                 ossimString metaPrefix) const
{
   std::vector<ossimString> metaList = metaData.split("\n");
   bool isCat = false;	
   bool isLat = false;
   bool isDht = false;
   bool isLht = false;
   bool isGrt = false;
   bool isDqt = false;
   bool isFcs = false;
   bool isFca = false;

   ossimString catKeyPrefix = "cat.";
   ossimString latKeyPrefix = "lat.";
   ossimString dhtKeyPrefix = "dht.";
   ossimString lhtKeyPrefix = "lht.";
   ossimString grtKeyPrefix = "grt.";
   ossimString dqtKeyPrefix = "dqt.";
   ossimString fcsKeyPrefix = "fcs.";
   ossimString fcaKeyPrefix = "fca.";
   
   for (ossim_uint32 i = 0; i < metaList.size(); i++)
   {
     ossimString metaNameValue = metaList[i].trim();
     ossimString metaStr = metaList[i].downcase().trim();
     if (metaStr.contains("(cat)"))
     {
       isCat = true;
       isLat = false; //set others to false
       isDht = false;
       isLht = false;
       isGrt = false;
       isDqt = false;
       isFcs = false;
       isFca = false;
       continue;
     }
     if (isCat)
     {
       if (metaStr.contains(":") && 
           !metaStr.contains("(lat)") && 
           !metaStr.contains("(dht)") && 
           !metaStr.contains("(lht)") && 
           !metaStr.contains("(grt)") && 
           !metaStr.contains("(dqt)") && 
           !metaStr.contains("(fcs)") && 
           !metaStr.contains("} } }  { family"))
       {
         ossimString keyValue = getKeyValue(metaPrefix, catKeyPrefix, 
                                            metaNameValue, kwl);
         ossimString metaValue = metaNameValue.split(":")[1].trim();
         ossimString prefix = ossimString(catKeyPrefix + keyValue);
         kwl.add(metaPrefix,
           prefix,
           metaValue,
           false);
       }
     }

     if (metaStr.contains("(lat)"))
     {
       isLat = true;
       isCat = false; //set others to false
       isDht = false;
       isLht = false;
       isGrt = false;
       isDqt = false;
       isFcs = false;
       isFca = false;
       continue;
     }
     if (isLat)
     {
       if (metaStr.contains(":") && 
         !metaStr.contains("(cat)") && 
         !metaStr.contains("(dht)") && 
         !metaStr.contains("(lht)") && 
         !metaStr.contains("(grt)") && 
         !metaStr.contains("(dqt)") && 
         !metaStr.contains("(fcs)") && 
         !metaStr.contains("} } }  { family"))
       {
         //do nothing. no need for lat info for now.
       }
     }

     if (metaStr.contains("(dht)"))
     {
       isDht = true;
       isCat = false;
       isLat = false;
       isLht = false;
       isGrt = false;
       isDqt = false;
       isFcs = false;
       isFca = false;
       continue;
     }
     if (isDht)
     {
       if (metaStr.contains(":") && 
         !metaStr.contains("(lat)") && 
         !metaStr.contains("(cat)") && 
         !metaStr.contains("(lht)") && 
         !metaStr.contains("(grt)") && 
         !metaStr.contains("(dqt)") && 
         !metaStr.contains("(fcs)") && 
         !metaStr.contains("} } }  { family"))
       {
         ossimString keyValue = getKeyValue(metaPrefix, dhtKeyPrefix, 
                                            metaNameValue, kwl);
         ossimString metaValue = metaNameValue.split(":")[1].trim();
         ossimString prefix = ossimString(dhtKeyPrefix + keyValue);
         kwl.add(metaPrefix,
           prefix,
           metaValue,
           false);
       }
     }

     if (metaStr.contains("(lht)"))
     {
       isLht = true;
       isDht = false;
       isCat = false;
       isLat = false;
       isGrt = false;
       isDqt = false;
       isFcs = false;
       isFca = false;
       continue;
     }
     if (isLht)
     {
       if (metaStr.contains(":") && 
         !metaStr.contains("(lat)") && 
         !metaStr.contains("(dht)") && 
         !metaStr.contains("(cat)") && 
         !metaStr.contains("(grt)") && 
         !metaStr.contains("(dqt)") && 
         !metaStr.contains("(fcs)") && 
         !metaStr.contains("} } }  { family"))
       {
         ossimString keyValue = getKeyValue(metaPrefix, lhtKeyPrefix, 
                                            metaNameValue, kwl);
         ossimString metaValue = metaNameValue.split(":")[1].trim();
         ossimString prefix = ossimString(lhtKeyPrefix + keyValue);
         kwl.add(metaPrefix,
           prefix,
           metaValue,
           false);
       }
     }

     if (metaStr.contains("(grt)"))
     {
       isGrt = true;
       isLht = false;
       isDht = false;
       isCat = false;
       isLat = false;
       isDqt = false;
       isFcs = false;
       isFca = false;
       continue;
     }
     if (isGrt)
     {
       if (metaStr.contains(":") && 
         !metaStr.contains("(lat)") && 
         !metaStr.contains("(dht)") && 
         !metaStr.contains("(lht)") && 
         !metaStr.contains("(cat)") && 
         !metaStr.contains("(dqt)") && 
         !metaStr.contains("(fcs)") && 
         !metaStr.contains("} } }  { family"))
       {
         ossimString keyValue = getKeyValue(metaPrefix, grtKeyPrefix, 
                                            metaNameValue, kwl);
         ossimString metaValue = metaNameValue.split(":")[1].trim();
         ossimString prefix = ossimString(grtKeyPrefix + keyValue);
         kwl.add(metaPrefix,
           prefix,
           metaValue,
           false);
       }
     }

     if (metaStr.contains("(dqt)"))
     {
       isDqt = true;
       isGrt = false;
       isLht = false;
       isDht = false;
       isCat = false;
       isLat = false;
       isFcs = false;
       isFca = false;
       continue;
     }
     if (isDqt)
     {
       if (metaStr.contains(":") && 
         !metaStr.contains("(lat)") && 
         !metaStr.contains("(dht)") && 
         !metaStr.contains("(lht)") && 
         !metaStr.contains("(grt)") && 
         !metaStr.contains("(cat)") && 
         !metaStr.contains("(fcs)") && 
         !metaStr.contains("} } }  { family"))
       {
         ossimString keyValue = getKeyValue(metaPrefix, dqtKeyPrefix, 
                                            metaNameValue, kwl);
         ossimString metaValue = metaNameValue.split(":")[1].trim();
         ossimString prefix = ossimString(dqtKeyPrefix + keyValue);
         kwl.add(metaPrefix,
           prefix,
           metaValue,
           false);
       }
     }

     if (metaStr.contains("(fcs)"))
     {
       isFcs = true;
       isDqt = false;
       isGrt = false;
       isLht = false;
       isDht = false;
       isCat = false;
       isLat = false;
       isFca = false;
       continue;
     }
     if (isFcs)
     {
       if (metaStr.contains(":") && 
         !metaStr.contains("(lat)") && 
         !metaStr.contains("(dht)") && 
         !metaStr.contains("(lht)") && 
         !metaStr.contains("(grt)") && 
         !metaStr.contains("(dqt)") && 
         !metaStr.contains("(cat)") && 
         !metaStr.contains("} } }  { family"))
       {
         ossimString keyValue = getKeyValue(metaPrefix, fcsKeyPrefix, 
                                            metaNameValue, kwl);
         ossimString metaValue = metaNameValue.split(":")[1].trim();
         ossimString prefix = ossimString(fcsKeyPrefix + keyValue);
         kwl.add(metaPrefix,
           prefix,
           metaValue,
           false);
       }
     }

     if (metaStr.contains("} } }  { family"))
     {
       isFca = true;
       isDqt = false;
       isGrt = false;
       isLht = false;
       isDht = false;
       isCat = false;
       isLat = false;
       isFcs = false;
     }
     if (isFca)
     {
       if (!metaStr.contains("(lat)") && 
         !metaStr.contains("(dht)") && 
         !metaStr.contains("(lht)") && 
         !metaStr.contains("(grt)") && 
         !metaStr.contains("(dqt)") && 
         !metaStr.contains("(fcs)"))
       {
         std::vector<ossimString> fcaTmpVector = metaNameValue.split("}}");
         ossimString fcaClassName;
         for (ossim_uint32 i = 0; i < fcaTmpVector.size(); i++)
         {
           ossimString fcaTemp = fcaTmpVector[i].trim();
           if (!fcaTemp.empty())
           {
             if (fcaTemp.contains("family"))
             {
               if (fcaTemp.split(" ").size() > 1)
               {
                 fcaClassName = fcaTemp.split(" ")[2].trim();
               }
             }
             else
             {
               std::vector<ossimString> fcaValues;
               if (fcaTemp.contains("<Grassland>displaymetadata {"))
               {
                 ossimString displaymetadataVector = fcaTemp.after("<Grassland>displaymetadata {").trim();
                 if (!displaymetadataVector.empty())
                 {
                   fcaValues = displaymetadataVector.split("{");
                 }
               }
               else
               {
                 fcaValues = fcaTemp.split("{");
               }

               if (fcaValues.size() > 2)
               {
                 ossimString fcaKey = fcaValues[1].trim();
                 ossimString fcaValue = fcaValues[2].trim();

                 ossimString prefix = ossimString(fcaKeyPrefix + fcaClassName + "." + fcaKey);
                 kwl.add(metaPrefix,
                   prefix,
                   fcaValue,
                   false);
               }
             }
           }          
         }
       }
     }
   }
}

bool ossimOgrInfo::getKeywordlist(ossimKeywordlist& kwl) const
{  
  if (m_ogrDatasource != NULL)  
  {
    ossimString driverName = getDriverName(ossimString(GDALGetDriverShortName(m_ogrDriver)).downcase());

    ossimString metaPrefix = ossimString(driverName + ".");

    //get meta data
    ossimString strValue;
    char** metaData = 0; // tmp drb m_ogrDatasource->GetMetadata("metadata");
    ossimString keyName = "tableinfo";
    size_t nLen = strlen(keyName.c_str());
    if (metaData != NULL && driverName == "vpf")
    {
      while(*metaData != NULL)
      {
        if (EQUALN(*metaData, keyName.c_str(), nLen)
          && ( (*metaData)[nLen] == '=' || 
          (*metaData)[nLen] == ':' ) )
        {
          strValue = (*metaData)+nLen+1;
        }
        metaData++;
      }

      if (!strValue.empty())
     {
        parseMetadata(strValue, kwl, metaPrefix);
      }
   }

    //get geometry data
    ossimString geomType;
    int layerCount = m_ogrDatasource->GetLayerCount();
    ossimString prefixInt = ossimString(metaPrefix + "layer");
    for(int i = 0; i < layerCount; ++i)
    {
      ossimString prefix = prefixInt + ossimString::toString(i) + ".";
      ossimString specialPrefix = "layer" + ossimString::toString(i) + ".";

      OGRLayer* layer = m_ogrDatasource->GetLayer(i);
      if(layer)
      {
        //get feature count and geometry type
        int featureCount = layer->GetFeatureCount();
        const char* layerName = layer->GetLayerDefn()->GetName();

        OGRFeature* feature = layer->GetFeature(0);
        if(feature)
        {
          OGRGeometry* geom = feature->GetGeometryRef(); 
          if(geom)
          {
            switch(geom->getGeometryType())
            {
            case wkbMultiPoint:
            case wkbMultiPoint25D:
              {
                geomType = "Multi Points";
                break;
              }
            case wkbPolygon25D:
            case wkbPolygon:
              {
                geomType = "Polygon";
                break;
              }
            case wkbLineString25D:
            case wkbLineString:
              {
                geomType = "LineString";
                break;
              }
            case wkbPoint:
            case wkbPoint25D:
              {
                geomType = "Points";
                break;
              }
            case wkbMultiPolygon25D:
            case wkbMultiPolygon:
              {
                geomType = "Multi Polygon";
                break;

              }
            default:
              {
                geomType = "Unknown Type";
                break;
              }
            }
          }
        }//end if feature
        OGRFeature::DestroyFeature(feature);

        kwl.add(specialPrefix,
          "features",
          featureCount,
          true);

        //get all attribute fields
        OGRFeatureDefn* featureDefn = layer->GetLayerDefn();
        if (featureDefn)
        {
          for(int iField = 0; iField < featureDefn->GetFieldCount(); iField++ )
          {
            OGRFieldDefn* fieldDefn = featureDefn->GetFieldDefn(iField);
            ossimString fieldType;
            if (fieldDefn)
            {
              ossimString fieldName = ossimString(fieldDefn->GetNameRef());
              if( fieldDefn->GetType() == OFTInteger )
              {
                fieldType = "Integer";
              }
              else if( fieldDefn->GetType() == OFTReal )
              {
                fieldType = "Real";
              }
              else if( fieldDefn->GetType() == OFTString )
              {
                fieldType = "String";
              }
              else if ( fieldDefn->GetType() == OFTWideString)
              {
                fieldType = "WideString";
              }
              else if ( fieldDefn->GetType() == OFTBinary )
              {
                fieldType = "Binary";
              }
              else if ( fieldDefn->GetType() == OFTDate )
              {
                fieldType = "Date";
              }
              else if ( fieldDefn->GetType() == OFTTime )
              {
                fieldType = "Time";
              }
              else if ( fieldDefn->GetType() == OFTDateTime )
              {
                fieldType = "DateTime";
              }
              else
              {
                fieldType = "String";
              }

              ossimString fieldInfo = ossimString(fieldName + " (" + fieldType + ")");

              ossimString colPrefix = prefix + "column" + ossimString::toString(iField);
              kwl.add(colPrefix,
                "",
                fieldInfo,
                true);
            }
          }
        }

        kwl.add(specialPrefix,
          "name",
          layerName,
          true);

        kwl.add(specialPrefix,
          "geometry",
          geomType,
          true);
      } //end if layer
    }// end i
    return true;
  }// end if datasource
  return false;
}

std::ostream& ossimOgrInfo::print(std::ostream& out) const
{
  static const char MODULE[] = "ossimOgrInfo::print";
  if (traceDebug())
  {    
     ossimNotify(ossimNotifyLevel_DEBUG)
      << MODULE << " DEBUG Entered...\n";
  }
  return out;
}

ossimString ossimOgrInfo::getDriverName(ossimString driverName) const
{
  if (driverName == "esri shapefile")
  {
    return "shp";
  }
  else if (driverName == "ogdi")
  {
    return "vpf";
  }
  else
  {
    return driverName;
  }
}

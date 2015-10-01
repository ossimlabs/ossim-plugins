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
// $Id: ossimHdfInfo.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

//gdal includes
#include <gdal_priv.h>
#include <cpl_string.h>

//ossim includes
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossimHdfInfo.h>

// Static trace for debugging
static ossimTrace traceDebug("ossimHdfInfo:debug");

ossimHdfInfo::ossimHdfInfo()
   : ossimInfoBase(),
     theFile(),
     m_hdfReader(0),
     m_driverName()
{
}

ossimHdfInfo::~ossimHdfInfo()
{
   m_hdfReader = 0;
   m_globalMeta.clear();
   m_globalMetaVector.clear();
}

bool ossimHdfInfo::open(const ossimFilename& file)
{
   static const char MODULE[] = "ossimHdfInfo::open";
   if (traceDebug())
   {    
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n"
         << "file: " << file << "\n";
   }

   bool result = false;
   
   ossimString ext = file.ext();
   ext.downcase();

   if ( ext == "hdf" || ext == "h4" || ext == "hdf4" || 
     ext == "he4" || ext == "hdf5" || ext == "he5" || ext == "h5" || 
     ext == "l1r")
   {
     m_hdfReader = new ossimHdfReader;
     m_hdfReader->setFilename(file);

     if ( m_hdfReader->open() )
     {
       m_driverName = m_hdfReader->getDriverName();
       if (m_driverName.contains("HDF4"))
       {
         m_driverName = "hdf4";
       }
       else if (m_driverName.contains("HDF5"))
       {
         m_driverName = "hdf5";
       }

       theFile = file;
       m_globalMeta.clear();
       m_globalMetaVector.clear();
       GDALDatasetH dataset = GDALOpen(theFile.c_str(), GA_ReadOnly);
       if (dataset != 0)
       {
          char** papszMetadata = GDALGetMetadata(dataset, NULL);
          if( CSLCount(papszMetadata) > 0 )
          {
             for(ossim_uint32 metaIndex = 0; papszMetadata[metaIndex] != 0; ++metaIndex)
             {
                ossimString metaInfo = papszMetadata[metaIndex];
                if (metaInfo.contains("="))
                {
                   std::vector<ossimString> metaInfos = metaInfo.split("=");
                   if (metaInfos.size() > 1)
                   {
                      ossimString key = metaInfos[0];
                      ossimString keyStr = key.substitute(":", ".", true);
                      keyStr = keyStr + ": ";
                      ossimString valueStr = metaInfos[1];
                      m_globalMeta[keyStr] = valueStr;
                      m_globalMetaVector.push_back(ossimString(keyStr + valueStr));
                   }
                }
             }
          }
       }
       GDALClose(dataset);

       result = true;
     }
   }

   if (traceDebug())
   {    
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true\n":"false\n");
   }

   return result;
}

std::ostream& ossimHdfInfo::print(std::ostream& out) const
{
   static const char MODULE[] = "ossimHdfInfo::print";
   if (traceDebug())
   {    
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimString prefix = ossimString(m_driverName + ".").downcase();

   //print out global meta first
   for (ossim_uint32 i = 0; i < m_globalMetaVector.size(); i++)
   {
       out << prefix << m_globalMetaVector[i] << "\n";
   }

   ossim_uint32 entryNum = m_hdfReader->getNumberOfEntries();
   for (ossim_uint32 i = 0; i < entryNum; i++)
   {
      ossimString imagePrefix = "image" + ossimString::toString(i) + ".";
      ossimString fileName = m_hdfReader->getEntryString(i);
      if (!fileName.empty())
      {
         GDALDatasetH dataset = GDALOpen(fileName.c_str(), GA_ReadOnly);
         if (dataset != 0)
         {
            ossim_uint32 numOfBands = GDALGetRasterCount(dataset);
            for (ossim_uint32 j = 0; j < numOfBands; j++)
            {
               ossimString bandPrefix = "band" + ossimString::toString(j) + ".";
               ossimString prefixStr = prefix + imagePrefix + bandPrefix;
               ossimString nameStr = "name: ";
               ossimString subDatasetName = fileName;
               std::vector<ossimString> subFileList = fileName.split(":");
               if (subFileList.size() > 2)
               {
                  if (m_driverName == "hdf4")
                  {
                     subDatasetName = subFileList[1] + ":" + subFileList[subFileList.size() - 2] + ":" + subFileList[subFileList.size() - 1];
                  }
                  else if (m_driverName == "hdf5")
                  {
                     subDatasetName = subFileList[subFileList.size() - 1];
                  }
               }
               out << prefixStr << nameStr << subDatasetName << "\n";

               char** papszMetadata = GDALGetMetadata(dataset, NULL);
               if( CSLCount(papszMetadata) > 0 )
               {
                  for(ossim_uint32 metaIndex = 0; papszMetadata[metaIndex] != 0; ++metaIndex)
                  {
                     ossimString metaInfo = papszMetadata[metaIndex];
                     if (metaInfo.contains("="))
                     {
                        std::vector<ossimString> metaInfos = metaInfo.split("=");
                        if (metaInfos.size() > 1)
                        {
                           ossimString key = metaInfos[0];
                           ossimString keyStr = key.substitute(":", ".", true);
                           keyStr = keyStr + ": ";
                           ossimString valueStr = metaInfos[1];
                           std::map<ossimString, ossimString, ossimStringLtstr>::const_iterator itSub = m_globalMeta.find(keyStr);
                           if (itSub == m_globalMeta.end())//avoid to print global again
                           {
                              out << prefixStr << keyStr << valueStr << "\n";
                           } 
                        }
                     }
                  }
               }
            }
            GDALClose(dataset);
         }//end if (dataset != 0)
      }//end if (!fileName.empty())
   }//end for
   
   if (traceDebug())
   {    
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return out;
}

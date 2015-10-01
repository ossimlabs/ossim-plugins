#include "ossimKakaduJpipInfo.h"
#include "ossimKakaduJpipHandler.h"
#include <ossim/base/ossimUrl.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimXmlDocument.h>
#include <ossim/support_data/ossimTiffInfo.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>

//---
// Kakadu code has defines that have (kdu_uint32) which should be
// (kdu_core::kdu_uint32)
//---
using namespace kdu_core;

ossimKakaduJpipInfo::ossimKakaduJpipInfo()
{
   
}

ossimKakaduJpipInfo::~ossimKakaduJpipInfo()
{
   
}

bool ossimKakaduJpipInfo::open(const ossimFilename& file)
{
   m_handler = new ossimKakaduJpipHandler();
   ossimImageHandler* handler = m_handler.get();
   bool result = false;
   ossimUrl url(file.c_str());
   ossimString protocol = url.getProtocol().downcase();
   if((protocol == "jpip")||
      (protocol == "jpips"))
   {
      result = handler->open(file);

   }
   else
   {
      url = ossimUrl();
      if(file.ext() == "jpip")
      {
         ossimKeywordlist kwl;
         
         if(kwl.addFile(file.c_str()))
         {
            result = handler->loadState(kwl);
         }
      }
   }
   if(!result) m_handler = 0;
   
   return result;
}

std::ostream& ossimKakaduJpipInfo::print(std::ostream& out) const
{
   if(m_handler.valid())
   {
      ossimKeywordlist kwl;
      ossimString xml;
      ossimKakaduJpipHandler::BoxList boxes;
      m_handler->extractBoxes(boxes);
      
      ossim_uint32 idx = 0;
      for(idx = 0; idx < boxes.size(); ++idx)
      {
         if(boxes[idx].m_type == jp2_xml_4cc)
         {
            ossimRefPtr<ossimXmlDocument> xmlDoc = new ossimXmlDocument();
            //std::istringstream in(ossimString((char*)&boxes[idx].m_buffer.front(),
            //                                  (char*)&boxes[idx].m_buffer.front() + boxes[idx].m_buffer.size()));
            ossimByteStreamBuffer byteStreamBuf((char*)&boxes[idx].m_buffer.front(), boxes[idx].m_buffer.size());
            std::iostream in(&byteStreamBuf);
            
        //    std::cout << "________________________________________________" << std::endl;
        //    std::cout << ossimString((char*)&boxes[idx].m_buffer.front(),
        //                             (char*)&boxes[idx].m_buffer.front() + boxes[idx].m_buffer.size()) << std::endl;
        //    std::cout << "________________________________________________" << std::endl;
            if(ossimString((char*)&boxes[idx].m_buffer.front(), (char*)&boxes[idx].m_buffer.front()+4) ==
               "<?xm")
            {
               xmlDoc->read(in);
            }
            else
            {
               ossimRefPtr<ossimXmlNode> node = new ossimXmlNode();
               if(node->read(in))
               {
                  xmlDoc->initRoot(node);
               }
            }
            if(xmlDoc->getRoot().valid())
            {
               ossimRefPtr<ossimXmlNode> node = new ossimXmlNode();
               node->setTag("jpip.image0");
               node->addChildNode(xmlDoc->getRoot().get());
               xmlDoc->initRoot(node.get());
               xmlDoc->toKwl(kwl);
               out << kwl;
            }
         }
         else if(boxes[idx].m_type == jp2_uuid_4cc)
         {
            kdu_byte geojp2_uuid[16] = {0xB1,0x4B,0xF8,0xBD,0x08,0x3D,0x4B,0x43,
               0xA5,0xAE,0x8C,0xD7,0xD5,0xA6,0xCE,0x03};
            
            if(memcmp(&boxes[idx].m_buffer.front(),geojp2_uuid, 16)==0)
            {
               ossimTiffInfo info;
               
               //---
               // Have geotiff boxes with badly terminated geotiffs. So to keep
               // the tag parser from walking past the first image file directory
               // (IFD) into garbage we will set the process overview flag to false.
               //
               // Note if we ever get multiple entries we will need to take this out.
               //---
               info.setProcessOverviewFlag(false);
               //ossimByteStreamBuffer buf((char*)&boxList[idx].m_buffer.front()+16, 
               //                          boxList[idx].m_buffer.size()-16);
               //std::istream str(&buf);
               std::ofstream outStr("/tmp/dump.bin", std::ios::out|std::ios::binary);
               outStr.write((char*)&boxes[idx].m_buffer.front(),
                            boxes[idx].m_buffer.size());
               outStr.close();
               std::istringstream str(ossimString((char*)&boxes[idx].m_buffer.front()+16,
                                                  (char*)&boxes[idx].m_buffer.front() + boxes[idx].m_buffer.size()));
               ossim_uint32 entry = 0;
               ossimKeywordlist kwl; // Used to capture geometry data. 
               // std::cout << ossimString((char*)&boxList[idx].m_buffer.front() + 16,
               //                          (char*)&boxList[idx].m_buffer.front() + boxList[idx].m_buffer.size()) << std::endl;
               if ( info.getImageGeometry(str, kwl, entry) )
               {
                  kwl.stripPrefixFromAll("image0.");
                  kwl.addPrefixToAll("jpip.image0.geojp2.");
                  out << kwl << std::endl;
                 // ossimRefPtr<ossimProjection> proj = ossimProjectionFactoryRegistry::instance()->createProjection(kwl, "image0.");
                  //if(proj.valid())
                 // {
                 //    ossimKeywordlist projKwl;
                 //    proj->saveState(projKwl,"jpip.image0.geojp2.");
                 //    out << projKwl << std::endl;
                 // }
               }
            }
            else
            {
            }
         }
      }
   }
   return out;
}

void ossimKakaduJpipInfo::setHandler(ossimKakaduJpipHandler* handler)
{
   m_handler = handler;
}


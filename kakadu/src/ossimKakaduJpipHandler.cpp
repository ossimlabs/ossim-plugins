#include "ossimKakaduJpipHandler.h"
#include <ossim/base/ossimUrl.h>
#include <ossim/base/ossimEndian.h>
#include <sstream>
//#include <ossim/base/ossimByteStreamBuffer.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimKeywordNames.h>
#include "ossimKakaduCommon.h"
#include <ossim/imaging/ossimImageDataFactory.h>
#include <OpenThreads/Thread>
#include <jpx.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimWebRequestFactoryRegistry.h>
#include "ossimKakaduJpipInfo.h"
#include <ossim/imaging/ossimImageGeometryRegistry.h>
#include <ossim/base/ossimTrace.h>

using namespace kdu_core;
using namespace kdu_supp;

static const ossimTrace traceDebug( ossimString("ossimKakaduJpipHandler:debug") );

RTTI_DEF1(ossimKakaduJpipHandler, "ossimKakaduJpipHandler", ossimImageHandler);
ossimKakaduJpipHandler::ossimKakaduJpipHandler()
:ossimImageHandler(),
m_headerClient(0), // let's cache the header information
m_client(0)
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimKakaduJpipHandler::ossimKakaduJpipHandler():  entered ..................\n";
   }
   m_imageBounds.makeNan();
   m_quality = 100.0;
}

ossimKakaduJpipHandler::~ossimKakaduJpipHandler()
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimKakaduJpipHandler::~ossimKakaduJpipHandler():  entered ..................\n";
   }
   close();
}

void ossimKakaduJpipHandler::close()
{
   ossimImageHandler::close();

   // Kakadu kdu_thread_entity::terminate throws exceptions...
   try
   {   
      m_jp2Family.close();
      if(m_client)
      {
         if(m_client->is_alive())
         {
            m_client->disconnect();
         }
         m_client->close();
         delete m_client;
         m_client = 0;
      }
      if(m_headerClient)
      {
         if(m_headerClient->is_alive())
         {
            m_headerClient->disconnect();
         }
         m_headerClient->close();
         delete m_headerClient;
         m_headerClient = 0;
      }
      m_request = 0;
   }
   catch ( kdu_core::kdu_exception exc )
   {
      ostringstream e;
      e << "ossimKakaduNitfReader::~ossimKakaduNitfReader\n"
        << "Caught exception from kdu_region_decompressor: " << exc << "\n";
      ossimNotify(ossimNotifyLevel_WARN) << e.str() << std::endl;
   }
   catch ( std::bad_alloc& )
   {
      std::string e =
         "Caught exception from kdu_region_decompressor: std::bad_alloc";
      ossimNotify(ossimNotifyLevel_WARN) << e << std::endl;
   }
   catch( ... )
   {
      std::string e =
         "Caught unhandled exception from kdu_region_decompressor";
      ossimNotify(ossimNotifyLevel_WARN) << e << std::endl;
   }   
   
   deleteRlevelCache();
}

bool ossimKakaduJpipHandler::isOpen()const
{
   return (m_client!=0);
}


int ossimKakaduJpipHandler::convertClassIdToKdu(int id)
{
   
   if(id >=0 &&id<=8)
   {
      return (id >> 1);
   }
#if 0
   switch(id)
   {
      case 0:
      case 1:
      {
         return KDU_PRECINCT_DATABIN;
      }
      case 2:
      {
         return KDU_TILE_HEADER_DATABIN;
      }
      case 4:
      case 5:
      {
         return KDU_TILE_DATABIN;
      }
      case 6:
      {
         return KDU_MAIN_HEADER_DATABIN;
      }
      case 8:
      {
         return KDU_META_DATABIN;
      }
      default:
      {
         break;
      }
   }
#endif
   return -1;
}

void ossimKakaduJpipHandler::showBoxes(kdu_supp::jp2_input_box* pParentBox)
{
   kdu_supp::jp2_input_box box;

   if(!pParentBox)
   {
      //std::cout << "DEBUG: SHOWING HEADER BOXES FOR STREAM" << std::endl;
      box.open(&m_jp2Family);
   }
   else
   {
      box.open(pParentBox);
   }
   while(box.exists())
   {
      kdu_uint32 boxType = box.get_box_type();
      char ch[] = { (char) ( boxType >> 24 ), (char) ( boxType >> 16 ), (char) ( boxType >> 8 ) , (char) ( boxType >> 0 ), '\0' };

      //std::cout << ch << std::endl;
      //std::cout << "Remaining bytes = " << box.get_remaining_bytes() << "\n";
      if ( box.get_box_type() == jp2_xml_4cc )
      {
         //std::cout << "XML_________________________________________________\n";
         std::vector<kdu_byte> bytes(box.get_remaining_bytes());
         box.read(&bytes.front(), bytes.size());
         
         //std::cout << ossimString((char*)&bytes.front(), ((char*)&bytes.front())+bytes.size() ) << std::endl;
      }
      if(box.get_box_type() == jp2_uuid_4cc)
      {
         std::vector<kdu_byte> bytes(box.get_remaining_bytes());
         box.read(&bytes.front(), bytes.size());
         ossim_uint32 idx = 0;
         //for(idx = 0; idx < 16; ++idx)
         //{
            //std::cout << (int)bytes[idx]<< " ";
         //}
         //std::cout << "\n";
      }
      if ( jp2_is_superbox( box.get_box_type() ) )
      {
         showBoxes( &box );
      }
      else
      {
      }
      box.close();
      box.open_next();
   }
}
void ossimKakaduJpipHandler::extractBoxes(BoxList& boxList)
{
   kdu_window window;
   window.init();
   m_headerClient->close();
   if(loadClient(m_headerClient, window))
   {
      m_headerClient->set_read_scope(KDU_MAIN_HEADER_DATABIN, 0, 0);
      kdu_codestream codestream;
      kdu_region_decompressor decompressor;
      codestream.create(m_headerClient);
      codestream.set_persistent();
      
      kdu_channel_mapping oChannels;
      oChannels.configure(codestream);
      
      kdu_coords ref_expansion(1, 1);
      kdu_dims view_dims = decompressor.get_rendered_image_dims(codestream, &oChannels, -1, 0,
                                                                ref_expansion, ref_expansion,
                                                                KDU_WANT_OUTPUT_COMPONENTS);
      siz_params* siz_in = codestream.access_siz();
      kdu_params* cod_in = siz_in->access_cluster("COD");
      m_jp2Family.close();
      m_client->set_read_scope(KDU_META_DATABIN, 0, 0);
      m_jp2Family.open(m_headerClient);
      
      extractBoxes(boxList, 0);
      m_jp2Family.close();
   }
}

void ossimKakaduJpipHandler::extractBoxes(BoxList& boxList, jp2_input_box* pParentBox)
{
   jp2_input_box box;
   
   if(!pParentBox)
   {
      //std::cout << "DEBUG: SHOWING HEADER BOXES FOR STREAM" << std::endl;
      box.open(&m_jp2Family);
   }
   else
   {
      box.open(pParentBox);
   }
   while(box.exists())
   {
      ossim_uint32 idx = boxList.size();
      boxList.push_back(Box());
      boxList[idx].m_type = box.get_box_type();
      if(box.get_remaining_bytes()>0)
      {
         boxList[idx].m_buffer.resize(box.get_remaining_bytes());
         box.read(&boxList[idx].m_buffer.front(), boxList[idx].m_buffer.size());
      }
      if ( jp2_is_superbox( box.get_box_type() ) )
      {
         extractBoxes(boxList, &box );
      }
      
      box.close();
      box.open_next();
   }
}

bool ossimKakaduJpipHandler::openStream()
{
   close();

   if(!m_client)
   {
      m_headerClient = new kdu_client();
      m_client       = new kdu_client();
   }
   bool result = false;
   ossimString protocolString = m_baseUrl.getProtocol();
   protocolString =  protocolString.downcase();
   m_useOurGrab = false;
   
   if(protocolString == "jpip")
   {
      m_baseUrl.setProtocol("http");
   }
   else
   {
      if(protocolString == "jpips")
      {
         m_useOurGrab = true;
         m_baseUrl.setProtocol("https");            
      }
      else
      {
         return false;
      }
   }
   //std::cout << "OUR GRAB ==== " << m_useOurGrab << std::endl;
  // m_useOurGrab = true;
   m_imageBounds.makeNan();
   kdu_window window;
   window.init();
   //std::cout << "calling loadClient " << std::endl;
   if(loadClient(m_client, window))
   {
      try 
      {
         m_client->set_read_scope(KDU_MAIN_HEADER_DATABIN, 0, 0);
         kdu_codestream codestream;
         kdu_region_decompressor decompressor;
         codestream.create(m_client);
         codestream.set_persistent();
         
         kdu_channel_mapping oChannels;
         oChannels.configure(codestream);
         
         
         
         kdu_coords ref_expansion(1, 1);
         kdu_dims view_dims = decompressor.get_rendered_image_dims(codestream, &oChannels, -1, 0,
                                                                   ref_expansion, ref_expansion,
                                                                   KDU_WANT_OUTPUT_COMPONENTS);
         siz_params* siz_in = codestream.access_siz();
         kdu_params* cod_in = siz_in->access_cluster("COD");
         m_jp2Family.close();
         m_client->set_read_scope(KDU_META_DATABIN, 0, 0);
         m_jp2Family.open(m_client);
         //std::cout <<"SHOWING BOXES!!!!!!!!!!!" << std::endl;
         showBoxes();
         if(cod_in&&siz_in)
         {
            //std::cout << "__________________________________====\n";
            int nResLevels = 0;
            int nQualityLayers = 0;
            int nTileSizeX = 0;
            int nTileSizeY = 0;
            
            cod_in->get("Clevels", 0, 0, nResLevels);
            cod_in->get("Clayers", 0, 0, nQualityLayers);
            siz_in->get("Stiles", 0, 0, nTileSizeY);  
            siz_in->get("Stiles", 0, 1, nTileSizeX);  
            m_nQualityLayers = nQualityLayers;
            m_nInputBands = codestream.get_num_components(false);
            m_nOutputBands = codestream.get_num_components(true);
            ossim_uint64 width =  view_dims.size.x;
            ossim_uint64 height = view_dims.size.y;
            //           ossim_int32 m_nQualityLayers = nQualityLayers;
            m_bitDepth = codestream.get_bit_depth(0);
            m_signed   = codestream.get_signed(0);
            m_resLevels = nResLevels;
            if(nTileSizeX&&nTileSizeY)
            {
               m_tileSize = ossimIpt( nTileSizeX, nTileSizeY);
            }
            else
            {
               m_tileSize = ossimIpt(2048, 128);
            }
            m_tileSize.x = m_tileSize.x>2048?2048:m_tileSize.x;
            m_tileSize.y = m_tileSize.y>128?128:m_tileSize.y;
            m_bYCC=1;
            cod_in->get("Cycc", 0, 0, m_bYCC);
            performRlevelSetup(codestream);
            
            m_jp2Family.close();
            result = true;
#if 0
            std::cout << std::dec << "width = " << width << std::endl;
            std::cout << std::dec << "height = " << height << std::endl;
            std::cout << "m_nOutputBands = " << m_nOutputBands << std::endl;
            std::cout << "m_nInputBands = " << m_nInputBands << std::endl;
            std::cout << "BitDepth   = " << m_bitDepth << std::endl;
            std::cout << "Overviews  = " << nResLevels<< std::endl;
            std::cout << "nQualityLayers  = " << m_nQualityLayers<< std::endl;
            std::cout << "nTileSize  = " << m_tileSize << std::endl;
#endif       
         }
         
         // walk boxes for testing
         //
         //               showBoxes(&box);
         // std::cout << "BOX OPENED!!!" << std::endl;
         //do {
         //   std::cout << "type: " <<std::ios::hex <<  box.get_box_type() << std::endl;
         //   std::cout << "SUPER? " << jp2_is_superbox(box.get_box_type()) << std::endl;
         //   box.close();
         //   box.open_next();
         //} while (box.exists());
         //            }
      } 
      catch(...)
      {
         result = false;
      }
   }
   
   if(result)
   {
      completeOpen(); 
   }
   else
   {
      close();
   }
   //std::cout << "RETURNING RESULT === " << result << std::endl;
   return result;
   
}

bool ossimKakaduJpipHandler::open()
{
   if(isOpen()) close();
   bool result = false;
   ossimFilename file = getFilename();
   // local file and we will test for jpip extension and this
   // will hold cached state information
   //
   if(file.exists())
   {
      if(file.ext().downcase()=="jpip")
      {
         ossimKeywordlist kwl;
         if(kwl.addFile(file))
         {
            result = loadState(kwl);
         }
      }
   }
   else
   {
      m_baseUrl = ossimUrl(file);
      result = openStream();
   }
   if(getOutputScalarType() == OSSIM_SCALAR_UNKNOWN)
   {
      result = false;
      close();
   }
   return result;
}

void ossimKakaduJpipHandler::allocateSession()
{
   m_request = ossimWebRequestFactoryRegistry::instance()->createHttp(m_baseUrl);
#if 0
   if(m_request.valid())
   {
      m_cid = "";
      m_path = "";
      m_transport ="";
      ossimString params;
      ossimUrl imageInfoUrl = m_baseUrl;
      params = "type=jpp-stream&stream=0&cnew=http";
      imageInfoUrl.setParams(params);
      std::cout << "URL------------------------- " << imageInfoUrl.toString() << std::endl;
      m_request->set(imageInfoUrl, ossimKeywordlist());
      m_request->addHeaderOption("HEADERS=Accept", "jpp-stream");
      ossimRefPtr<ossimHttpResponse> response = dynamic_cast<ossimHttpResponse*>(m_request->getResponse());
      if(response.valid()&&(response->getStatusCode()==200))
      {
         std::cout << "__________________________" << std::endl;
         std::cout << response->headerKwl() << std::endl;
         ossimString jpipCnew = response->headerKwl()["JPIP-cnew"];
         jpipCnew = jpipCnew.trim();
         if(!jpipCnew.empty())
         {
            m_cid = "";
            m_path = "";
            m_transport ="";
            ossimString tempKwlString = jpipCnew.substitute(",", "\n", true);
            std::istringstream in(tempKwlString.c_str());
            
            ossimKeywordlist kwl('=');
            if(kwl.parseStream(in))
            {
               m_cid = kwl["cid"];
               m_path = kwl["path"];
               m_transport = kwl["transport"];
               m_cid = m_cid.trim();
               m_path = m_path.trim();
               m_transport = m_transport.trim();
               
               std::cout << "PATH === " << m_path << std::endl;
            }
         }
      }
   }   
#endif
}
bool ossimKakaduJpipHandler::loadClient(kdu_client* client, kdu_window& window)
{
   bool result =false;

   if(!client) return result;
   // we will assume that the libcurl library support ssl connections and we will use our own 
   // urlquery instead of using kakadu's persistant code
   // if m_ourGrab is true we currently only support stateless requests.  Kakadu
   // interface we use if m_useOurGrab communicates through the channel session 
   // id.
   if(m_useOurGrab)
   {
      //if(!m_request.valid())
         allocateSession();
      
      client->close();
      bool needNewChannel = false;
#if 0
      if(!m_request.valid())
      {
         m_request = ossimWebRequestFactoryRegistry::instance()->createHttp(m_baseUrl);
         needNewChannel = true;
      }
#endif
      if(m_request.valid())
      {
         m_request->clearHeaderOptions();
         ossimString params;
         ossimUrl imageInfoUrl = m_baseUrl;
         if(window.region.size.x == 0&&
            window.region.size.y == 0)
         {
            
            params = "type=jpp-stream&stream=0";
         }
         else
         {
            params = "type=jpp-stream&stream=0";
            
            params += ("&roff=" + ossimString::toString((ossim_int64)window.region.pos.x)+","+ossimString::toString((ossim_int64)window.region.pos.y));
            params += ("&rsiz=" + ossimString::toString((ossim_int64)window.region.size.x)+","+ossimString::toString((ossim_int64)window.region.size.y));
            params += ("&fsiz=" + ossimString::toString((ossim_int64)window.resolution.x)+","+ossimString::toString((ossim_int64)window.resolution.y));
            params += ("&layers=" + ossimString::toString(window.get_max_layers()));
            
         }
         if(!m_cid.empty())
         {
            params += ("&cid=" + m_cid); 
           // if(!m_path.empty()) imageInfoUrl.setPath(m_path);
         }
         if(needNewChannel)
         {
            params += "&cnew=http";
         }
         imageInfoUrl.setParams(params);

         m_request->set(imageInfoUrl, ossimKeywordlist());
         m_request->addHeaderOption("HEADERS=Accept", "jpp-stream");
         
         //std::cout << "URL======================== " << imageInfoUrl.toString() << std::endl;
         ossimRefPtr<ossimHttpResponse> response = dynamic_cast<ossimHttpResponse*>(m_request->getResponse());
         //std::cout << "__________________________" << std::endl;
         if(response.valid()&&(response->getStatusCode()==200))
         {
//            std::cout << response->headerKwl() << std::endl;
           //std::cout << "__________________________" << std::endl;
            //std::cout << response->headerKwl() << std::endl;
#if 0
            ossimString jpipCnew = response->headerKwl()["JPIP-cnew"];
            jpipCnew = jpipCnew.trim();
            
            if(!jpipCnew.empty())
            {
               m_cid = "";
               m_path = "";
               m_transport ="";
               ossimString tempKwlString = jpipCnew.substitute(",", "\n", true);
               std::istringstream in(tempKwlString.c_str());
               
               ossimKeywordlist kwl('=');
               if(kwl.parseStream(in))
               {
                  m_cid = kwl["cid"];
                  m_path = kwl["path"];
                  m_transport = kwl["transport"];
                  m_cid = m_cid.trim();
                  m_path = m_path.trim();
                  m_transport = m_transport.trim();
                  
                  std::cout << "PATH === " << m_path << std::endl;
               }
            }
#endif       
            if(traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG) << ossimString(response->headerBuffer().buffer(),response->headerBuffer().buffer()+response->headerBuffer().bufferSize())  << std::endl;    
            }
            ossimWebResponse::ByteBuffer buf;
            response->copyAllDataFromInputStream(buf);
            if(buf.size())
            {
               try 
               {
                  ossimRefPtr<ossimJpipMessageDecoder> decoder = new ossimJpipMessageDecoder();
                  decoder->inputStreamBuffer().setBuf(reinterpret_cast<char*>(&buf.front()),
                                                      buf.size(),
                                                      true);
                  ossimRefPtr<ossimJpipMessage> message = decoder->readMessage();
                  ossimRefPtr<ossimJpipMessage> mainHeaderMessage;
                  if(message.valid())
                  {
                     ossim_uint32 dataWritten = 0;
                     while(message.valid()&&!message->header()->m_isEOR)
                     {
                        int id = convertClassIdToKdu(message->header()->m_classIdentifier);
                        if((!message->header()->m_isEOR)&&
                           (id >=0))
                        {
                           client->add_to_databin( id, 
                                                    message->header()->m_CSn,
                                                    message->header()->m_inClassIdentifier,
                                                    (const kdu_byte*)&message->messageBody().front(), 
                                                    message->header()->m_msgOffset, 
                                                    message->messageBody().size(),
                                                    message->header()->m_isLastByte);
                           
                           dataWritten+= message->messageBody().size();
                        }
                        message = decoder->readMessage();
                     }
                  }
                  result = true;
               } 
               catch(...) 
               {
                  //std::cout << "EXCEPTION!!!!!!!!!!!!!!!!!!!\n"; 
                  //std::cout << "-----------------------------------------------\n";
                  result = false;
               }
            }
         }
         else if(response.valid())
         {
            if(traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG) << ossimString(response->headerBuffer().buffer(),response->headerBuffer().buffer()+response->headerBuffer().bufferSize())  << std::endl;            
            }
            //std::cout << imageInfoUrl.toString() << std::endl;
            //std::cout << ossimString(response->headerBuffer().buffer(),response->headerBuffer().buffer()+response->headerBuffer().bufferSize())  << std::endl;
         }
      }
   }
   else
   {
      //std::cout << "TRYING TO MAKE A CONNECTION" << std::endl;
      result = makeConnectionIfNeeded(client);
      if(result)
      {
         result = false;
         if(client->post_window(&window, m_requestQueueId))
         {
            try 
            {
               //
               while( !client->is_idle() &&client->is_alive())
               {
                  OpenThreads::Thread::microSleep(2000);
               }

               result = true;
            }
            catch(...)
            {
               result = false;
            }
         }
      }
   }
   //std::cout << "RESULT? " << result << std::endl;
  return result;
}

ossim_uint32 ossimKakaduJpipHandler::getNumberOfLines(ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if (resLevel < m_overviewDimensions.size())
   {
      result = m_overviewDimensions[resLevel].height();
   }
   else if (theOverview.valid())
   {
      result = theOverview->getNumberOfLines(resLevel);
   }
   return result;
}

ossim_uint32 ossimKakaduJpipHandler::getNumberOfSamples(ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if (resLevel < m_overviewDimensions.size())
   {
      result = m_overviewDimensions[resLevel].width();
   }
   else if (theOverview.valid())
   {
      result = theOverview->getNumberOfSamples(resLevel);
   }
   return result;
}

ossim_uint32 ossimKakaduJpipHandler::getNumberOfInputBands() const
{
   return m_nInputBands;
}

ossim_uint32 ossimKakaduJpipHandler::getNumberOfOutputBands() const
{
   return m_nOutputBands;
}

ossim_uint32 ossimKakaduJpipHandler::getImageTileWidth() const
{
   return m_tileSize.hasNans()?0:m_tileSize.x;
}

ossim_uint32 ossimKakaduJpipHandler::getImageTileHeight() const
{
   return m_tileSize.hasNans()?0:m_tileSize.y;
}

ossimScalarType ossimKakaduJpipHandler::getOutputScalarType() const
{
   ossimScalarType result = OSSIM_SCALAR_UNKNOWN;
   switch(m_bitDepth)
   {
      case 8:
      {
         result = m_signed?OSSIM_SINT8:OSSIM_UINT8;
         break;
      }
      case 11:
      {
         result = m_signed?OSSIM_SINT16:OSSIM_USHORT11;
         break;
      }
      case 16:
      {
         result = m_signed?OSSIM_SINT16:OSSIM_UINT16;
         break;
      }
   };
   return result;
}

ossim_uint32 ossimKakaduJpipHandler::getNumberOfDecimationLevels() const
{
   ossim_uint32 result = m_overviewDimensions.size();
   if (theOverview.valid())
   {
      result += theOverview->getNumberOfDecimationLevels();
   }
   return result;
}

void ossimKakaduJpipHandler::allocateTile()
{
   m_tile = ossimImageDataFactory::instance()->create(this, this);
   
   if(m_tile.valid())
   {
      m_tile->initialize();
   }
}
#include <ossim/imaging/ossimMemoryImageSource.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/projection/ossimImageViewAffineTransform.h>

ossim_uint64 findLevel(ossim_uint64 power2Value, ossim_uint32 maxLevel)
{
   ossim_uint64 test = 1<<maxLevel;
   ossim_uint64 currentLevel = maxLevel;
   while(currentLevel)
   {
      if(test&power2Value) break;
      test >>=1;
      --currentLevel;
   }
   return currentLevel;
}

ossimRefPtr<ossimImageData> ossimKakaduJpipHandler::getTile(const  ossimIrect& rect,
                                                            ossim_uint32 resLevel)
{
   ossimRefPtr<ossimImageData> result;

   try
   {
      ossim_int32 nLevels = getNumberOfDecimationLevels()-1;
   
      ossim_uint64 scalePower2 = 1<<nLevels;
      ossim_int32 targetLevel = nLevels - ossim::round<ossim_uint32>(log(1.0+((scalePower2*m_quality)/100.0))/log(2.0));
   
      // now try log way
      if(targetLevel > static_cast<ossim_int32>(resLevel))
      {
         double scale = 1.0/(1<<targetLevel);
         ossimDrect scaledRect = rect*ossimDpt(scale, scale);
         ossimIrect scaledRectRound = scaledRect;
         scaledRectRound = ossimIrect(scaledRectRound.ul().x - 1,
                                      scaledRectRound.ul().y - 1,
                                      scaledRectRound.lr().x + 1,
                                      scaledRectRound.lr().y + 1);
         ossimRefPtr<ossimImageData> copy = getTileAtRes(scaledRectRound, targetLevel);
         copy = (ossimImageData*)copy->dup();
    
         //result->setImageRectangle(rect);
      
         ossimRefPtr<ossimMemoryImageSource> memSource = new ossimMemoryImageSource();
         ossimRefPtr<ossimImageRenderer> renderer = new ossimImageRenderer();
         ossimRefPtr<ossimImageViewAffineTransform> transform = new ossimImageViewAffineTransform();
         transform->scale(1<<targetLevel, 1<<targetLevel);
         memSource->setImage(copy.get());
         renderer->connectMyInputTo(memSource.get());
         renderer->getResampler()->setFilterType("bilinear");
         renderer->setImageViewTransform(transform.get());
         result = renderer->getTile(rect);
         renderer->disconnect();
         memSource->disconnect();
         renderer = 0;
         memSource = 0;
      }
      else
      {
         // we need to apply a quality and then scale to the proper resolution.
         // so if we want a 50% quality image and they are requesting a reslevel 0 product
         // we need to query jpip at the next level down and not at 0 and scale the product up
         // using a bilinear or some form of resampling
         //
         result = getTileAtRes(rect, resLevel);
      }
   }
   catch ( kdu_exception exc )
   {
      ostringstream e;
      e << "ossimKakaduNitfReader::~ossimKakaduNitfReader\n"
        << "Caught exception from kdu_region_decompressor: " << exc << "\n";
      ossimNotify(ossimNotifyLevel_WARN) << e.str() << std::endl;
   }
   catch ( std::bad_alloc& )
   {
      std::string e =
         "Caught exception from kdu_region_decompressor: std::bad_alloc";
      ossimNotify(ossimNotifyLevel_WARN) << e << std::endl;
   }
   catch( ... )
   {
      std::string e =
         "Caught unhandled exception from kdu_region_decompressor";
      ossimNotify(ossimNotifyLevel_WARN) << e << std::endl;
   }   

   return result.get();
}

ossimRefPtr<ossimImageData>  ossimKakaduJpipHandler::getTileAtRes(const  ossimIrect& rect,
                                                                  ossim_uint32 resLevel)
{
   ossim_uint32 nLevels = getNumberOfDecimationLevels();
   ossimRefPtr<ossimImageData> result;
   
   if((resLevel>=nLevels)||!isOpen()||!isSourceEnabled())
   {
      return result;
   }
   
   if(resLevel > m_overviewDimensions.size()&&theOverview.valid())
   {
      return theOverview->getTile(rect, resLevel);
   }
   if(!m_tile.valid()) allocateTile();
   m_tile->setImageRectangle(rect);
   
   if(getOverviewTile(resLevel, m_tile.get()))
   {
      return m_tile.get();
   }
   
   ossimIrect bounds = getBoundingRect(resLevel);
   ossimIrect clipRect = bounds.clipToRect(rect);
   
   kdu_dims  dims;
   if(!bounds.intersects(rect))
   {
      return result;
   }
   
   kdu_coords ref_expansion;
   kdu_coords exp_numerator;
   kdu_coords exp_denominator;
   kdu_dims rr_win;
   ref_expansion.x = 1;
   ref_expansion.y = 1;
   
   kdu_dims view_dims;	
   
   ossimString comps;
   ossim_uint32 nBands = getNumberOfInputBands();
   ossim_uint32 idx = 0;
   for(idx=0;idx<nBands;++idx)
   {
      comps+=ossimString::toString(idx);
      if(idx+1!=nBands)
      {
         comps+=",";
      }
   }
   ossimIpt tileSize;
   tileSize = m_rlevelTileSize[resLevel];//m_tileSize;
   // if(static_cast<ossim_int32>(bounds.width()) < tileSize.x) tileSize.x  = bounds.width();
   // if(static_cast<ossim_int32>(bounds.height()) < tileSize.y) tileSize.y = bounds.height();
   
   ossimIrect tileBoundaryRect = clipRect;
   tileBoundaryRect.stretchToTileBoundary(tileSize);
   
   
   tileBoundaryRect.clipToRect(bounds);
   ossim_int64 ulx = tileBoundaryRect.ul().x;
   ossim_int64 uly = tileBoundaryRect.ul().y;
   ossim_int64 maxx = tileBoundaryRect.lr().x;
   ossim_int64 maxy = tileBoundaryRect.lr().y;
   m_tile->makeBlank();
   kdu_window window;
   kdu_dims region;
   for(;ulx < maxx; ulx += tileSize.x)
   {
      for(;uly < maxy; uly += tileSize.y)
      {
         ossim_int64 x = ulx;
         ossim_int64 y = uly;
         ossim_int64 w = tileSize.x;
         ossim_int64 h = tileSize.y;
         
         if(x+w-1 > maxx) w = maxx-x;
         if(y+h-1 > maxy) h = maxy-y;
         
         ossimRefPtr<ossimImageData> cacheTile =ossimAppFixedTileCache::instance()->getTile(m_rlevelBlockCache[resLevel],ossimIpt(x,y));
         if(!cacheTile.valid())
         {
            double percent = (m_quality/100.0);
            window.init();
            region.pos.x = x;
            region.pos.y = y;
            region.size.x = w;
            region.size.y = h;
            //std::cout << "NLAYERS ================= " << m_nQualityLayers << std::endl;
            //ossim_int32 nQualityLayers = ((m_quality/100.0)*m_nQualityLayers);
            ossim_int32 nQualityLayers = (m_nQualityLayers);//*percent;
            nQualityLayers = nQualityLayers<1?1:nQualityLayers;
            // std::cout << "nquality === " << nQualityLayers << std::endl;
            window.set_region(region);
            
            window.set_max_layers(nQualityLayers);
            window.resolution.x = bounds.width();
            window.resolution.y = bounds.height();
            
            try
            {
               if(loadClient(m_client, window))
               {
                  //std::cout << "WE LOADED THE CLIENT!!!!" << std::endl;
                  bool gotRegion = true;
                  // if we do not need to use ssl we use kakadu for the window loads
                  if(!m_useOurGrab)
                  {
                     kdu_window reqWindow;
                     m_client->get_window_in_progress(&reqWindow);
                     gotRegion = ((reqWindow.region.size.x>0)&&
                                  (reqWindow.region.size.y>0));
                  }
                  //kdu_window reqWindow = window;
                  //m_client->get_window_in_progress(&reqWindow);
                  // now get a codestream ready for decompressing the data
                  //
                  
                  if(gotRegion)
                  {
                     jp2_family_src jp2FamilySource;
                     jp2FamilySource.open(m_client);
                     
                     jpx_source jpxSource;
                     if(jpxSource.open(&jp2FamilySource, true)==1)
                     {
                        jpx_codestream_source jpx_codestream = jpxSource.access_codestream(0);
                        kdu_codestream codestream;
                        jpx_input_box* inputBox = jpx_codestream.open_stream();
                        if(inputBox)
                        {
                           codestream.create(inputBox);
                           codestream.set_persistent();
                           codestream.apply_input_restrictions(
                              0, 0, 
                              resLevel, nQualityLayers, &region, 
                              KDU_WANT_CODESTREAM_COMPONENTS);
                           if ( ossim::clipRegionToImage(codestream,
                                                         region,
                                                         resLevel,
                                                         region) )
                           {
                              kdu_channel_mapping channels;
                              channels.configure(codestream);
                              cacheTile = ossimImageDataFactory::instance()->create(this, this);
                              cacheTile->setImageRectangle(ossimIrect(region.pos.x,region.pos.y,
                                                                      region.pos.x + region.size.x - 1, 
                                                                      region.pos.y+region.size.y - 1));
                              cacheTile->initialize();
                              
                              ossim::copyRegionToTile(&channels, codestream, resLevel, 0, 0, cacheTile.get());
                              ossimAppFixedTileCache::instance()->addTile(m_rlevelBlockCache[resLevel], cacheTile.get(), false);
                           }
                        }
                     }
                     jpxSource.close();
                     jp2FamilySource.close();
                  }
                  else
                  {
                     ossimNotify(ossimNotifyLevel_WARN) << "Unable to get block from server, defaulting to blank\n";
                  }
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN) << "Unable to establish a connection to the server\n";
               }
            }
            catch(...)
            {
               cacheTile = 0;
            }
         }
         if(cacheTile.valid())
         {
            m_tile->loadTile(cacheTile->getBuf(), cacheTile->getImageRectangle(), OSSIM_BSQ);
         }
      }
   }
   m_tile->validate();
   
   return m_tile;
}


bool ossimKakaduJpipHandler::makeConnectionIfNeeded(kdu_client* client)
{
   if(!client->is_alive())
   {           
      ossimString server = m_baseUrl.getIp()+(m_baseUrl.getPort().empty()?"":":"+m_baseUrl.getPort());
      ossimString path = m_baseUrl.getPath();
       
      m_requestQueueId = client->connect(server.c_str(), "",path.c_str(), "http", "", KDU_CLIENT_MODE_INTERACTIVE, "");   
   }
   return client->is_alive();
   
}

void ossimKakaduJpipHandler::deleteRlevelCache()
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < m_rlevelBlockCache.size();++idx)
   {
      ossimAppFixedTileCache::instance()->deleteCache(m_rlevelBlockCache[idx]);
   }
   m_rlevelBlockCache.clear();
}

void ossimKakaduJpipHandler::performRlevelSetup(kdu_codestream& codestream)
{
   deleteRlevelCache();
   m_rlevelTileSize.clear();
   
   //Get resolution level dimensions
   //
   ossim_uint32 nDiscard = 0;
   for(nDiscard = 0; nDiscard < m_resLevels; ++nDiscard)
   {
      kdu_dims  dims;
      codestream.apply_input_restrictions( 0, 0, nDiscard, 0, NULL );
      codestream.get_dims( 0, dims );
      ossimIrect rect(0,0,dims.size.x-1, dims.size.y-1);
      m_overviewDimensions.push_back(rect);
      ossimIpt tileSize(m_tileSize);
      tileSize.x = ((tileSize.x > static_cast<ossim_int32>(rect.width()))?static_cast<ossim_int32>(rect.width()):tileSize.x);
      tileSize.y = ((tileSize.y > static_cast<ossim_int32>(rect.height()))?static_cast<ossim_int32>(rect.height()):tileSize.y);
      m_rlevelTileSize.push_back(tileSize);
      rect.stretchToTileBoundary(tileSize);
      m_rlevelBlockCache.push_back(ossimAppFixedTileCache::instance()->newTileCache(rect, tileSize)); 
   }
}

void ossimKakaduJpipHandler::initializeRlevelCache()
{
   deleteRlevelCache();
   m_rlevelTileSize.clear();
   ossim_uint32 idx = 0;
   if(m_rlevelTileSize.size() != m_overviewDimensions.size()) return;
   for(idx = 0; idx < m_rlevelTileSize.size(); ++idx)
   {
      ossimIrect rect = m_overviewDimensions[idx];
      ossimIpt tileSize(m_rlevelTileSize[idx]);
      rect.stretchToTileBoundary(tileSize);
      m_rlevelBlockCache.push_back(ossimAppFixedTileCache::instance()->newTileCache(rect, tileSize)); 
   }
}

void ossimKakaduJpipHandler::setProperty(ossimRefPtr<ossimProperty> property)
{
   bool refreshCache = false;
   if(property.valid())
   {
      ossimString name = property->getName();
      if(name == "quality")
      {
         refreshCache = !ossim::almostEqual((double)m_quality, 
                                           property->valueToString().toDouble());
         m_quality = property->valueToString().toDouble();
      }
      else
      {
         ossimImageHandler::setProperty(property.get());
      }
   }
   
   if(refreshCache)
   {
      flushCache();
      if(m_client) m_client->close();
   }
}

ossimRefPtr<ossimProperty> ossimKakaduJpipHandler::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> result;
   if(name == "quality")
   {
      result = new ossimNumericProperty(name, ossimString::toString(m_quality),
                                        0, 100);
      result->setCacheRefreshBit();
      result->setFullRefreshBit();
   }
   
   return result.get();
}

void ossimKakaduJpipHandler::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   ossimImageHandler::getPropertyNames(propertyNames);
   propertyNames.push_back(ossimKeywordNames::QUALITY_KW);
}

 ossimRefPtr<ossimImageGeometry> ossimKakaduJpipHandler::getImageGeometry()
{
   if(theGeometry.valid()) return theGeometry.get();
   
   // if base is able to create a goemetry then return it
   theGeometry = ossimImageHandler::getImageGeometry();
   
   if(!theGeometry->getProjection())
   {
      ossimKeywordlist kwl;
      
      ossimRefPtr<ossimKakaduJpipInfo> info = new ossimKakaduJpipInfo();
      info->setHandler(this);
      info->getKeywordlist(kwl);
      info = 0;
      ossimString prefix = "jpip.image"+ossimString::toString(getCurrentEntry()) + ".";
      // dump the infromation into a kwl and then pass it to the geometry resgistry
      
      //std::cout << kwl << std::endl;
      //
      theGeometry = ossimImageGeometryRegistry::instance()->createGeometry(kwl, prefix.c_str());
      if(theGeometry.valid())
      {
         initImageParameters(theGeometry.get());
      }
   }
   
   return theGeometry.get();
}

bool ossimKakaduJpipHandler::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   close();
   bool result = ossimImageHandler::loadState(kwl, prefix);
   if(result)
   {
      m_baseUrl = ossimUrl(kwl.find(prefix, "url"));
      ossimString qualityString = kwl.find(prefix,ossimKeywordNames::QUALITY_KW);
      if(!qualityString.empty())
      {
         m_quality = qualityString.toDouble();
      }
      result = openStream();
   }

   
   if(!result)
   {
      close();
   }
   return result;
}

bool ossimKakaduJpipHandler::saveState(ossimKeywordlist& kwl, const char* prefix)const
{
   bool result = ossimImageHandler::saveState(kwl, prefix);
   
   ossimUrl url = m_baseUrl;
   ossimString protocol = m_baseUrl.getProtocol();
   if(protocol == "http") protocol = "jpip";
   else if(protocol == "https") protocol = "jpips";
   url.setProtocol(protocol);
   
   kwl.add(prefix, "url", url.toString().c_str(), true);
   kwl.add(prefix, ossimKeywordNames::QUALITY_KW, m_quality, true);
   
   return result;
}


void ossimKakaduJpipHandler::flushCache()
{
   ossim_uint32 idx = 0;
   for(idx = 0; idx < m_rlevelBlockCache.size();++idx)
   {
      ossimAppFixedTileCache::instance()->flush(m_rlevelBlockCache[idx]);
   }
}



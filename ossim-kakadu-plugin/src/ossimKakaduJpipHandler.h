//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author: Garrett Potts
//
// Description: Factory for OSSIM JPIP reader using kakadu library.
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimJpipHandler_HEADER
#define ossimJpipHandler_HEADER
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimUrl.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>
#include <ossim/support_data/ossimJpipMessageDecoder.h>
#include <ossim/base/ossimHttpResponse.h>
#include <ossim/base/ossimHttpRequest.h>

#include <kdu_cache.h>
#include <kdu_region_decompressor.h>
#include <kdu_client.h>
#include <kdu_compressed.h>


class  ossimKakaduJpipHandler : public ossimImageHandler
{
public:
   typedef std::vector<kdu_core::kdu_byte> ByteBuffer;
   struct Box
   {
      ossim_uint32 m_type;
      ByteBuffer m_buffer;
   };
   typedef std::vector<Box> BoxList;
   typedef std::vector<ossimIrect> RectList;
   typedef std::vector<ossimString> StringList;
   typedef std::vector<ossimIpt> RLevelBlockSizeList;
   typedef std::vector<ossimAppFixedTileCache::ossimAppFixedCacheId> RLevelCacheList;
   
   ossimKakaduJpipHandler();
   virtual ~ossimKakaduJpipHandler();
   virtual void close();
   virtual bool open();
   virtual bool isOpen()const;
   virtual ossimRefPtr<ossimImageData> getTile(const  ossimIrect& rect,
                                               ossim_uint32 resLevel=0);
   
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 resLevel = 0) const;
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 resLevel = 0) const;
   virtual ossim_uint32 getImageTileWidth() const;
   virtual ossim_uint32 getImageTileHeight() const;
   
   virtual ossim_uint32 getNumberOfInputBands() const;
   virtual ossim_uint32 getNumberOfOutputBands() const;
   virtual ossim_uint32 getNumberOfDecimationLevels() const;
   virtual bool isImageTiled() const
   {
      return !m_tileSize.hasNans();
   }
   virtual ossimScalarType getOutputScalarType() const;
   virtual void setProperty(ossimRefPtr<ossimProperty> property);
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;
   virtual void extractBoxes(BoxList& boxList);
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();
   //virtual void extractXml(ossimString& xmlBoxes);
   virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix = 0);
   virtual bool saveState(ossimKeywordlist& kwl, const char* prefix = 0)const;
   
protected:
   ossimRefPtr<ossimImageData> getTileAtRes(const  ossimIrect& rect,
                                            ossim_uint32 resLevel=0);
   //void extractXmlRecurse(ossimString& xmlBoxes, jp2_input_box* pParentBox=0);
   virtual void extractBoxes(BoxList& boxList, kdu_supp::jp2_input_box* pParentBox);
   void showBoxes(kdu_supp::jp2_input_box* pParentBox=0);
   bool makeConnectionIfNeeded(kdu_supp::kdu_client* client);
   //virtual bool initializeImplementation();
   int convertClassIdToKdu(int id);
   void allocateTile();
   void deleteRlevelCache();
   void performRlevelSetup(kdu_core::kdu_codestream& codestream);   
   bool loadClient(kdu_supp::kdu_client* client, kdu_supp::kdu_window& window);
   bool openStream();
   void flushCache();
   void initializeRlevelCache();
   void allocateSession();
   
   bool            m_useOurGrab;
   ossimUrl        m_baseUrl;
   RectList        m_overviewDimensions;
   ossim_uint32    m_nInputBands;
   ossim_uint32    m_nOutputBands;
   bool            m_signed;
   ossim_uint16    m_bitDepth;
   ossim_uint32    m_nQualityLayers;
   ossimIpt        m_tileSize;
   ossimIrect      m_imageBounds;
   ossim_uint32    m_resLevels;
   int             m_bYCC;
   ossimRefPtr<ossimImageData> m_tile;
   
   ossim_float32   m_quality;
   kdu_supp::kdu_client*     m_headerClient;
   kdu_supp::kdu_client*     m_client;
   kdu_supp::jp2_family_src  m_jp2Family;
   int             m_requestQueueId;
   RLevelBlockSizeList m_rlevelTileSize;
   RLevelCacheList m_rlevelBlockCache;

   ossimString     m_cid;
   ossimString     m_path;
   ossimString     m_transport;
   ossimRefPtr<ossimHttpRequest> m_request;
   TYPE_DATA;    
};
#endif

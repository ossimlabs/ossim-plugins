#include "CurlHeaderCache.h"
#include "CurlStreamDefaults.h"

std::shared_ptr<ossim::CurlHeaderCache> ossim::CurlHeaderCache::m_instance;


ossim::CurlHeaderCache::CurlHeaderCache()
:m_maxCacheEntries(ossim::CurlStreamDefaults::m_nReadCacheHeaders)
{

}

ossim::CurlHeaderCache::~CurlHeaderCache()
{
   m_cache.clear();
}

std::shared_ptr<ossim::CurlHeaderCache> ossim::CurlHeaderCache::instance()
{
   if(!m_instance)
   {
      m_instance = std::make_shared<CurlHeaderCache>();
   }

   return m_instance;
}

bool ossim::CurlHeaderCache::getCachedFilesize(const Key_t& key, ossim_int64& filesize)const
{
   std::unique_lock<std::mutex> lock(m_mutex);
   bool result = false;
   if(m_maxCacheEntries<=0) return result;
   CacheType::const_iterator iter = m_cache.find(key);

   if(iter != m_cache.end())
   {
      filesize = iter->second->m_filesize;
      result = true;
   }
   if(result)
   {
      ossim::CurlHeaderCache* curlHeaderConstPtr = const_cast<ossim::CurlHeaderCache*>(this);
      curlHeaderConstPtr->touchEntryProtected(key);
   }

   return result;
}

void ossim::CurlHeaderCache::addHeader(const Key_t& key, Node_t& node)
{
   std::unique_lock<std::mutex> lock(m_mutex);
   if(m_maxCacheEntries<=0) return;
   CacheType::const_iterator iter = m_cache.find(key);
   if(iter != m_cache.end())
   {
      iter->second->m_filesize = node->m_filesize;
      touchEntryProtected(key);
   }  
   else
   {
      if(m_cache.size() >= m_maxCacheEntries)
      {
         shrinkEntries();
      }
      node->m_timestamp = ossimTimer::instance()->tick();
      m_cache.insert( std::make_pair(key, node) );
      m_cacheTime.insert( std::make_pair(node->m_timestamp, key) );
   } 
}

void ossim::CurlHeaderCache::touchEntry(const Key_t& key)
{
   // I think unique lock is recursive but need to test first
   // So for now we will wrap with a touchEntryProtected that does not lock
   // so all public methods are locking.
   //
   std::unique_lock<std::mutex> lock(m_mutex);
   touchEntryProtected(key);
}

void ossim::CurlHeaderCache::touchEntryProtected(const Key_t& key)
{
   CacheType::iterator iter = m_cache.find(key);
   if(iter!=m_cache.end())
   {
      CacheTimeIndexType::iterator cacheTimeIter = m_cacheTime.find(iter->second->m_timestamp);
      if(cacheTimeIter!=m_cacheTime.end())
      {
         m_cacheTime.erase(cacheTimeIter);
         iter->second->m_timestamp = ossimTimer::instance()->tick();
         m_cacheTime.insert( std::make_pair(iter->second->m_timestamp, key) );
      }
   }
}

void ossim::CurlHeaderCache::shrinkEntries()
{
   ossim_int64 targetSize = static_cast<ossim_int64>(0.2*m_maxCacheEntries);

   if(m_cacheTime.empty()) return;
   if(targetSize >= m_cache.size())
   {
      m_cache.clear();
      m_cacheTime.clear();
      return;
   }

   CacheTimeIndexType::iterator iter = m_cacheTime.begin();

   while(targetSize > 0)
   {
      m_cache.erase(iter->second);
      iter = m_cacheTime.erase(iter);
      --targetSize;
   }
}


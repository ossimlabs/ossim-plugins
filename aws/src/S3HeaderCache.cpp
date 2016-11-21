#include "S3HeaderCache.h"
#include "S3StreamDefaults.h"

std::shared_ptr<ossim::S3HeaderCache> ossim::S3HeaderCache::m_instance;


ossim::S3HeaderCache::S3HeaderCache()
:m_maxCacheEntries(ossim::S3StreamDefaults::m_nReadCacheHeaders)
{

}

ossim::S3HeaderCache::~S3HeaderCache()
{
   m_cache.clear();
}

std::shared_ptr<ossim::S3HeaderCache> ossim::S3HeaderCache::instance()
{
   if(!m_instance)
   {
      m_instance = std::make_shared<S3HeaderCache>();
   }

   return m_instance;
}

bool ossim::S3HeaderCache::getCachedFilesize(const Key_t& key, ossim_int64& filesize)const
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

   return result;
}

void ossim::S3HeaderCache::addHeader(const Key_t& key, Node_t& node)
{
   std::unique_lock<std::mutex> lock(m_mutex);
   if(m_maxCacheEntries<=0) return;
   CacheType::const_iterator iter = m_cache.find(key);

   if(iter != m_cache.end())
   {
      iter->second->m_filesize = node->m_filesize;
   }  
   else
   {
      if(m_cache.size() >= m_maxCacheEntries)
      {
         shrinkEntries();
      }
      m_cache.insert( std::make_pair(key, node) );
      m_cacheTime.insert( std::make_pair(node->m_timestamp, key) );
   } 
}

void ossim::S3HeaderCache::shrinkEntries()
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


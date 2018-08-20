#include "ossimKakaduJ2kCodec.h"

ossimKakaduJ2kCodec::ossimKakaduJ2kCodec()
    : m_extension("jp2"),
      m_codecType("j2k")
{

}

ossimString ossimKakaduJ2kCodec::getCodecType() const
{
   return m_codecType;
}

bool ossimKakaduJ2kCodec::encode(const ossimRefPtr<ossimImageData> &in,
                                 std::vector<ossim_uint8> &out) const
{
   bool result = false;

   return false;
}

bool ossimKakaduJ2kCodec::decode(const std::vector<ossim_uint8> &in,
                                 ossimRefPtr<ossimImageData> &out) const
{
   bool result = false;

   return false;
}

const std::string &ossimKakaduJ2kCodec::getExtension() const
{
   return m_extension;
}

bool ossimKakaduJ2kCodec::loadState(const ossimKeywordlist &kwl, const char *prefix)
{
   std::cout << "LOADING AND INITIALIZING THE DECOMPRESSOR AND COMPRESSORS\n";

   return true;
}

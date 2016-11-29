#include "ossimKakaduJpipInfoFactory.h"
#include "ossimKakaduJpipInfo.h"
#include <ossim/base/ossimRefPtr.h>
#include <iostream>
#include <memory>

ossimKakaduJpipInfoFactory::~ossimKakaduJpipInfoFactory()
{
}

ossimKakaduJpipInfoFactory* ossimKakaduJpipInfoFactory::instance()
{
   static ossimKakaduJpipInfoFactory inst;
   return &inst;
}

std::shared_ptr<ossimInfoBase> ossimKakaduJpipInfoFactory::create(const ossimFilename& file) const
{

   std::shared_ptr<ossimInfoBase> jpipInfo = std::make_shared<ossimKakaduJpipInfo>();
   if(!jpipInfo->open(file))
   {
      jpipInfo.reset();
   }
   return jpipInfo;
}

std::shared_ptr<ossimInfoBase> ossimKakaduJpipInfoFactory::create(std::shared_ptr<ossim::istream>& str,
                                                                  const std::string& connectionString)const
{
   std::shared_ptr<ossimInfoBase> jpipInfo;

   return jpipInfo;
}
/** hidden from use default constructor */
ossimKakaduJpipInfoFactory::ossimKakaduJpipInfoFactory()
{
}

/** hidden from use copy constructor */
ossimKakaduJpipInfoFactory::ossimKakaduJpipInfoFactory(const ossimKakaduJpipInfoFactory& )
{
   
}

/** hidden from use operator = */
const ossimKakaduJpipInfoFactory& ossimKakaduJpipInfoFactory::operator=(const ossimKakaduJpipInfoFactory& )
{
   return *this;
}

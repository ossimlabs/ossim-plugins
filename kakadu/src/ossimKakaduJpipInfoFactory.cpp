#include "ossimKakaduJpipInfoFactory.h"
#include "ossimKakaduJpipInfo.h"
#include <ossim/base/ossimRefPtr.h>
#include <iostream>

ossimKakaduJpipInfoFactory::~ossimKakaduJpipInfoFactory()
{
}

ossimKakaduJpipInfoFactory* ossimKakaduJpipInfoFactory::instance()
{
   static ossimKakaduJpipInfoFactory inst;
   return &inst;
}

ossimInfoBase* ossimKakaduJpipInfoFactory::create(const ossimFilename& file) const
{

   ossimRefPtr<ossimKakaduJpipInfo> jpipInfo = new ossimKakaduJpipInfo();
   if(!jpipInfo->open(file))
   {
      jpipInfo = 0;
   }
   return jpipInfo.release();
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

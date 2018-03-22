//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description:  Factory class definition for codec(encoder/decoder).
// 
//----------------------------------------------------------------------------
// $Id$

#include "ossimKakaduCodecFactory.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>

#include <ossim/imaging/ossimJpegCodec.h>

#include <string>

ossimKakaduCodecFactory* ossimKakaduCodecFactory::theInstance = 0;

static const std::string TYPE_KW    = "type";

ossimKakaduCodecFactory::~ossimKakaduCodecFactory()
{}

ossimKakaduCodecFactory* ossimKakaduCodecFactory::instance()
{
   if ( !theInstance )
   {
      theInstance = new ossimKakaduCodecFactory();
   }
   return theInstance;
}
ossimCodecBase* ossimKakaduCodecFactory::createCodec(const ossimString& type)const
{
   ossimRefPtr<ossimCodecBase> result;

   if((type.downcase() == "c8") )
   {
      std::cout << "ossimKakaduCodecFactory::createCodec: WE DO NEED A ossimKakaduCodec allocation here!\n";
   }

   return result.release();
}

ossimCodecBase* ossimKakaduCodecFactory::createCodec(const ossimKeywordlist& kwl, const char* prefix)const
{
   ossimString type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   ossimCodecBase* result = 0;
   if(!type.empty())
   {
      result = this->createCodec(type);
      if(result)
      {
         result->loadState(kwl, prefix);
      }
   }

   return result;
}

void ossimKakaduCodecFactory::getTypeNameList(std::vector<ossimString>& typeNames)const
{
   typeNames.push_back("C8");
}

ossimKakaduCodecFactory::ossimKakaduCodecFactory()
{}

ossimKakaduCodecFactory::ossimKakaduCodecFactory(const ossimKakaduCodecFactory& /* obj */ )
{}

const ossimKakaduCodecFactory& ossimKakaduCodecFactory::operator=(
   const ossimKakaduCodecFactory& /* rhs */)
{
   return *this;
}

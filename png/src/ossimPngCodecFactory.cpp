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

#include "ossimPngCodecFactory.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageData.h>

#include "ossimPngCodec.h"

#include <string>

ossimPngCodecFactory* ossimPngCodecFactory::m_instance = 0;

static const std::string TYPE_KW    = "type";

ossimPngCodecFactory::~ossimPngCodecFactory()
{}

ossimPngCodecFactory* ossimPngCodecFactory::instance()
{
   if ( !m_instance )
   {
      m_instance = new ossimPngCodecFactory();
   }
   return m_instance;
}
ossimCodecBase* ossimPngCodecFactory::createCodec(const ossimString& type)const
{
   ossimRefPtr<ossimPngCodec> result;
   ossimString tempType = type.downcase();
   if(tempType == "png") 
   {
      result = new ossimPngCodec();
   }
   else if(tempType == "pnga")
   {
      result = new ossimPngCodec(true);
   }
   else if(tempType == STATIC_TYPE_NAME(ossimPngCodec))
   {
      result = new ossimPngCodec();
   }

   return result.release();
}

ossimCodecBase* ossimPngCodecFactory::createCodec(const ossimKeywordlist& kwl, const char* prefix)const
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

void ossimPngCodecFactory::getTypeNameList(std::vector<ossimString>& typeNames)const
{
   typeNames.push_back("png");
   typeNames.push_back("pnga");
   typeNames.push_back(STATIC_TYPE_NAME(ossimPngCodec));
}

ossimPngCodecFactory::ossimPngCodecFactory()
{}

ossimPngCodecFactory::ossimPngCodecFactory(const ossimPngCodecFactory& /* obj */ )
{}

const ossimPngCodecFactory& ossimPngCodecFactory::operator=(
   const ossimPngCodecFactory& /* rhs */)
{
   return *this;
}

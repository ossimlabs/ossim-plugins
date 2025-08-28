//---
//
// License: MIT
//
// Author: David Burken
//
// Description: Description: Singleton class to hold a common
// kdu_core::membroker.
// 
//---
// $Id$

#include "ossimKakaduMembroker.h"
#include <kdu_compressed.h> /* class kdu_membroker */

ossimKakaduMembroker* ossimKakaduMembroker::m_instance = 0;

ossimKakaduMembroker::~ossimKakaduMembroker()
{
   if ( m_membroker )
   {
      delete m_membroker;
      m_membroker = 0;
   }
}

ossimKakaduMembroker* ossimKakaduMembroker::instance()
{
   if ( !m_instance )
   {
      m_instance = new ossimKakaduMembroker();
   }
   return m_instance;
}

kdu_core::kdu_membroker*  ossimKakaduMembroker::getMembroker() const
{
   return m_membroker;
}

ossimKakaduMembroker::ossimKakaduMembroker()
   : m_membroker( new kdu_core::kdu_membroker() ) // No limit
{}

ossimKakaduMembroker::ossimKakaduMembroker(const ossimKakaduMembroker& /* obj */ )
{}

const ossimKakaduMembroker& ossimKakaduMembroker::operator=(
   const ossimKakaduMembroker& /* rhs */)
{
   return *this;
}

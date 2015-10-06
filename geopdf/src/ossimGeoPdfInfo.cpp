//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Ogr Info object.
// 
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfInfo.cpp 20935 2012-05-18 14:19:30Z dburken $
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring> // for memset

//PoDoFo includes
#include <podofo/doc/PdfMemDocument.h>
#include <podofo/base/PdfString.h>

//ossim includes
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordlist.h>

#include "ossimGeoPdfInfo.h"

// Static trace for debugging
static ossimTrace traceDebug("ossimGeoPdfInfo:debug");
static ossimTrace traceDump("ossimGeoPdfInfo:dump"); // This will dump offsets.

ossimGeoPdfInfo::ossimGeoPdfInfo()
   : ossimInfoBase(),
     theFile(),
     m_PdfMemDocument(NULL)
{
}

ossimGeoPdfInfo::~ossimGeoPdfInfo()
{
  if (m_PdfMemDocument != NULL)
  {
    delete m_PdfMemDocument;
    m_PdfMemDocument = 0;
  }
}

bool ossimGeoPdfInfo::open(const ossimFilename& file)
{
   theFile = file;
   if (isOpen())
   {
     PoDoFo::PdfError::EnableDebug(false); //do not print out debug info
     m_PdfMemDocument = new PoDoFo::PdfMemDocument(theFile.c_str());

     if (m_PdfMemDocument == NULL)
     {
       return false;
     }
     return true;
   }
   return false;
}

bool ossimGeoPdfInfo::isOpen()
{
  ossimString ext = theFile.ext().downcase();

  if(ext == "pdf")
  {
    return true;
  }
  else
  {
    return false;
  }
}

std::ostream& ossimGeoPdfInfo::print(std::ostream& out) const
{
  static const char MODULE[] = "ossimGeoPdfInfo::print";
 
  int count = m_PdfMemDocument->GetPageCount();
  PoDoFo::PdfString author = m_PdfMemDocument->GetInfo()->GetAuthor();
  PoDoFo::PdfString creator = m_PdfMemDocument->GetInfo()->GetCreator();
  PoDoFo::PdfString title = m_PdfMemDocument->GetInfo()->GetTitle();
  PoDoFo::PdfString subject = m_PdfMemDocument->GetInfo()->GetSubject();
  PoDoFo::PdfString keywords = m_PdfMemDocument->GetInfo()->GetKeywords();
  PoDoFo::PdfString producer = m_PdfMemDocument->GetInfo()->GetProducer();


  ossimString createDate;
  ossimString modifyDate;
  PoDoFo::PdfObject* obj = m_PdfMemDocument->GetInfo()->GetObject();
  if (obj->IsDictionary())
  {
    PoDoFo::PdfDictionary pdfDictionary = obj->GetDictionary();

    PoDoFo::TKeyMap keyMap = pdfDictionary.GetKeys();
    PoDoFo::TKeyMap::iterator it = keyMap.begin();
    while (it != keyMap.end())
    {
      ossimString refName = ossimString(it->first.GetName());
      PoDoFo::PdfObject* refObj = it->second;

      std::string objStr;
      refObj->ToString(objStr);
      
      if (refName == "CreationDate")
      {
        createDate = ossimString(objStr);
        createDate = createDate.substitute("(", "", true).trim();
        createDate = createDate.substitute(")", "", true).trim();
        createDate = createDate.substitute("D:", "", true).trim();
      }
      else if (refName == "ModDate")
      {
        modifyDate = ossimString(objStr);
        modifyDate = modifyDate.substitute("(", "", true).trim();
        modifyDate = modifyDate.substitute(")", "", true).trim();
        modifyDate = modifyDate.substitute("D:", "", true).trim();
      }
      it++;
    }
  }

  try
  {
     m_PdfMemDocument->FreeObjectMemory(obj);
  }
  catch (...)
  {  
  }

  ossimString authorStr = author.GetString();
  ossimString creatorStr = creator.GetString();
  ossimString titleStr = title.GetString();
  ossimString producerStr = producer.GetString();
  ossimString subjectStr = subject.GetString();
  ossimString keywordsStr = keywords.GetString();

  ossimString prefix = "geopdf.";

  out << prefix << "pagecount:  "
    << ossimString::toString(count).c_str() << "\n";

  if (!authorStr.empty())
  {
    out << prefix << "author:  "
      << authorStr.c_str() << "\n";
  }

  if (!creatorStr.empty())
  {
    out << prefix << "creator:  "
      << creatorStr.c_str() << "\n";
  }

  if (!titleStr.empty())
  {
    out << prefix << "title:  "
      << titleStr.c_str() << "\n";
  }

  if (!producerStr.empty())
  {
    out << prefix << "producer:  "
      << producerStr.c_str() << "\n";
  }

  if (!subjectStr.empty())
  {
    out << prefix << "subject:  "
      << subjectStr.c_str() << "\n";
  }

  if (!keywordsStr.empty())
  {
    out << prefix << "keywords:  "
      << keywordsStr.c_str() << "\n";
  }

  if (!createDate.empty())
  {
    out << prefix << "creationdate:  "
      << createDate.c_str() << "\n";
  }

  if (!modifyDate.empty())
  {
    out << prefix << "modificationdate:  "
      << modifyDate.c_str() << "\n";
  }

  if (traceDebug())
  {    
     ossimNotify(ossimNotifyLevel_DEBUG)
      << MODULE << " DEBUG Entered...\n";
  }
  return out;
}

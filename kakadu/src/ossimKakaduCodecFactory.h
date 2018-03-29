//----------------------------------------------------------------------------
//
// License:  MIT
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
// Description: Factory class declaration for codec(encoder/decoder).
// 
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimKakaduCodecFactory_HEADER
#define ossimKakaduCodecFactory_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimCodecFactoryInterface.h>

class ossimFilename;
class ossimCodecBase;

/**
 * @brief Codec factory.
 */
class OSSIM_DLL ossimKakaduCodecFactory : public ossimCodecFactoryInterface
{
public:

   /** virtual destructor */
   virtual ~ossimKakaduCodecFactory();

   /**
   * @return instance
   */
   static ossimKakaduCodecFactory* instance();

   /**
   * createCodec takes a type and will return a new codec to encode decode image buffers
   *
   * @param in type.  Type identifer used to allocate the proper codec.
   * @return ossimCodecBase type.
   */
   virtual ossimCodecBase* createCodec(const ossimString& type)const;


   /**
   * createCodec takes a type in the keywordlist and will return a new codec to encode decode image buffers
   *
   * @param in kwl.  Type identifer used to allocate the proper codec.
   * @param in prefix.  prefix used to prefix keywords during the construction
   *                    of the codec
   * @return ossimCodecBase type.
   */
   virtual ossimCodecBase* createCodec(const ossimKeywordlist& kwl, const char* prefix=0)const;

   virtual void getTypeNameList(std::vector<ossimString>& typeNames)const;
   
private:
   
   /** hidden from use default constructor */
   ossimKakaduCodecFactory();

   /** hidden from use copy constructor */
   ossimKakaduCodecFactory(const ossimKakaduCodecFactory& obj);

   /** hidden from use operator = */
   const ossimKakaduCodecFactory& operator=(const ossimKakaduCodecFactory& rhs);

   /** The single instance of this class. */
   static ossimKakaduCodecFactory* theInstance;
};

#endif /* End of "#ifndef ossimKakaduCodecFactory_HEADER" */

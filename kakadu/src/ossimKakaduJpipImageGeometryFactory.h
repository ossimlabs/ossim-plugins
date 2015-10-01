#ifndef ossimKakaduJpipImageGeometryFactory_HEADER
#define ossimKakaduJpipImageGeometryFactory_HEADER 1
#include <ossim/imaging/ossimImageGeometryFactoryBase.h>

class ossimImageHandler;
class ossimKakaduJpipImageGeometryFactory : public ossimImageGeometryFactoryBase
{
public:
   static ossimKakaduJpipImageGeometryFactory* instance();
   virtual ossimImageGeometry* createGeometry(const ossimString& typeName)const;
   virtual ossimImageGeometry* createGeometry(const ossimKeywordlist& kwl,
                                              const char* prefix=0)const;
   virtual ossimImageGeometry* createGeometry(const ossimFilename& filename,
                                              ossim_uint32 entryIdx)const;
   virtual bool extendGeometry(ossimImageHandler* handler)const;
   
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
   /**
    * This is a utility method used by crateGeoemtry that takes an image handler
    */
   virtual ossim2dTo2dTransform* createTransform(ossimImageHandler* handler)const;
   
   
   /**
    * This is a utility method used by crateGeoemtry that takes keywordlist and prefix
    */
   virtual ossim2dTo2dTransform* createTransform(const ossimKeywordlist& kwl, 
                                                 const char* prefix=0)const;
   
   /**
    * @brief Utility method to create a projection from an image handler.
    * @param handler The image handler to create projection from.
    * @return Pointer to an ossimProjection on success, null on error.
    */
   virtual ossimProjection* createProjection(ossimImageHandler* handler) const;
   
protected:
   ossimKakaduJpipImageGeometryFactory();

};

#endif

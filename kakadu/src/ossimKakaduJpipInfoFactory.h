#ifndef ossimKakaduJpipInfoFactory_HEADER
#define ossimKakaduJpipInfoFactory_HEADER 1
#include <ossim/support_data/ossimInfoFactoryInterface.h>

class ossimKakaduJpipInfoFactory : public ossimInfoFactoryInterface
{
public:
   
   /** virtual destructor */
   virtual ~ossimKakaduJpipInfoFactory();
   
   static ossimKakaduJpipInfoFactory* instance();
   
   /**
    * @brief create method.
    *
    * @param file Some file you want info for.
    *
    * @return ossimInfoBase* on success 0 on failure.  Caller is responsible
    * for memory.
    */
   virtual std::shared_ptr<ossimInfoBase> create(const ossimFilename& file) const;
   virtual std::shared_ptr<ossimInfoBase> create(std::shared_ptr<ossim::istream>& str,
                                                 const std::string& connectionString) const;
   
private:
   
   /** hidden from use default constructor */
   ossimKakaduJpipInfoFactory();
   
   /** hidden from use copy constructor */
   ossimKakaduJpipInfoFactory(const ossimKakaduJpipInfoFactory& obj);
   
   /** hidden from use operator = */
   const ossimKakaduJpipInfoFactory& operator=(const ossimKakaduJpipInfoFactory& rhs);

};

#endif

#ifndef ossimKakaduJ2kCodec_HEADER
#define ossimKakaduJ2kCodec_HEADER 1
#include <ossim/imaging/ossimCodecBase.h>

class ossimKakaduJ2kCodec : public ossimCodecBase
{
public:
  ossimKakaduJ2kCodec();

  /**
    * Will return the identifier used to identify the codec type.  For example the Jpeg codec
    * will have "jpeg" as the identifier
    *
    * @return Codec identifier
    */
  virtual ossimString getCodecType() const;

  /**
    * @brief Encode method.
    *
    * Pure virtual method that encodes the passed in buffer to this codec.
    *
    * @param in Input data to encode.
    * 
    * @param out Encoded output data.
    *
    * @return true on success, false on failure.
    */

  virtual bool encode(const ossimRefPtr<ossimImageData> &in,
                      std::vector<ossim_uint8> &out) const;

  /**
    * @brief Decode method.
    *
    * @param in Input data to decode.
    * 
    * @param out Output tile.  If the pointer to ossimImageData is null
    * internally it will be created.  For code loops it is better to pre
    * initialized to correct size.
    *
    * @note Caller should set "out's" image rectangle upon successful
    * decode.
    *
    * @return true on success, false on failure.
    */
  virtual bool decode(const std::vector<ossim_uint8> &in,
                      ossimRefPtr<ossimImageData> &out) const;

  virtual const std::string &getExtension() const;

  virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix=0);
protected:
  std::string  m_extension;
  ossimString m_codecType;
};

#endif
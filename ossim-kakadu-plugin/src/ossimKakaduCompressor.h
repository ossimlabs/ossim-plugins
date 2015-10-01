//----------------------------------------------------------------------------
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Wrapper class to compress whole tiles using kdu_analysis
// object.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduCompressor.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduCompressor_HEADER
#define ossimKakaduCompressor_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>

#include <kdu_compressed.h>
#include <kdu_elementary.h>
#include <kdu_sample_processing.h>
#include <kdu_stripe_compressor.h>

#include <iosfwd>

class ossimFilename;
class ossimImageData;
class ossimImageGeometry;
class ossimIpt;
class ossimKakaduCompressedTarget;

namespace kdu_supp
{
   class jp2_family_tgt;
   class jp2_target; 
}

class ossimKakaduCompressor
{
public:

   // Matches static "COMPRESSION_QUALITY" string array in .cpp.
   enum ossimKakaduCompressionQuality
   {
      // Prefixed with OKP for OSSIM Kakadu Plugin to avoid clashes.
      OKP_UNKNOWN              = 0,
      OKP_USER_DEFINED         = 1,
      OKP_NUMERICALLY_LOSSLESS = 2,
      OKP_VISUALLY_LOSSLESS    = 3,
      OKP_LOSSY                = 4
   };

   /** default constructor */
   ossimKakaduCompressor();

   /** destructor */
   ~ossimKakaduCompressor();

   /**
    * @brief Create method.
    * @param os Stream to write to.
    * @param scalar Scalar type of source tiles to be fed to compressor.
    * @param bands Number of bands in source tiles to be fed to compressor.
    * @param imageRect The image rectangle.
    * @param tileSize The size of a tile.
    * @param tilesTileWrite The number of tiles to be written.
    * If zero, the tlm marker segment will not be used.
    * @param jp2 If true jp2 header and jp2 geotiff block will be written out.
    * @note Throws ossimException on error.
    */
 void create(std::ostream* os,
             ossimScalarType scalar,
             ossim_uint32 bands,
             const ossimIrect& imageRect,
             const ossimIpt& tileSize,
             ossim_uint32 tilesToWrite,
             bool jp2);

   /**
    * @brief Calls "open_codestream" on the m_jp2Target.
    *
    * Note: Only valid if create method was called with jp2 = true.
    */
   void openJp2Codestream();

   /**
    * @brief Write tile method.
    *
    * Writes tiles stream provided to create method.  Note that tiles should
    * be fed to compressor in left to right, top to bottom order.
    * 
    * @param srcTile The source tile to write.
    *
    * @return true on success, false on error.
    */
   bool writeTile(ossimImageData& srcTile);

   /**
    * @brief Finish method.  Every call to "create" should be matched by a
    * "finish".  Note the destructor calls finish.
    */
   void finish();

   /**
    * @brief Sets the quality type.
    * @param type See enumeration for types.
    */
   void setQualityType(ossimKakaduCompressionQuality type);

   /** @return The quality type setting. */
   ossimKakaduCompressionQuality getQualityType() const;

   /**
    * @brief Sets the m_reversible flag.
    *
    * If set to true the compression will be lossless; if not, lossy.
    * Default is lossless.
    * 
    * @param reversible Flag to set.
    */
   void setReversibleFlag(bool reversible);
   
   /** @return The reversible flag. */
   bool getReversibleFlag() const;
   
   /**
    * Set the writer to add an alpha channel to the output png image.
    *
    * @param flag true to create an alpha channel.
    */
   void setAlphaChannelFlag( bool flag );

   /**
    * Retrieve the writer's setting for whether or not to add an 
    * alpha channel to the output png image.
    *
    * @return true if the writer is configured to create an alpha channel.
    */
   bool getAlphaChannelFlag() const;

   /**
    * @brief Sets the number of levels.
    *
    * This must be positive and at least 1.
    * Default = 5 ( r0 - r5 )
    *
    * @param levels Levels to set.
    */
   void setLevels(ossim_int32 levels);

   /** @return The number of levels. */
   ossim_int32 getLevels() const;
   
    /**
    * @brief Sets the number of threads.
    *
    * This must be positive and at least 1.  Default = 1 thread.
    *
    * @param threads The number of threads.
    */
   void setThreads(ossim_int32 threads);

   /** @return The number of threads. */
   ossim_int32 getThreads() const;

   /**
    * @brief Sets the options array.
    *
    * These get passed to the generic kdu_params::parse_string method.  Note
    * this adds options to the list.  Does not zero out existing options.
    *
    * @param options Array of options to add.
    */
   void setOptions(const std::vector<ossimString>& options);
   
   /**
    * @brief Get the array of options.
    * @param options Array to initialize.
    */
   void getOptions(std::vector<ossimString>& options) const;
  
   /**
    * saves the state of the object.
    */
   bool saveState(ossimKeywordlist& kwl, const char* prefix=0)const;
   
   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   bool loadState(const ossimKeywordlist& kwl, const char* prefix=0);
   
   /**
    * Will set the property whose name matches the argument
    * "property->getName()".
    *
    * @param property Object containing property to set.
    *
    * @return true if property was consumed, false if not.
    */
   bool setProperty(ossimRefPtr<ossimProperty> property);
   
   /**
    * @param name Name of property to return.
    * 
    * @returns A pointer to a property object which matches "name".
    */
   ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;
   
   /**
    * Pushes this's names onto the list of property names.
    *
    * @param propertyNames array to add this's property names to.
    */
   void getPropertyNames(std::vector<ossimString>& propertyNames)const;

   /**
    * @brief Writes the geotiff box to the jp2
    * @param geom Output image geometry.
    * @param rect Output rectangle (view rect).
    * @param tmpFile Temp tiff file to write out for reading back in.
    * @param pixelType OSSIM_PIXEL_IS_POINT(0) or OSSIM_PIXEL_IS_AREA(1)
    */
   bool writeGeotiffBox(const ossimImageGeometry* geom,
                        const ossimIrect& rect,
                        const ossimFilename& tmpFile,
                        ossimPixelType pixelType);
   
private:

   void initializeCodingParams(kdu_core::kdu_params* cod, const ossimIrect& imageRect);
   
   int  getNumberOfLayers() const;

   ossimString getQualityTypeString() const;
   void setQualityTypeString(const ossimString& s);

   /**
    * @brief Set levels
    *
    * Number of wavelet decomposition levels, or stages.  May not exceed 32.
    * Kakadu Default is 5
    *
    * @param cod Pointer to cod_params object.
    * @param imageRect The image rectangle.
    * @param levels Number of levels.
    */
   void setLevels(kdu_core::kdu_params* cod,
                  const ossimIrect& imageRect,
                  ossim_int32 levels);

   /**
    * @brief Set code block size.
    *
    * Nominal code-block dimensions (must be powers of 2 no less than 4 and
    * no greater than 1024).
    * Kakadu Default block dimensions are {64,64}
    *
    * @param xSize
    * @param ySize
    */   
   void setCodeBlockSize(kdu_core::kdu_params* cod,
                         ossim_int32 xSize,
                         ossim_int32 ySize);

   /**
    * @brief Sets progression order.
    *
    * Default progression order (may be overridden by Porder).
    * The four character identifiers have the following interpretation:
    * L=layer; R=resolution; C=component; P=position.
    * The first character in the identifier refers to the index which
    * progresses most slowly, while the last refers to the index which
    * progresses most quickly.  [Default is LRCP]
    * Enumerations:  (LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4) 
    *
    * @param corder The progression order which should be one of the above
    * enumerations.
    */   
   void setProgressionOrder(kdu_core::kdu_params* cod,
                            ossim_int32 corder);

   /**
    * @brief Sets the wavelet kernel to use.
    *
    * Wavelet kernels to use.  The special value, `ATK' means that an ATK
    * (Arbitrary Transform Kernel) marker segment is used to store the DWT
    * kernel.  In this case, the `Catk' attribute must be non-zero.
    * [Default is W5X3 if `Creversible' is true, W9X7 if `Creversible' is
    * false, and ATK if `Catk' is non-zero.
    * 
    * Enumerations: (W9X7=0,W5X3=1,ATK=-1) 
    *
    * @param kernel The kernel which should be one of the above enumerations.
    */   
   
   void setWaveletKernel(kdu_core::kdu_params* cod, ossim_int32 kernel);

   /**
    * @brief Sets the number of quality layers.
    *
    * Number of quality layers. May not exceed 16384. Kakadu default is 1.
    *
    * @param layers.
    */   
   void setQualityLayers(kdu_core::kdu_params* cod, ossim_int32 layers);

   void setTlmTileCount(ossim_uint32 tilesToWrite);

   ossimKakaduCompressedTarget* m_target;
   
   kdu_supp::jp2_family_tgt*    m_jp2FamTgt;
   kdu_supp::jp2_target*        m_jp2Target;
   kdu_core::kdu_codestream     m_codestream;
   kdu_core::kdu_thread_env*    m_threadEnv;
   kdu_core::kdu_thread_queue*  m_threadQueue;

   /** Num specs provided in 'flush' calls. */
   int                          m_layerSpecCount;

   /** Layer sizes provided in 'flush' calls. */
   std::vector<kdu_core::kdu_long> m_layerByteSizes;
   
   /** Layer slopes array provided in 'flush' calls. */
   // std::vector<kdu_uint16>      m_layerSlopes;

   /** Image rectangle.  Used for clip in writeTile. */
   ossimIrect                   m_imageRect;

   /** Lossless or lossy */
   bool                         m_reversible;

   /** If true write alpha channel. */
   bool                         m_alpha;

   /** Reduced resolution levels. */
   ossim_int32                  m_levels;

   /** Number of threads. */
   ossim_int32                  m_threads;

   /** Container for kakadu options to pass to kdu_params::parse_string. */
   std::vector<ossimString> m_options;

   ossimKakaduCompressionQuality m_qualityType;

   /** tile to use for normalized float data. */
   ossimRefPtr<ossimImageData> m_normTile;
 
};

#endif /* matches: #ifndef ossimKakaduCompressor_HEADER */
 

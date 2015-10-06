//----------------------------------------------------------------------------
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Wrapper class to compress whole tiles using OpenJPEG code.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimOpjCompressor_HEADER
#define ossimOpjCompressor_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <opj_config.h>
#include <openjpeg.h>
#include <iosfwd>

class ossimFilename;
class ossimImageData;
class ossimImageGeometry;
class ossimIpt;

class ossimOpjCompressor
{
public:

   // Matches static "COMPRESSION_QUALITY" string array in .cpp.
   enum ossimOpjCompressionQuality
   {
      OPJ_UNKNOWN              = 0,
      OPJ_USER_DEFINED         = 1,
      OPJ_NUMERICALLY_LOSSLESS = 2,
      OPJ_VISUALLY_LOSSLESS    = 3,
      OPJ_LOSSY                = 4
   };
   
   /** default constructor */
   ossimOpjCompressor();
   
   /** destructor */
   ~ossimOpjCompressor();
   
   /**
    * @brief Create method.
    * @param os Stream to write to.
    * @param scalar Scalar type of source tiles to be fed to compressor.
    * @param bands Number of bands in source tiles to be fed to compressor.
    * @param imageRect The image rectangle.
    * @param tileSize The size of a tile.
    * @param jp2 If true jp2 header and jp2 geotiff block will be written out.
    * @note Throws ossimException on error.
    */
   void create(std::ostream* os,
               ossimScalarType scalar,
               ossim_uint32 bands,
               const ossimIrect& imageRect,
               const ossimIpt& tileSize,
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
    * @param tileIndex Index starting at 0.  Currently must be sequential.
    * @return true on success, false on error.
    */
   bool writeTile(ossimImageData* srcTile, ossim_uint32 tileIndex);

   /**
    * @brief Finish method.  Every call to "create" should be matched by a
    * "finish".  Note the destructor calls finish.
    */
   void finish();
   
   /**
    * @brief Sets the quality type.
    * @param type See enumeration for types.
    */
   void setQualityType(ossimOpjCompressionQuality type);
   
   /** @return The quality type setting. */
   ossimOpjCompressionQuality getQualityType() const;
   
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
    * Default = 6 ( r0 - r5 )
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
    * @param stream The stream to write to.
    * @param geom Output image geometry.
    * @param rect Output rectangle (view rect).
    * @param tmpFile Temp file written out.
    * @param pixelType OSSIM_PIXEL_IS_POINT(0) or OSSIM_PIXEL_IS_AREA(1)
    */
   bool writeGeotiffBox(std::ostream* stream,
                        const ossimImageGeometry* geom,
                        const ossimIrect& rect,
                        const ossimFilename& tmpFile,
                        ossimPixelType pixelType);
   
   /**
    * @brief Writes the gml box to the jp2
    * @param stream The stream to write to.
    * @param geom Output image geometry.
    * @param rect Output rectangle (view rect).
    * @param pixelType OSSIM_PIXEL_IS_POINT(0) or OSSIM_PIXEL_IS_AREA(1)
    */
   bool writeGmlBox(std::ostream* stream,
                    const ossimImageGeometry* geom,
                    const ossimIrect& rect,
                    ossimPixelType pixelType);
   
private:

   void initOpjCodingParams( bool jp2,
                             const ossimIpt& tileSize,
                             const ossimIrect& imageRect );
   
   int  getNumberOfLayers() const;

   ossimString getQualityTypeString() const;
   void setQualityTypeString(const ossimString& s);

   /**
    * @brief Set levels, class attribute m_levels and
    * m_parameters->numresolution.
    *
    * Number of wavelet decomposition levels, or stages.  May not exceed 32.
    * Opj Default is 6 (0 - 5)
    *
    * @param imageRect The image rectangle.
    */
   void initLevels( const ossimIrect& imageRect );

   /**
    * @brief Set code block size.
    *
    * Nominal code-block dimensions (must be powers of 2 no less than 4 and
    * no greater than 1024).
    * Opj Default block dimensions are {64,64}
    *
    * @param xSize
    * @param ySize
    */   
   void setCodeBlockSize( ossim_int32 xSize, ossim_int32 ySize);

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
   void setProgressionOrder( OPJ_PROG_ORDER progressionOrder );

   void setTlmTileCount(ossim_uint32 tilesToWrite);

   opj_cparameters_t* createOpjCodingParameters(
      bool jp2,
      const ossimIpt& tileSize,
      const ossimIrect& imageRect ) const;
   
   opj_codec_t* createOpjCodec( bool jp2 ) const;

   opj_stream_t* createOpjStream( std::ostream* os ) const;
   
   opj_image_t* createOpjImage( ossimScalarType scalar,
                                ossim_uint32 bands,
                                const ossimIrect& imageRect ) const;

   opj_cparameters_t* m_params;
   opj_codec_t*       m_codec;
   opj_stream_t*      m_stream;
   
   opj_image_t*       m_image;
   
   
   // opj_codec* m_stream;
   // opj_image_t* m_image = 0;

   
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

   /** Container for Opj options. */
   std::vector<ossimString> m_options;

   ossimOpjCompressionQuality m_qualityType;
   
};

#endif /* matches: #ifndef ossimOpjCompressor_HEADER */
 

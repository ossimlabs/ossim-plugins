//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Portable Network Graphics (PNG) writer.
//
//----------------------------------------------------------------------------
// $Id: ossimPngWriter.h 19878 2011-07-28 16:27:26Z dburken $
#ifndef ossimPngWriter_HEADER
#define ossimPngWriter_HEADER


#include <ossim/base/ossimRtti.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNBandLutDataObject.h>
#include <ossim/base/ossimRefPtr.h>
#include <png.h>
#include <ostream>

class ossimPngWriter : public ossimImageFileWriter
{
public:

   /* default constructor */
   ossimPngWriter();

   /* virtual destructor */
   virtual ~ossimPngWriter();

   /** @return "png writer" */
   virtual ossimString getShortName() const;

   /** @return "ossim png writer" */
   virtual ossimString getLongName()  const;

   /** @return "ossimPngReader" */
   virtual ossimString getClassName()    const;

   /**
    * Returns a 3-letter extension from the image type descriptor 
    * (theOutputImageType) that can be used for image file extensions.
    *
    * @param imageType string representing image type.
    *
    * @return the 3-letter string extension.
    */
   virtual ossimString getExtension() const;

   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * This writer only has one type "png".
    *
    * @param imageTypeList stl::vector<ossimString> list to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   
   virtual bool isOpen()const;   
   
   virtual bool open();

   virtual void close();
   
   /**
    * saves the state of the object.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;
   
   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);

   /**
    * Will set the property whose name matches the argument
    * "property->getName()".
    *
    * @param property Object containing property to set.
    */
   virtual void setProperty(ossimRefPtr<ossimProperty> property);

   /**
    * @param name Name of property to return.
    * 
    * @returns A pointer to a property object which matches "name".
    */
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;

   /**
    * Pushes this's names onto the list of property names.
    *
    * @param propertyNames array to add this's property names to.
    */
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;

   bool hasImageType(const ossimString& imageType) const;

   void setLut(const ossimNBandLutDataObject& lut);

   /**
    * Get the png compression level as a string.
    *
    * @return The level which will be one of:
    * z_no_compression
    * z_best_speed
    * z_best_compression
    * z_default_compression
    */
   ossimString getCompressionLevel() const;

   /**
    * Set the png compression level from a string.
    *
    * @param level Should be one of:
    * z_no_compression
    * z_best_speed
    * z_best_compression
    * z_default_compression
    *
    * @return true on success, false if level is not accepted.
    */
   bool setCompressionLevel(const ossimString& level);

   /**
    * Retrieve the writer's setting for whether or not to add an 
    * alpha channel to the output png image.
    *
    * @return true if the writer is configured to create an alpha channel.
    */
   bool getAlphaChannelFlag( void ) const;

   /**
    * Set the writer to add an alpha channel to the output png image.
    *
    * @param flag true to create an alpha channel.
    */
   void setAlphaChannelFlag( bool flag );

   /**
    * @brief Method to write the image to a stream.
    *
    * @return true on success, false on error.
    */
   virtual bool writeStream();

   /**
    * @brief Sets the output stream to write to.
    *
    * The stream will not be closed/deleted by this object.
    *
    * @param output The stream to write to.
    */
   virtual bool setOutputStream(std::ostream& stream);

private:

   /**
    * @brief Writes the file to disk or a stream.
    * @return true on success, false on error.
    */
   virtual bool writeFile();

   /**
    * @brief Callback method for writing to stream.  This allows us to choose
    * the stream; hence, writing to a file or memory.  This will be passed
    * to libpng's png_set_write_fn.
    */
   static void pngWriteData(png_structp png_ptr, png_bytep data,
                            png_size_t length);

   /**
    * @brief Callback method for a flush.  This is a required method for
    * png_set_write_fn.
    */
   static void pngFlush(png_structp png_ptr);

   /** @return true is swapping should be performed, false if not. */
   bool doSwap(ossimScalarType outputScalar) const;

   bool isLutEnabled()const;

   /**
    * @brief writes optionial chunks.
    */
   void writeOptionalChunks(png_structp pp, png_infop info);
   
   //---
   // Chunck writers.  In place but not implemnted yet.
   //---

   /**
    * Write transparent color chunk (tRNS)
    * Location:   
    * Multiple:   
    */
   void writeTransparentColorChunk(png_structp pp, png_infop info);

   /**
    * Write gamm chunk (gAMMA)
    * Location:   Before PLTE, before first IDAT.
    * Multiple:   no
    */
   void writeGammaChunk(png_structp pp, png_infop info);

   /**
    * Write background color chunk (bKGD)
    * Location:   After PLTE, before first IDAT.
    * Multiple:   no
    */
   void writeBackgroundColorChunk(png_structp pp, png_infop info);
   
   /**
    * Write time stamp chunk (tIME).
    * Location: anywhere
    * Multiple: no
    */
   void writeTimeStampChunk(png_structp pp, png_infop info);
   
   /**
    * Write any text (tEXt, zTXt).
    * Location: anywhere
    * Multiple: yes
    */
   void writeLatinTextChunk(png_structp pp, png_infop info);
   
   /**
    * Write any text (iTXt).
    * Location: anywhere
    * Multiple: yes
    */
   void writeInternationalTextChunk(png_structp pp, png_infop info);
   
   /**
    * Write histogram (hIST)
    * Location: after PLTE, before first IDAT
    * Multiple:   no
    */
   void writeHistogram(png_structp pp, png_infop info);
   
   /**
    * Write suggested palette (sPLT)
    * Location: before first IDAT
    * Multiple: yes
    */
   void writeSuggestedPalette(png_structp pp, png_infop info);

   /**
    * Write significant bits (sBIT)
    * Location: before PLTE and first IDAT
    * Multiple: no
    */
   void writeSignificantBits(png_structp pp, png_infop info);
   
   /**
    * Write physical pixel dimensions (pHYs)
    * Location: before first IDAT
    * Multiple: no
    */
   void writePhysicalPixelDimensions(png_structp pp, png_infop info);
   
   /**
    * Write physical scale (sCAL)
    * Location: before first IDAT
    * Multiple: no
    */
   void writePhysicalScale(png_structp pp, png_infop info);
   
   /**
    * Write image offset (oFFs)
    * Location: before first IDAT
    * Multiple: no
    */
   void writeImageOffset(png_structp pp, png_infop info);
   
   /**
    * Write pixel calibration (pCAL)
    * Location: after PLTE, before first IDAT
    * Multiple: no
    */
   void writePixelCalibration(png_structp pp, png_infop info);
   
   /**
    * Write fractal parameters (fRAc)
    * Location: anywhere
    * Multiple: yes
    */
   void writeFractalParameters(png_structp pp, png_infop info);

   /**
    * Get the png color type.
    *
    * @return png color type.  One of:
    * PNG_COLOR_TYPE_GRAY
    * PNG_COLOR_TYPE_PALETTE
    * PNG_COLOR_TYPE_RGB
    * PNG_COLOR_TYPE_RGB_ALPHA
    * PNG_COLOR_TYPE_GRAY_ALPHA
    */
   ossim_int32 getColorType(ossim_int32 bands) const;
   
   ossim_int32 getBitDepth(ossimScalarType outputScalar) const;
   
   std::ostream* theOutputStream;
   bool          theOwnsStreamFlag;

   /**
    * Compression level from zlib.h:
    * Z_NO_COMPRESSION         0
    * Z_BEST_SPEED             1
    * Z_BEST_COMPRESSION       9
    * Z_DEFAULT_COMPRESSION  (-1)
    *
    * Defaulted to Z_BEST_COMPRESSION
    */
   ossim_int32       theCompressionLevel;

   /**
    * Interlace support:
    * PNG_INTERLACE_ADAM7
    * PNG_INTERLACE_NONE
    *
    * Defaulted to PNG_INTERLACE_NONE
    */
   ossim_int32 theInterlaceSupport;

   /**
    * Z_FILTERED            1
    * Z_HUFFMAN_ONLY        2
    * Z_RLE                 3
    * Z_FIXED               4
    * Z_DEFAULT_STRATEGY    0
    *
    * Default to Z_FILTERED if not palette image,
    * Z_DEFAULT_STRATEGY if palette.
    */
   ossim_int32 theCompressionStratagy;

   /**
    * PNG_NO_FILTERS     0x00
    * PNG_FILTER_NONE    0x08
    * PNG_FILTER_SUB     0x10
    * PNG_FILTER_UP      0x20
    * PNG_FILTER_AVG     0x40
    * PNG_FILTER_PAETH   0x80
    *
    * Defaulted to PNG_FILTER_NONE if palette;  If not palette this is left
    * up to zlib to decide unless the user overrides it.
    */
   ossim_int32 thePngFilter;

   /**
    * For gamma support.
    * gamma multiplied by 100,000 and rounded to the nearest integer.
    * So if gamma is 1/2.2 (or 0.45454545...), the value in the gAMA chunk is
    * 45,455.
    *
    * There can be only one gAMA chunk, and it must appear before any
    * IDATs and also before the PLTE chunk, if one is present.
    */
   bool                                 theGammaFlag;
   ossim_float64                        theGamma;

   /** Time support, either on or off.  Default = on. */
   bool                                 theTimeFlag;

   /** For alpha channel support. */
   bool                                 theAlphaChannelFlag;

   /** For background support. */
   bool                                 theBackgroundFlag;
   ossim_uint16                         theBackgroundRed;
   ossim_uint16                         theBackgroundGreen;
   ossim_uint16                         theBackgroundBlue;
   ossim_uint16                         theBackgroundGray;

   /** For transparent color support. */
   bool                                 theTransparentFlag;
   ossim_uint16                         theTransparentRed;
   ossim_uint16                         theTransparentGreen;
   ossim_uint16                         theTransparentBlue;
   ossim_uint16                         theTransparentGray;

   TYPE_DATA

};

#endif /* #ifndef ossimPngVoid Writer_HEADER */

#include "ossimPngCodec.h"
#include <ossim/base/ossimConstants.h>
#include <png.h>
#include <cstdlib>

static const char ADD_ALPHA_CHANNEL_KW[] = "add_alpha_channel";

static void user_read_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
   ossim_uint8** input_pointer = reinterpret_cast<ossim_uint8**>(png_get_io_ptr (png_ptr));

   memcpy (data, *input_pointer, sizeof (ossim_uint8) * length);
   (*input_pointer) += length;
}

static void PngWriteCallback(png_structp  png_ptr, png_bytep data, png_size_t length) 
{
   std::vector<ossim_uint8> *p = (std::vector<ossim_uint8>*)png_get_io_ptr(png_ptr);
   p->insert(p->end(), data, data + length);
}
 
struct TPngDestructor {
   png_struct *p;
   TPngDestructor(png_struct *p) : p(p)  {}
   ~TPngDestructor() { if (p) {  png_destroy_write_struct(&p, NULL); } }
};

ossimPngCodec::ossimPngCodec(bool addAlpha)
   :m_addAlphaChannel(addAlpha),
    m_ext("png")
{

}

ossimString ossimPngCodec::getCodecType()const
{
   return "png";
}

const std::string& ossimPngCodec::getExtension() const
{
   return m_ext; // "png"
}


bool ossimPngCodec::encode(const ossimRefPtr<ossimImageData>& in,
                           std::vector<ossim_uint8>& out ) const
{
   out.clear();
   ossim_int32 colorType = -1;
   ossim_int32 bitDepth = 0;
   if(!in->getBuf()) return false;
   if(in->getNumberOfBands() == 1)
   {
      if(m_addAlphaChannel)
      {
         colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
      }
      else
      {
         colorType = PNG_COLOR_TYPE_GRAY;
      }
   }
   else if(in->getNumberOfBands() == 3)
   {
      if(m_addAlphaChannel)
      {
         colorType = PNG_COLOR_TYPE_RGB_ALPHA;
      }
      else
      {
         colorType = PNG_COLOR_TYPE_RGB;
      }
   }
   if(colorType < 0) return false;

   switch(in->getScalarType())
   {
      case OSSIM_UINT8:
      {
         bitDepth = 8;
         break;
      }
      case OSSIM_USHORT11:
      case OSSIM_UINT16:
      {
         bitDepth = 16;
         break;
      }
      default:
      {
         bitDepth = 0;
      }
   }
   if(bitDepth == 0) return false;
   // std::cout << "bitDepth = " << bitDepth << ", BANDS = " << in->getNumberOfBands() << std::endl;
   ossim_int32 w = in->getWidth();
   ossim_int32 h = in->getHeight();
   png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   TPngDestructor destroyPng(p);
   png_infop info_ptr = png_create_info_struct(p);
   setjmp(png_jmpbuf(p));

   png_set_IHDR(p, info_ptr, w, h, bitDepth,
                colorType,
                PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
   png_set_compression_level(p, 1);

   switch(colorType)
   {
      case PNG_COLOR_TYPE_GRAY:
      {
         if(bitDepth == 8)
         {
            ossim_uint8* buf = (ossim_uint8*)in->getBuf();
            std::vector<ossim_uint8*> rows(h);
            for(ossim_int32 y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(buf + y * w) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }
         else if(bitDepth == 16)
         {
            ossim_uint16* buf = (ossim_uint16*)in->getBuf();
            std::vector<ossim_uint8*> rows(h);
            for(int y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(buf + y * w) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }
         break;
      }
      case PNG_COLOR_TYPE_GRAY_ALPHA:
      {
         if(bitDepth == 8)
         {
            std::vector<ossim_uint8> buf(w*h*2);
            std::vector<ossim_uint8*> rows(h);
            ossim_uint8* bufPtr = &buf.front();
            in->unloadTileToBipAlpha(&buf.front(), in->getImageRectangle(), in->getImageRectangle());
            for(int y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(bufPtr + y * (w*2)) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

         }
         else
         {
            std::vector<ossim_uint16> buf(w*h*2);
            std::vector<ossim_uint8*> rows(h);
            ossim_uint16* bufPtr = &buf.front();
            in->unloadTileToBipAlpha(&buf.front(), in->getImageRectangle(), in->getImageRectangle());
            for(ossim_int32 y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(bufPtr + y * (w*2)) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }

         break;
      }
      case PNG_COLOR_TYPE_RGB:
      {
         if(bitDepth == 8)
         {
            std::vector<ossim_uint8> buf(w*h*3);
            std::vector<ossim_uint8*> rows(h);
            in->unloadTile(&buf.front(), in->getImageRectangle(), in->getImageRectangle(), OSSIM_BIP);
            ossim_uint8* bufPtr = &buf.front();
            for(ossim_int32 y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(bufPtr + y * (w*3)) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }
         else
         {
            std::vector<ossim_uint16> buf(w*h*3);
            std::vector<ossim_uint8*> rows(h);
            ossim_uint16* bufPtr = &buf.front();
            in->unloadTile(&buf.front(), in->getImageRectangle(), in->getImageRectangle(),OSSIM_BIP);
            for(ossim_int32 y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(bufPtr + y * (w*3)) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }
         break;
      }
      case PNG_COLOR_TYPE_RGB_ALPHA:
      {
         if(bitDepth == 8)
         {
            std::vector<ossim_uint8> buf(w*h*4);
            std::vector<ossim_uint8*> rows(h);
            in->unloadTileToBipAlpha(&buf.front(), in->getImageRectangle(), in->getImageRectangle());
            ossim_uint8* bufPtr = &buf.front();
            for(int y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(bufPtr + y * (w*4)) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }
         else
         {
            std::vector<ossim_uint16> buf(w*h*4);
            std::vector<ossim_uint8*> rows(h);
            in->unloadTileToBipAlpha(&buf.front(), in->getImageRectangle(), in->getImageRectangle());
            ossim_uint16* bufPtr = &buf.front();
            for(ossim_int32 y=0; y<h;++y)
            {
               rows[y] = (ossim_uint8*)(bufPtr + y * (w*4)) ;    		
            }
            png_set_rows(p, info_ptr, &rows[0]);
            png_set_write_fn(p, &out, PngWriteCallback, NULL);
            png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
         }
         break;
      }
   }

   return true;
}

// #include <fstream>
// static bool traced = false;

bool ossimPngCodec::decode(const std::vector<ossim_uint8>& in,
                           ossimRefPtr<ossimImageData>& out ) const
{
   bool result = true;
   if ( in.size() )
   {

#if 0 /* Please leave for debug. drb */
      if ( !traced )
      {
         std::ofstream os;
         os.open("debug.png", ios::out | ios::binary);
         if (os.good())
         {
            os.write((const char*)(&in.front()), in.size());
            cout << "wrote file: debug.png" << endl;
         }
         traced = true;
      }
#endif

      png_structp  png_ptr = 0;
      png_infop   info_ptr = 0;

      png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);

      // Initialize info structure
      info_ptr = png_create_info_struct (png_ptr);

      // Setup Exception handling
      setjmp (png_jmpbuf(png_ptr));
      
      const ossim_uint8* inputPointer = &in.front();//&pngData_arg[0];
      png_set_read_fn (png_ptr, reinterpret_cast<void*> (&inputPointer), user_read_data);
      
      png_read_info (png_ptr, info_ptr);
      
      ossim_uint32 width           = png_get_image_width(png_ptr,info_ptr);
      ossim_uint32 height          = png_get_image_height(png_ptr,info_ptr);
      ossim_uint8  bit_depth       = png_get_bit_depth(png_ptr,info_ptr);
      ossim_uint8  color_type      = png_get_color_type(png_ptr,info_ptr);
      // ossim_uint8  channels         = png_get_channels(png_ptr, info_ptr);
      ossim_uint32 bytes_per_pixel = (bit_depth <= 8) ? 1 : 2;
      ossimScalarType scalar_type  = ((bytes_per_pixel==1)?OSSIM_UINT8:OSSIM_UINT16);

      if( color_type == PNG_COLOR_TYPE_PALETTE)
      {
         png_set_palette_to_rgb(png_ptr);
      }
      
      if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      {
         png_set_expand_gray_1_2_4_to_8(png_ptr);
      }
      
      if(png_get_valid(png_ptr, info_ptr,PNG_INFO_tRNS))
      {
         png_set_tRNS_to_alpha(png_ptr);
      }
      
      if(bit_depth < 8)
      {
         png_set_packing(png_ptr);
      }

      // Update the info structures after the transformations are set.
      png_read_update_info(png_ptr, info_ptr);

      // Update the input/output bands after any tranformations...
      ossim_uint32 input_bands = png_get_channels(png_ptr, info_ptr);
      ossim_uint32 output_bands = input_bands;
      if( (output_bands == 2) || (output_bands==4) )
      {
         output_bands = output_bands - 1;
      }
      
      ossim_uint32 bytes = height*width*input_bands*bytes_per_pixel;
      
#if 0 /* Please leave for debug. drb */
      ossim_uint8  filter_method    = png_get_filter_type(png_ptr,info_ptr);
      ossim_uint8  compression_type = png_get_compression_type(png_ptr,info_ptr);
      ossim_uint8  interlace_type   = png_get_interlace_type(png_ptr,info_ptr);
      cout << "width:            " << width
           << "\nheight:           " << height
           << "\nbit_depth:        " << (int)bit_depth
           << "\ncolor_type:       " << (int)color_type
           << "\nfilter_method:    " << (int)filter_method
           << "\ncompression_type: " << (int)compression_type
           << "\ninterlace_type:   " << (int)interlace_type
           << "\nchannels:         " << (int)channels
           << "\ninput_bands:      " << input_bands
           << "\noutput_bands:     " << output_bands
           << "\nbytes_per_pixel:  " << bytes_per_pixel
           << "\nbytes:            " << bytes
           << "\n";
#endif
      std::vector<ossim_uint8> data(bytes);
      std::vector<ossim_uint8*> rowPointers(height);
      ossim_uint8* dataPtr = &data.front();
      for (ossim_uint32 y = 0; y < height; ++y)
      {
         rowPointers[y] = reinterpret_cast<ossim_uint8*>
            (dataPtr + ( y*(width*bytes_per_pixel*input_bands)));
      }

      png_read_image (png_ptr, &rowPointers.front());
   
      // now allocate the ossimImageData object if not already allocated
      if(!out.valid())
      {
         out = new ossimImageData(0, scalar_type, output_bands, width, height);
         out->initialize();
      }
      else
      {
         out->setNumberOfDataComponents(output_bands);
         out->setImageRectangleAndBands(ossimIrect(0,0,width-1,height-1), output_bands);
         out->initialize();
      }

      if(input_bands == 1)
      {
         // ossim_uint32 idx = 0;
         memcpy(out->getBuf(0), dataPtr, bytes);
         out->validate();
         // once we support alpha channel properly we will need to add alpha settings here

      }
      else if(input_bands == 2)
      {
         //std::cout << "DECODING 2 channels\n";
         ossim_uint32 size = width*height;
         ossim_uint32 idx = 0;
         if(scalar_type == OSSIM_UINT16)
         {
            ossim_uint16* tempDataPtr = reinterpret_cast<ossim_uint16*> (dataPtr);
            ossim_uint16* buf = static_cast<ossim_uint16*>(out->getBuf(0));

            for(idx = 0; idx < size;++idx)
            {
               *buf = tempDataPtr[0];

               tempDataPtr+=2;++buf;
            }
            out->validate();
         }
         else if(scalar_type == OSSIM_UINT8)
         {
            ossim_uint8* tempDataPtr = dataPtr;
            ossim_uint8* buf = static_cast<ossim_uint8*>(out->getBuf(0));

            for(idx = 0; idx < size;++idx)
            {
               *buf = *tempDataPtr;

               tempDataPtr+=2;++buf;
            }
            out->validate();
         }
         else
         {
            result = false;
         }
      }
      else if(input_bands == 4)
      {
         // cout << "scalarType: " << scalarType << endl;
      
         //std::cout << "DECODING 4 channels\n";
         ossim_uint32 size = width*height;
         ossim_uint32 idx = 0;
         if(scalar_type == OSSIM_UINT16)
         {
            ossim_uint16* tempDataPtr = reinterpret_cast<ossim_uint16*> (dataPtr);
            ossim_uint16* buf1 = static_cast<ossim_uint16*>(out->getBuf(0));
            ossim_uint16* buf2 = static_cast<ossim_uint16*>(out->getBuf(1));
            ossim_uint16* buf3 = static_cast<ossim_uint16*>(out->getBuf(2));

            for(idx = 0; idx < size;++idx)
            {
               *buf1 = tempDataPtr[0];
               *buf2 = tempDataPtr[1];
               *buf3 = tempDataPtr[2];

               tempDataPtr+=4;++buf1;++buf2;++buf3;
            }
            out->validate();
         }
         else if(scalar_type == OSSIM_UINT8)
         {
            ossim_uint8* tempDataPtr = dataPtr;
            ossim_uint8* buf1 = static_cast<ossim_uint8*>(out->getBuf(0));
            ossim_uint8* buf2 = static_cast<ossim_uint8*>(out->getBuf(1));
            ossim_uint8* buf3 = static_cast<ossim_uint8*>(out->getBuf(2));

            for(idx = 0; idx < size;++idx)
            {
               *buf1 = tempDataPtr[0];
               *buf2 = tempDataPtr[1];
               *buf3 = tempDataPtr[2];

               tempDataPtr+=4;++buf1;++buf2;++buf3;
            }
            out->validate();
         }
         else
         {
            result = false;
         }
      }
      else
      {
         out->loadTile(dataPtr, out->getImageRectangle(), OSSIM_BIP);
      }

      if (info_ptr)
      {
         png_free_data (png_ptr, info_ptr, PNG_FREE_ALL, -1);
      }
      if (png_ptr)
      {
         png_destroy_read_struct (&png_ptr, 0, 0);
      }
   }
   else
   {
      result = false;
   }
   
   return result;
}

void ossimPngCodec::setProperty(ossimRefPtr<ossimProperty> property)
{
   if(property->getName() == ADD_ALPHA_CHANNEL_KW)
   {
      m_addAlphaChannel = property->valueToString().toBool();
   }
   else
   {
      ossimCodecBase::setProperty(property);
   }
}

ossimRefPtr<ossimProperty> ossimPngCodec::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> result;

   if(name == ADD_ALPHA_CHANNEL_KW)
   {

   }
   else
   {
      result = ossimCodecBase::getProperty(name);
   }

   return result;
}

void ossimPngCodec::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(ADD_ALPHA_CHANNEL_KW);
}

bool ossimPngCodec::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   ossimString addAlphaChannel = kwl.find(prefix, ADD_ALPHA_CHANNEL_KW);

   if(!addAlphaChannel.empty())
   {
      m_addAlphaChannel = addAlphaChannel.toBool();
   }

   return ossimCodecBase::loadState(kwl, prefix);
}

bool ossimPngCodec::saveState(ossimKeywordlist& kwl, const char* prefix)const
{
   kwl.add(prefix, ADD_ALPHA_CHANNEL_KW, m_addAlphaChannel);

   return ossimCodecBase::saveState(kwl, prefix);
}

#include "ossimPngCodec.h"
#include <ossim/base/ossimConstants.h>
#include <png.h>
#include <stdlib.h>

static const char ADD_ALPHA_CHANNEL_KW[] = "add_alpha_channel";

RTTI_DEF1(ossimPngCodec, "ossimPngCodec", ossimCodecBase);


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
   :m_addAlphaChannel(addAlpha)
{

}

ossimString ossimPngCodec::getCodecType()const
{
   return "png";
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

bool ossimPngCodec::decode(const std::vector<ossim_uint8>& in,
                           ossimRefPtr<ossimImageData>& out ) const
{
   bool result = true;
   ossim_uint32 y = 0;
   png_structp  pngPtr = 0;
   png_infop    infoPtr = 0;
   png_uint_32 pngWidth = 0;
   png_uint_32 pngHeight = 0;
   ossim_int32  pngBitDepth=0, pngByteDepth = 0, pngColorType=0, pngInterlaceType=0;

   // png_bytep* row_pointers=0;

   if (in.empty())
   {
      return false;
   }

   pngPtr = png_create_read_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
   //assert (png_ptr && "creating png_create_write_structpng_create_write_struct failed");

   // Initialize info structure
   infoPtr = png_create_info_struct (pngPtr);

   // Setup Exception handling
   setjmp (png_jmpbuf(pngPtr));

   const ossim_uint8* inputPointer = &in.front();//&pngData_arg[0];
   png_set_read_fn (pngPtr, reinterpret_cast<void*> (&inputPointer), user_read_data);

   png_read_info (pngPtr, infoPtr);

   png_get_IHDR (pngPtr, infoPtr, &pngWidth, &pngHeight, &pngBitDepth,
                 &pngColorType, &pngInterlaceType, NULL, NULL);



   // ensure a color bit depth of 8
   //assert(png_bit_depth==sizeof(T)*8);

   ossim_uint32 pngChannels = 0;
   switch (pngColorType)
   {
      case PNG_COLOR_TYPE_GRAY:
      {
         pngChannels = 1;
         break;
      }
      case PNG_COLOR_TYPE_GRAY_ALPHA:
      {
         pngChannels = 2;
         break;	  	
      }
      case PNG_COLOR_TYPE_RGB:
      {
         pngChannels = 3;
         break;	  	
      }
      case PNG_COLOR_TYPE_RGB_ALPHA:
      {
         pngChannels = 4;
         break;	  	
      }
      default:
      {
         pngChannels = 0;
         break;	  	
      }
   }
   pngByteDepth = pngBitDepth>>3;
   //imageData_arg.clear ();
   //imageData_arg.resize (png_height * png_width * png_channels);

   ossim_uint32 bytes = pngHeight*pngWidth*pngChannels*pngByteDepth;
   //row_pointers = reinterpret_cast<png_bytep*> (malloc (sizeof(png_bytep) * png_height));
   std::vector<ossim_uint8> data(bytes);
   std::vector<ossim_uint8*> rowPointers(pngHeight);
   ossim_uint8* dataPtr = &data.front();
   for (y = 0; y < pngHeight; y++)
   {
      rowPointers[y] = reinterpret_cast<ossim_uint8*> (dataPtr + ( y*(pngWidth*pngByteDepth*pngChannels)));
   }

   png_read_image (pngPtr, &rowPointers.front());

   ossimScalarType scalarType = ((pngByteDepth==1)?OSSIM_UINT8:OSSIM_UINT16);
   ossim_uint32 bands = pngChannels;

   if((bands == 2) ||(bands==4))
   {
      bands = bands - 1;
   }
   // now allocate the ossimImageData object if not already allocated
   if(!out.valid())
   {
      out = new ossimImageData(0, scalarType, bands, pngWidth, pngHeight);
      out->initialize();
   }
   else
   {
      out->setNumberOfDataComponents(bands);
      out->setImageRectangleAndBands(ossimIrect(0,0,pngWidth-1,pngHeight-1), bands);
      out->initialize();
   }

   if(pngChannels == 1)
   {
      // ossim_uint32 idx = 0;
      memcpy(out->getBuf(0), dataPtr, bytes);
      out->validate();
      // once we support alpha channel properly we will need to add alpha settings here

   }
   else if(pngChannels == 2)
   {
      //std::cout << "DECODING 2 channels\n";
      ossim_uint32 size = pngWidth*pngHeight;
      ossim_uint32 idx = 0;
      if(scalarType == OSSIM_UINT16)
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
      else if(scalarType == OSSIM_UINT8)
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
   else if(pngChannels == 4)
   {
      //std::cout << "DECODING 4 channels\n";
      ossim_uint32 size = pngWidth*pngHeight;
      ossim_uint32 idx = 0;
      if(scalarType == OSSIM_UINT16)
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
      else if(scalarType == OSSIM_UINT8)
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

   if (infoPtr)
   {
      png_free_data (pngPtr, infoPtr, PNG_FREE_ALL, -1);
   }
   if (pngPtr)
   {
      png_destroy_read_struct (&pngPtr, 0, 0);
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

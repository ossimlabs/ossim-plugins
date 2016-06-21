//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************


#include "ossimFftw3Filter.h"
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/base/ossimStringProperty.h>
#include <complex.h>
#include <fftw3.h>

RTTI_DEF1(ossimFftw3Filter, "ossimFftw3Filter", ossimImageSourceFilter);

ossimFftw3Filter::ossimFftw3Filter(ossimObject* owner)
   :ossimImageSourceFilter(owner),
    m_tile(0),
    m_xformDir(FORWARD),
    m_scalarRemapper(new ossimScalarRemapper())
{
   m_scalarRemapper->setOutputScalarType(OSSIM_DOUBLE);
}

ossimFftw3Filter::ossimFftw3Filter(ossimImageSource* inputSource)
   :ossimImageSourceFilter(inputSource),
    m_tile(0),
    m_xformDir(FORWARD),
    m_scalarRemapper(new ossimScalarRemapper())
{
   m_scalarRemapper->setOutputScalarType(OSSIM_DOUBLE);
}

ossimFftw3Filter::ossimFftw3Filter(ossimObject* owner,
                               ossimImageSource* inputSource)
   :ossimImageSourceFilter(owner, inputSource),
    m_tile(0),
    m_xformDir(FORWARD),
    m_scalarRemapper(new ossimScalarRemapper())
{
   m_scalarRemapper->setOutputScalarType(OSSIM_DOUBLE);
}

ossimFftw3Filter::~ossimFftw3Filter()
{
   if(m_scalarRemapper.valid())
   {
      m_scalarRemapper->disconnect();
      m_scalarRemapper = 0;
   }
}

ossimRefPtr<ossimImageData> ossimFftw3Filter::getTile(const ossimIrect& rect,
                                                    ossim_uint32 resLevel)
{
   if(!isSourceEnabled())
   {
      return ossimImageSourceFilter::getTile(rect, resLevel);
   }
   
   ossimIrect tempRequest = rect;

   ossim_uint32 w = rect.width();
   ossim_uint32 h = rect.height();
   
   if (w & 1)
      ++w;
   if (h & 1)
      ++h;

   tempRequest = ossimIrect(rect.ul().x,     rect.ul().y,
                            rect.ul().x+w-1, rect.ul().y+h-1);
   
   ossimRefPtr<ossimImageData> inTile = m_scalarRemapper->getTile(tempRequest, resLevel);
   if(!inTile.valid())
      return inTile;
   if(!m_tile.valid())
      initialize();
   if(!m_tile.valid() || !inTile->getBuf())
      return m_tile;

   m_tile->setImageRectangle(rect);
   ossimRefPtr<ossimImageData> tempTile = m_tile;
   
   if(rect != tempRequest)
   {
      tempTile = (ossimImageData*)m_tile->dup();
      tempTile->setImageRectangle(tempRequest);
   }

   runFft(inTile, tempTile);
          
   if(tempTile != m_tile)
   {
      m_tile->loadTile(tempTile.get());
   }
   
   m_tile->validate();

   return m_tile;
}

void ossimFftw3Filter::initialize()
{
   ossimImageSourceFilter::initialize();

   m_tile = ossimImageDataFactory::instance()->create(this, this);
   
   if(m_tile.valid())
   {
      m_tile->initialize();
   }
   if(m_xformDir == FORWARD)
   {
      m_scalarRemapper->setOutputScalarType(OSSIM_NORMALIZED_DOUBLE);
   }
   else
   {
      m_scalarRemapper->setOutputScalarType(OSSIM_DOUBLE);
   }
   m_scalarRemapper->connectMyInputTo(0, getInput());
}

ossimScalarType ossimFftw3Filter::getOutputScalarType() const
{
   if(!isSourceEnabled())
   {
      return ossimImageSourceFilter::getOutputScalarType();
   }
   
   if(m_xformDir == FORWARD)
   {
      return OSSIM_DOUBLE;
   }
   
   return OSSIM_NORMALIZED_DOUBLE;
}

double ossimFftw3Filter::getNullPixelValue(ossim_uint32 band)const
{
   if(!isSourceEnabled())
   {
      return ossimImageSourceFilter::getNullPixelValue(band);
   }
   if(m_xformDir == FORWARD)
   {
      return ossim::nan();
   }

   // it will invert to a normalized float output
   return 0.0;
}

double ossimFftw3Filter::getMinPixelValue(ossim_uint32 band)const
{
   if(!isSourceEnabled())
   {
      return ossimImageSourceFilter::getMinPixelValue(band);
   }
   if(m_xformDir == FORWARD)
   {
      return OSSIM_DEFAULT_MIN_PIX_DOUBLE;
   }
   return OSSIM_DEFAULT_MIN_PIX_NORM_DOUBLE;
}

double ossimFftw3Filter::getMaxPixelValue(ossim_uint32 band)const
{
   if(!isSourceEnabled())
   {
      return ossimImageSourceFilter::getMaxPixelValue(band);
   }
   if(m_xformDir == FORWARD)
   {
      return OSSIM_DEFAULT_MAX_PIX_DOUBLE;
   }
   return OSSIM_DEFAULT_MAX_PIX_NORM_DOUBLE;
}

ossim_uint32 ossimFftw3Filter::getNumberOfOutputBands() const
{
   if(!isSourceEnabled())
   {
      return ossimImageSourceFilter::getNumberOfOutputBands();
   }
   ossim_uint32 bands = ossimImageSourceFilter::getNumberOfOutputBands();
   
   if(m_xformDir == FORWARD)
   {
      bands *=2;
   }
   else 
   {
      bands /=2;
   }

   return bands;
}

void ossimFftw3Filter::setForward()
{
   m_xformDir = FORWARD;
}

void ossimFftw3Filter::setInverse()
{
   m_xformDir = INVERSE;
}

ossimString ossimFftw3Filter::getDirectionTypeAsString()const
{
   if(m_xformDir == FORWARD)
   {
      return "Forward";
   }

   return "Inverse";
}

void ossimFftw3Filter::setDirectionType(const ossimString& directionType)
{
   ossimString tempDirectionType = directionType;
   tempDirectionType = tempDirectionType.downcase();
   
   if(tempDirectionType.contains("forward"))
   {
      setDirectionType(FORWARD);
   }
   else
   {
      setDirectionType(INVERSE);
   }
}

void ossimFftw3Filter::setDirectionType(TransformDirection dir)
{
   m_xformDir = dir;
   if(m_tile.valid())
      m_tile = NULL;
}

ossimRefPtr<ossimProperty> ossimFftw3Filter::getProperty(const ossimString& name)const
{
   if(name == "FFT Direction")
   {
      std::vector<ossimString> filterNames;
      filterNames.push_back("Forward");
      filterNames.push_back("Inverse");
      ossimStringProperty* stringProp = new ossimStringProperty("FFT Direction",
								getDirectionTypeAsString(),
								false,
								filterNames);
      stringProp->setCacheRefreshBit();

      return stringProp;
   }

   return ossimImageSourceFilter::getProperty(name);
}

void ossimFftw3Filter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if(!property) return;
   
   if(property->getName() == "FFT Direction")
   {
      if(m_tile.valid())
      {
         m_tile = NULL;
      }
      setDirectionType(property->valueToString());
   }
   else
   {
      ossimImageSourceFilter::setProperty(property);
   }
}

void ossimFftw3Filter::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   ossimImageSourceFilter::getPropertyNames(propertyNames);
   propertyNames.push_back("FFT Direction");
}


void ossimFftw3Filter::runFft(ossimRefPtr<ossimImageData>& input,
                              ossimRefPtr<ossimImageData>& output)
{
   ossim_uint32 w = input->getWidth();
   ossim_uint32 h = input->getHeight();
   ossim_uint32 n = w*h;
   double realVal, imagVal;
   double nullPix = getNullPixelValue();
   ossim_uint32 numBands = input->getNumberOfBands();
   if (m_xformDir == INVERSE)
      numBands /= 2;

   fftw_complex* indata  = fftw_alloc_complex(n);
   fftw_complex* outdata = fftw_alloc_complex(n);
   ossim_uint32 flags = 0;
   fftw_plan fp = fftw_plan_dft_2d(w, h, indata, outdata, (int) m_xformDir, flags);
   double* realBuf = 0;
   double* imagBuf = 0;

   // Transfer the image data to the FFTW data structure:
   ossim_uint32 cplx_band_idx = 0;
   for(ossim_uint32 band = 0; band < numBands;)
   {
      cplx_band_idx = 2*band;

      // Fill the FFTW3 complex buffer with pixel values representing real part and imaginary
      // part (if doing inverse) of input:
      if (m_xformDir == FORWARD)
      {
         realBuf = (double*) input->getBuf(band);
         imagBuf = 0;
      }
      else
      {
         realBuf = (double*) input->getBuf(cplx_band_idx);
         imagBuf = (double*) input->getBuf(cplx_band_idx+1);
      }

      ossim_uint32 i = 0;
      for(ossim_uint32 y = 0; y < h; ++y)
      {
         for(ossim_uint32 x = 0; x < w; ++x)
         {
            indata[i][0] = realBuf[i];
            if (m_xformDir == INVERSE)
               indata[i][1] = imagBuf[i];
            else
               indata[i][1] = 0;
            i++;
         }
      }

      // Compute the FFT (or inverse FFT):
      fftw_execute(fp);

      // Pull data out of FFTW3 structure and into OSSIM output tile:
      if (m_xformDir == INVERSE)
      {
         realBuf = (double*) output->getBuf(band);
         imagBuf = 0;
      }
      else
      {
         realBuf = (double*) output->getBuf(cplx_band_idx);
         imagBuf = (double*) output->getBuf(cplx_band_idx+1);
      }

      i = 0;
      for(ossim_uint32 y = 0; y < h; ++y)
      {
         for(ossim_uint32 x = 0; x < w; ++x)
         {
            realBuf[i] = outdata[i][0];
            if (m_xformDir == FORWARD)
               imagBuf[i] = outdata[i][1];
            i++;
         }
      }

      // Note that band represent input bands, these need t be invremented by 2 when performing
      // inverse transform:
      band++;
      if (m_xformDir == INVERSE)
         band++;
   }
   
   fftw_cleanup();
}

bool ossimFftw3Filter::loadState(const ossimKeywordlist& kwl,
                               const char* prefix)
{
   const char* direction = kwl.find(prefix, "fft_direction");
   if(direction)
   {
      setDirectionType(ossimString(direction));
   }
   
   return ossimImageSourceFilter::loadState(kwl, prefix);
}

bool ossimFftw3Filter::saveState(ossimKeywordlist& kwl,
                               const char* prefix)const
{
   kwl.add(prefix,
           "fft_direction",
           getDirectionTypeAsString(),
           true);
   
   return ossimImageSourceFilter::saveState(kwl, prefix);
}

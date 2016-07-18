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

RTTI_DEF1(ossimFftw3Filter, "ossimFftw3Filter", ossimFftFilter);

ossimFftw3Filter::ossimFftw3Filter(ossimObject* owner)
   :ossimFftFilter(owner)
{
}

ossimFftw3Filter::ossimFftw3Filter(ossimImageSource* inputSource)
   :ossimFftFilter(inputSource)
{
}

ossimFftw3Filter::ossimFftw3Filter(ossimObject* owner,
                               ossimImageSource* inputSource)
   :ossimFftFilter(owner, inputSource)
{
}

ossimFftw3Filter::~ossimFftw3Filter()
{
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
   if (theDirectionType == INVERSE)
      numBands /= 2;

   fftw_complex* indata  = fftw_alloc_complex(n);
   fftw_complex* outdata = fftw_alloc_complex(n);
   ossim_uint32 flags = 0;

   int dir = 1; // inverse
   if (theDirectionType == FORWARD)
      dir = -1;

   fftw_plan fp = fftw_plan_dft_2d(w, h, indata, outdata, dir, flags);
   double* realBuf = 0;
   double* imagBuf = 0;

   // Transfer the image data to the FFTW data structure:
   ossim_uint32 cplx_band_idx = 0;
   for(ossim_uint32 band = 0; band < numBands; ++band)
   {
      cplx_band_idx = 2*band;

      // Fill the FFTW3 complex buffer with pixel values representing real part and imaginary
      // part (if doing inverse) of input:
      if (theDirectionType == FORWARD)
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
      int sign = 1.0;
      for(ossim_uint32 y = 0; y < h; ++y)
      {
         for(ossim_uint32 x = 0; x < w; ++x)
         {
            if (theDirectionType == FORWARD)
            {
               indata[i][0] = sign*realBuf[i];
               indata[i][1] = 0;
            }
            else
            {
               indata[i][0] = sign*realBuf[i];
               indata[i][1] = sign*imagBuf[i];
            }
            i++;

            //sign *= -1;
         }
         //sign *= -1;
      }

      // Compute the FFT (or inverse FFT):
      fftw_execute(fp);

      // Pull data out of FFTW3 structure and into OSSIM output tile:
      if (theDirectionType == FORWARD)
      {
         realBuf = (double*) output->getBuf(cplx_band_idx);
         imagBuf = (double*) output->getBuf(cplx_band_idx+1);
      }
      else
      {
         realBuf = (double*) output->getBuf(band);
         imagBuf = 0;
      }

      i = 0;
      for(ossim_uint32 y = 0; y < h; ++y)
      {
         for(ossim_uint32 x = 0; x < w; ++x)
         {
            if (theDirectionType == FORWARD)
            {
               realBuf[i] = outdata[i][0] / (double)n;
               imagBuf[i] = outdata[i][1] / (double)n;
            }
            else
            {
               realBuf[i] = outdata[i][0];
            }
            i++;
         }
      }

      band++;
   }
   
   fftw_cleanup();
}


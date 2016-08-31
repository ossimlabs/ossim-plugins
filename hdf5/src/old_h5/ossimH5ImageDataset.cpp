//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: OSSIM HDF5 Image DataSet.
//
//----------------------------------------------------------------------------
// $Id

#include "ossimH5ImageDataset.h"
#include "ossimH5Util.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimScalarTypeLut.h>

//---
// This includes everything!  Note the H5 includes are order dependent; hence,
// the mongo include.
//---
#include <hdf5.h>
#include <H5Cpp.h>

#include <iostream>
using namespace std;

ossimH5ImageDataset::ossimH5ImageDataset()
   :
   m_dataset(0),
   m_datasetName(),
   m_scalar(OSSIM_SCALAR_UNKNOWN),
   m_bands(1),
   m_lines(0),
   m_samples(0),
   m_validRect(),
   m_endian(0)
{   
}

ossimH5ImageDataset::ossimH5ImageDataset( const ossimH5ImageDataset& obj )
   :
   m_dataset( ( (obj.m_dataset)?new H5::DataSet(*(obj.m_dataset)):0 ) ),
   m_datasetName(obj.m_datasetName),
   m_scalar(obj.m_scalar),
   m_bands(obj.m_bands),
   m_lines(obj.m_lines),
   m_samples(obj.m_samples),
   m_validRect(obj.m_validRect),
   m_endian( obj.m_endian ? new ossimEndian() : 0 )
{
   if ( obj.m_dataset )
   {
      m_dataset = new H5::DataSet( *(obj.m_dataset) );
   }
}

ossimH5ImageDataset::~ossimH5ImageDataset()
{
   close();
}

const ossimH5ImageDataset& ossimH5ImageDataset::operator=( const ossimH5ImageDataset& rhs )
{
   if ( this != &rhs )
   {
      if ( m_dataset )
      {
         delete m_dataset;
      }
      if ( rhs.m_dataset )
      {
         m_dataset = new H5::DataSet( *(rhs.m_dataset) );
      }
      else
      {
         m_dataset = 0;
      }
      m_datasetName = rhs.m_datasetName;
      m_scalar      = rhs.m_scalar;
      m_bands       = rhs.m_bands;
      m_lines       = rhs.m_lines;
      m_samples     = rhs.m_samples;
      m_validRect   = rhs.m_validRect;
      m_endian      = ( rhs.m_endian ? new ossimEndian() : 0 );
   }
   return *this;
}

bool ossimH5ImageDataset::initialize( const H5::DataSet& dataset,
                                      const std::string& datasetName )
{
   bool result = false;
   
   // Clear old stuff.
   close();
   
   m_dataset = new H5::DataSet( dataset );
   m_datasetName = datasetName;

   if ( ossim_hdf5::getValidBoundingRect( *m_dataset, m_datasetName, m_validRect ) )
   {
      // Get the extents:
      std::vector<ossim_uint32> extents;
      ossim_hdf5::getExtents( m_dataset, extents );
      if ( extents.size() >= 2 )
      {
         m_samples = extents[1];
         m_lines   = extents[0];
         if ( extents.size() >= 3 )
         {
            m_bands = extents[2];
         }
         else
         {
            m_bands = 1;
         }
         
         // Scalar type:
         m_scalar = ossim_hdf5::getScalarType( m_dataset );
         
         if ( m_scalar != OSSIM_SCALAR_UNKNOWN )
         {
            result = true;
            
            // Byte swapping logic:
            if ( ( ossim::scalarSizeInBytes( m_scalar ) > 1 ) &&
                 ( ossim::byteOrder() != ossim_hdf5::getByteOrder( m_dataset ) ) )
            {
               m_endian = new ossimEndian();
            }
         }
      }
   }

   if ( !result )
   {
      // One dimensional dataset:
      close();
   }

   return result;
   
} // End: ossimH5ImageDataset::initialize

void ossimH5ImageDataset::close()
{
   if ( m_dataset )
   {
      m_dataset->close();
      delete m_dataset;
      m_dataset = 0;
   }
   if ( m_endian )
   {
      delete m_endian;
      m_endian = 0;
   }
}

const H5::DataSet* ossimH5ImageDataset::getDataset() const
{
   return m_dataset;
}

H5::DataSet* ossimH5ImageDataset::getDataset()
{
   return m_dataset;
}

const std::string& ossimH5ImageDataset::getName() const
{
   return m_datasetName;
}

ossimScalarType ossimH5ImageDataset::getScalarType() const
{
   return m_scalar;
}

ossim_uint32 ossimH5ImageDataset::getNumberOfBands() const
{
   return m_bands;
}

ossim_uint32 ossimH5ImageDataset::getNumberOfLines() const
{
   return m_validRect.height();
}

ossim_uint32 ossimH5ImageDataset::getNumberOfSamples() const
{
   return m_validRect.width();
}

bool ossimH5ImageDataset::getSwapFlag() const
{
   return (m_endian ? true: false);
}

const ossimIpt& ossimH5ImageDataset::getSubImageOffset() const
{
   return m_validRect.ul();
}

const ossimIrect& ossimH5ImageDataset::getValidImageRect() const
{
   return m_validRect;
}

void ossimH5ImageDataset::getTileBuf(void* buffer, const ossimIrect& rect, ossim_uint32 band)
{
   static const char MODULE[] = "ossimH5ImageDataset::getTileBuf";

   if ( m_dataset )
   {
      try
      {
         // Shift rectangle by the sub image offse (if any) from the m_validRect.
         ossimIrect irect = rect + m_validRect.ul();
         
         //--
         // Turn off the auto-printing when failure occurs so that we can
         // handle the errors appropriately
         //---
         // H5::Exception::dontPrint();

         // NOTE: rank == array dimensions in hdf5 documentation lingo.

         // Get dataspace of the dataset.
         H5::DataSpace imageDataSpace = m_dataset->getSpace();
      
         // Number of dimensions of the input dataspace.:
         const ossim_int32 IN_DIM_COUNT = imageDataSpace.getSimpleExtentNdims();

         // Native type:
         H5::DataType dataType = m_dataset->getDataType();
      
         std::vector<hsize_t> inputCount(IN_DIM_COUNT);
         std::vector<hsize_t> inputOffset(IN_DIM_COUNT);

         if ( IN_DIM_COUNT == 2 )
         {
            inputOffset[0] = irect.ul().y;
            inputOffset[1] = irect.ul().x;
         
            inputCount[0] = irect.height();
            inputCount[1] = irect.width();
         }
         else
         {
            inputOffset[0] = band;
            inputOffset[1] = irect.ul().y;
            inputOffset[2] = irect.ul().x;
         
            inputCount[0] = 1;
            inputCount[1] = irect.height();
            inputCount[2] = irect.width();
         }
      
         // Define hyperslab in the dataset; implicitly giving strike strike and block NULL.
         imageDataSpace.selectHyperslab( H5S_SELECT_SET,
                                         &inputCount.front(),
                                         &inputOffset.front() );
      
         // Output dataspace dimensions.
         const ossim_int32 OUT_DIM_COUNT = 3;
         std::vector<hsize_t> outputCount(OUT_DIM_COUNT);
         outputCount[0] = 1;             // single band
         outputCount[1] = irect.height(); // lines
         outputCount[2] = irect.width();  // samples

         // Output dataspace offset.
         std::vector<hsize_t> outputOffset(OUT_DIM_COUNT);
         outputOffset[0] = 0;
         outputOffset[1] = 0;
         outputOffset[2] = 0;
      
         // Output dataspace.
         H5::DataSpace bufferDataSpace( OUT_DIM_COUNT, &outputCount.front());
         bufferDataSpace.selectHyperslab( H5S_SELECT_SET,
                                          &outputCount.front(),
                                          &outputOffset.front() );
      
         // Read data from file into the buffer.
         m_dataset->read( buffer, dataType, bufferDataSpace, imageDataSpace );

         if ( m_endian )
         {
            // If the m_endian pointer is initialized(not zero) swap the bytes.
            m_endian->swap( m_scalar, buffer, irect.area() );
         }
      
         // Cleanup:
         bufferDataSpace.close();
         dataType.close();
         imageDataSpace.close();
         
         // memSpace.close();
         // dataType.close();
         // dataSpace.close();
      }
      catch( const H5::FileIException& error )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " caught H5::FileIException!" << std::endl;
         error.printError();
      }
   
      // catch failure caused by the DataSet operations
      catch( const H5::DataSetIException& error )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " caught H5::DataSetIException!" << std::endl;
         error.printError();
      }
   
      // catch failure caused by the DataSpace operations
      catch( const H5::DataSpaceIException& error )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " caught H5::DataSpaceIException!" << std::endl;
         error.printError();
      }
   
      // catch failure caused by the DataSpace operations
      catch( const H5::DataTypeIException& error )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " caught H5::DataTypeIException!" << std::endl;
         error.printError();
      }
      catch( ... )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " caught unknown exception !" << std::endl;
      }
      
   } // Matches: if ( m_dataset )
   
} // End: ossimH5ImageDataset::getTileBuf


std::ostream& ossimH5ImageDataset::print( std::ostream& out ) const
{
   out << "ossimH5ImageDataset: "
       << "\nH5::DataSet::id: " << (m_dataset?m_dataset->getId():0)
       << "\nname:            " << m_datasetName
       << "\nscalar:          " << ossimScalarTypeLut::instance()->getEntryString( m_scalar )
       << "\nbands:           " << m_bands
       << "\nlines:           " << m_lines
       << "\nsamples:         " << m_samples
       << "\nvalid rect:      " << m_validRect
       << "\nswap_flage:      " << (m_endian?"true":"false")
       << std::endl;
   return out;
}

std::ostream& operator<<( std::ostream& out, const ossimH5ImageDataset& obj )
{
   return obj.print( out );
}


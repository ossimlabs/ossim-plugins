//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: OSSIM HDF5 utility class.
//
//----------------------------------------------------------------------------
// $Id

#include "ossimH5Util.h"
#include "ossimH5Options.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/projection/ossimBilinearProjection.h>

//---
// This includes everything!  Note the H5 includes are order dependent; hence,
// the mongo include.
//---
#include <hdf5.h>
#include <H5Cpp.h>

#include <iomanip> /* tmp */
#include <iostream> /* tmp */
#include <ostream>
#include <iterator>
using namespace std;

static ossimTrace traceDebug("ossimH5Util:debug");

std::ostream& ossim_hdf5::print(H5::H5File* file, std::ostream& out)
{
   if ( file )
   {
      std::string groupName     = "/";
      std::string prefix        = "hdf5";
      ossim_uint32 recurseCount = 0;
      
      std::vector<std::string> datasetNames;


      getDatasetNames(file, datasetNames );

//std::cout << "DATASET NAMES ======= " << datasetNames.size() << "\n";
      ossim_uint32 datasetNameIdx = 0;
      ossimString dataSetNamesStr;
      for(;datasetNameIdx<datasetNames.size();++datasetNameIdx)
      {
         if(dataSetNamesStr.empty())
         {
            dataSetNamesStr = datasetNames[datasetNameIdx];
         }
         else
         {
            dataSetNamesStr = dataSetNamesStr + ", " + datasetNames[datasetNameIdx];
         }
      }
      if(!dataSetNamesStr.empty())
      {
         out << prefix << ".datasetnames: " << dataSetNamesStr << "\n";
      }

      ossim_hdf5::printIterative( file, groupName, prefix, recurseCount, out );
   }

   return out;
}

void ossim_hdf5::printIterative( H5::H5File* file,
                                 const std::string& groupName,
                                 const std::string& prefix,
                                 ossim_uint32& recursedCount,
                                 std::ostream& out )
{
   if ( file && groupName.size() )
   {
      ++recursedCount;

      H5::Group* group = new H5::Group( file->openGroup(groupName) );

      // Print attributes:
      const ossim_uint32 ATTRS_COUNT = group->getNumAttrs();
      for ( ossim_uint32 aIdx = 0; aIdx < ATTRS_COUNT; ++aIdx )
      {
         H5::Attribute attr( group->openAttribute( aIdx ) );
         ossim_hdf5::printAttribute( attr, prefix, out );
      }
      
      const hsize_t OBJ_COUNT = group->getNumObjs();
      for ( hsize_t i = 0; i < OBJ_COUNT; ++i )
      {
         std::string objName = group->getObjnameByIdx(i);

         if ( objName.size() )
         {
            char separator = '/';
            std::string combinedName;
            combine( groupName, objName, separator, combinedName );
            
            separator = '.';
            std::string combinedPrefix;
            combine( prefix, objName, separator, combinedPrefix );
            
            H5G_obj_t objType   = group->getObjTypeByIdx(i);

#if 0
            std::cout << "combinedName: " << combinedName
                      << "\ncombinedPrefix: " << combinedPrefix
                      << "\ngetObjnameByIdx[" << i << "]: " << objName
                      << "\ngetObjTypeByIdx[" << i << "]: " << objType
                      << std::endl;
#endif
            
            if ( objType == H5G_GROUP )
            {
               // Recursive call:
               if ( recursedCount < ossimH5Options::instance()->getMaxRecursionLevel())//ossim_hdf5::MAX_RECURSION_LEVEL )
               {
                  ossim_hdf5::printIterative(
                     file, combinedName, combinedPrefix, recursedCount, out );
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "ossim_hdf5::printIterative WARNING!"
                     << "\nMax iterations reached!" << std::endl;
               }
            }
            else if ( objType == H5G_DATASET )
            {
               printObject( file, combinedName, combinedPrefix, out );
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "ossim_hdf5::printIterative WARNING!"
                  << "\nUnhandled object type: " << objType << std::endl;
            }
         }
      }
      
      group->close();
      delete group;
      group = 0;
      --recursedCount;
      
   } // Matches: if ( file )
   
} // End: void ossim_hdf5::printIterative method.

void ossim_hdf5::printObject(  H5::H5File* file,
                               const std::string& objectName,
                               const std::string& prefix,
                               std::ostream& out )
{
#if 0
   std::cout << "printObject entered..."
             << "\nobjectName: " << objectName
             << "\nprefix: " << prefix
             << std::endl;
#endif
   
   H5::DataSet dataset = file->openDataSet( objectName );
   // Get the class of the datatype that is used by the dataset.
   H5T_class_t type_class = dataset.getTypeClass();
   out << prefix << ".class_type: "
       << ossim_hdf5::getDatatypeClassType( type_class ) << std::endl;

   const ossim_uint32 ATTRS_COUNT = dataset.getNumAttrs();
   for ( ossim_uint32 aIdx = 0; aIdx < ATTRS_COUNT; ++aIdx )
   {
      H5::Attribute attr = dataset.openAttribute( aIdx );
      ossim_hdf5::printAttribute( attr, prefix, out );
   }

   switch(type_class)
   {
      case H5T_COMPOUND:
      {
         ossim_hdf5::printCompound(dataset, prefix, out);
         break;
      }
      case H5T_ENUM:
      {
         H5::EnumType dataType = dataset.getEnumType();
         ossim_hdf5::printEnumType(dataset, dataType,
                                   prefix, out);
         break;
      }
      case H5T_REFERENCE:
      {
        out << prefix << ".class_type: "
            << ossim_hdf5::getDatatypeClassType( type_class ) << std::endl;

         break;
      }
      default:
      {
         //std::cout << "TYPE CLASS NOT HANDLED ===== " << type_class << std::endl;
         break;
      }
   }
   //std::cout << "NPOINTS === "
   // Extents:
   std::vector<ossim_uint32> extents;
   ossim_hdf5::getExtents( &dataset, extents );
   for ( ossim_uint32 i = 0; i < extents.size(); ++i )
   {
      ossimString os;
      std::string exStr = ".extent";
      exStr += os.toString(i).string();
      out << prefix << exStr << ": " << extents[i] << std::endl;
   }

   ossimScalarType scalar;
   switch(type_class)
   {
      case H5T_INTEGER:
      {
         H5::IntType dataType = dataset.getIntType();
         bool isSigned = dataType.getSign() == H5T_SGN_NONE ? false : true;

         scalar = ossim_hdf5::getScalarType( type_class, dataType.getSize(), isSigned);

         break;
      }
      case H5T_FLOAT:
      {
         H5::FloatType dataType = dataset.getFloatType();
         bool isSigned = true;

         scalar = ossim_hdf5::getScalarType( type_class, dataType.getSize(), isSigned);
         break;
      }
      default:
      {
         scalar = OSSIM_SCALAR_UNKNOWN;
      }
   } 
   //dataset.getId() );
   if ( scalar != OSSIM_SCALAR_UNKNOWN)
   {
      out << prefix << "." << ossimKeywordNames::SCALAR_TYPE_KW << ": "
          << ossimScalarTypeLut::instance()->getEntryString( scalar ) << std::endl;

      if ( ossim::scalarSizeInBytes( scalar ) > 1 )
      {
         ossimByteOrder byteOrder = ossim_hdf5::getByteOrder( &dataset );
         std::string byteOrderString = "little_endian";
         if ( byteOrder == OSSIM_BIG_ENDIAN )
         {
            byteOrderString = "big_endian";
         }
         out << prefix << "." <<ossimKeywordNames::BYTE_ORDER_KW << ": "
             << byteOrderString << std::endl;
      }
   }

#if 0
   // Attributes:
   int numberOfAttrs = dataset.getNumAttrs();
   cout << "numberOfAttrs: " << numberOfAttrs << endl;
   for ( ossim_int32 attrIdx = 0; attrIdx < numberOfAttrs; ++attrIdx )
   {
      H5::Attribute attribute = dataset.openAttribute( attrIdx );
      cout << "attribute.from class: " << attribute.fromClass() << endl;
   }
#endif
   dataset.close();
   
} // End: printObject

void ossim_hdf5::printEnumType(H5::DataSet& dataset, 
                               H5::EnumType& dataType,
                               const std::string& prefix,
                               std::ostream& out)
{
   ossim_int32 nEnumMembers = dataType.getNmembers();
   ossim_int32 enumMemberIdx = 0;
   ossim_int32 enumTypeSize = dataType.getSize(); 
   if(dataType.getSize() == 4)
   {
      out << prefix << ".class_type: H5T_ENUM\n";
      for(;enumMemberIdx<nEnumMembers;++enumMemberIdx)
      {
         try{
            ossim_int32 value = 0;

            dataType.getMemberValue(enumMemberIdx, &value);
            H5std_string name = dataType.nameOf(&value, 2048);
            out << prefix << "." << name << ": " <<value <<"\n"; 
         }
         catch(H5::Exception& e)
         {
         }
      }
   }
}

void ossim_hdf5::printIntType(H5::DataSet& dataset, 
                              H5::IntType& dataType,
                              const char* dataPtr,
                              const std::string& prefix,
                              std::ostream& out)
{
   ossim_uint32 dataTypeSize = dataType.getSize();
   ossimByteOrder order = getByteOrder(&dataType);
   ossimEndian endian;
   bool isSigned = dataType.getSign() == H5T_SGN_NONE ? false : true;
   ossimString valueStr;
   ossimScalarType scalarType = getScalarType(dataType.getClass(), dataTypeSize, isSigned);
   switch(scalarType)
   {
      case OSSIM_UINT8:
      {
         ossim_uint8 value = *reinterpret_cast<const ossim_uint8*>(dataPtr);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_SINT8:
      {
         ossim_int8 value = *reinterpret_cast<const ossim_int8*>(dataPtr);
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_UINT16:
      {
         ossim_uint16 value = *reinterpret_cast<const ossim_uint16*>(dataPtr);
         if(order!=ossim::byteOrder())
         {
            endian.swap(value);
         }
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_SINT16:
      {
         ossim_int16 value = *reinterpret_cast<const ossim_int16*>(dataPtr);
         if(order!=ossim::byteOrder())
         {
            endian.swap(value);
         }
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_UINT32:
      {
         ossim_uint32 value = *reinterpret_cast<const ossim_uint32*>(dataPtr);
         if(order!=ossim::byteOrder())
         {
            endian.swap(value);
         }
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_SINT32:
      {
         ossim_int32 value = *reinterpret_cast<const ossim_int32*>(dataPtr);
         if(order!=ossim::byteOrder())
         {
            endian.swap(value);
         }
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_UINT64:
      {
         ossim_uint64 value = *reinterpret_cast<const ossim_uint64*>(dataPtr);
         if(order!=ossim::byteOrder())
         {
            endian.swap(value);
         }
         valueStr = ossimString::toString(value);
         break;
      }
      case OSSIM_SINT64:
      {
         ossim_int64 value = *reinterpret_cast<const ossim_int64*>(dataPtr);
         if(order!=ossim::byteOrder())
         {
            endian.swap(value);
         }
         valueStr = ossimString::toString(value);
         break;
      }
      default:
      {
         valueStr = "<UNHANDLED SCALAR TYPE>";
         break;
      }
   }
   out << prefix<<": "<<valueStr<<"\n";
}

void ossim_hdf5::printFloatType(H5::DataSet& dataset, 
                              H5::FloatType& dataType,
                              const char* dataPtr,
                              const std::string& prefix,
                              std::ostream& out)
{
   ossim_uint32 floatSize = dataType.getSize();
   ossimByteOrder order = getByteOrder(&dataType);
   ossimEndian endian;

   if(floatSize == 4)
   {
      ossim_float32 value = *reinterpret_cast<const ossim_float32*>(dataPtr);
      if(order!=ossim::byteOrder())
      {
         endian.swap(value);
      }
      out << prefix<<": "<<ossimString::toString(value)<<"\n";
   }
   else if(floatSize == 8)
   {
      ossim_float64 value = *reinterpret_cast<const ossim_float64*>(dataPtr);
      if(order!=ossim::byteOrder())
      {
         endian.swap(value);
        // std::cout << "NOT THE SAME ORDER!!!!!!!!!!!!\n";
      }
      out << prefix<<": "<<ossimString::toString(value)<<"\n";
   }
}

void ossim_hdf5::printStrType(H5::DataSet& dataset, 
                                 H5::StrType& dataType,
                                 const char* dataPtr,
                                 const std::string& prefix,
                                 std::ostream& out)
{
   const char* startPtr = dataPtr;
   const char* endPtr   = dataPtr;
   const char* maxPtr   = dataPtr + dataType.getSize();
   while((endPtr != maxPtr)&&(*endPtr!='\0')) ++endPtr;
   ossimString value(startPtr, endPtr);
   out << prefix <<": "<<value<<"\n";
}

void ossim_hdf5::printArrayType(H5::DataSet& dataset, 
                                 H5::ArrayType& dataType,
                                 const char* dataPtr,
                                 const std::string& prefix,
                                 std::ostream& out)
{
   ossim_uint32 arrayNdims = dataType.getArrayNDims();
   ossimByteOrder order = getByteOrder(&dataType);
   ossimEndian endian;
   H5::DataType superType = dataType.getSuper();
   //out << prefix<<".class_type: " << ossim_hdf5::getDatatypeClassType( dataType.getClass() ) << "\n";
   if(arrayNdims)
   {
      std::vector<hsize_t> dims(arrayNdims);
      dataType.getArrayDims(&dims.front());
      if(dims.size() > 0)
      {
         std::stringstream dimOut;
         ossimString dimString;
         ossim_uint32 idx = 1;
         ossim_uint32 nArrayElements = dims[0];
         std::copy(dims.begin(), --dims.end(),
            std::ostream_iterator<hsize_t>(dimOut,","));
         for(;idx<dims.size();++idx)
         {
          nArrayElements*=dims[idx]; 
         }
         dimString = ossimString("(") + dimOut.str() + ossimString::toString(dims[dims.size()-1])+")";  
         out << prefix << ".dimensions: " << dimString << "\n";
         ossim_uint32 typeSize = superType.getSize();
         switch(superType.getClass())
         {
            case H5T_STRING:
            {
               out<<prefix<<".values: (";
               const char* startPtr = 0;
               const char* endPtr = 0;
               const char* mem = 0;
               H5T_str_t       pad;
               ossim_int32 strSize = 0;
               for(idx=0;idx<nArrayElements;++idx)
               {
                  mem = ((const char*)dataPtr) + (idx * typeSize);
                  if(superType.isVariableStr())
                  {
                     startPtr = *(char**) mem;
                     if(startPtr) strSize = std::strlen(startPtr);
                     else strSize = 0;
                  }
                  else
                  {
                     startPtr = mem;
                  }
                  if(startPtr)
                  {
                    ossim_uint32 charIdx = 0;
                    endPtr = startPtr;
                    for (; ((charIdx < strSize) && (*endPtr)); ++charIdx,++endPtr);

                    //std::cout << "WHAT????? " << ossimString(startPtr, endPtr)<<std::endl;
                  }
                  ossimString value = ossimString(startPtr, endPtr);
                  if(idx == 0)
                  {
                     out << "\""<<value<< "\"";
                  }
                  else 
                  {
                     out << ", \""<<value<< "\"";
                  }
               }
               //out<<prefix<<".array_type: H5T_STRING\n";
//               out<<prefix<<".values: <NOT SUPPORTED>";
               break;
            }
            case H5T_INTEGER:
            {
               //out<<prefix<<".array_type: H5T_INTEGER\n";
               out<<prefix<<".values: (";
               for(idx=0;idx<nArrayElements;++idx)
               {
                  if(typeSize == 2)
                  {
                     ossim_int16 value = *reinterpret_cast<const ossim_int16*>(dataPtr);
                     if(order!=ossim::byteOrder())
                     {
                        endian.swap(value);
                     }
                     if((idx + 1) <nArrayElements)
                     {
                        out << ossimString::toString(value) << ", ";
                     }
                     else
                     {
                        out << ossimString::toString(value);
                     }
                  }
                  else if(typeSize == 4)
                  {
                     ossim_int32 value = *reinterpret_cast<const ossim_int32*>(dataPtr);
                     if(order!=ossim::byteOrder())
                     {
                        endian.swap(value);
                     }
                     if((idx + 1) <nArrayElements)
                     {
                        out << ossimString::toString(value) << ", ";
                     }
                     else
                     {
                        out << ossimString::toString(value);
                     }
                  }
                  dataPtr+=typeSize;
               }
               break;
            }
            case H5T_FLOAT:
            {
               //out<<prefix<<".array_type: H5T_FLOAT\n";
               out<<prefix<<".values: (";
               for(idx=0;idx<nArrayElements;++idx)
               {
                  if(typeSize == 4)
                  {
                     ossim_float32 value = *reinterpret_cast<const ossim_float32*>(dataPtr);
                     if(order!=ossim::byteOrder())
                     {
                        endian.swap(value);
                     }
                     if((idx + 1) <nArrayElements)
                     {
                        out << ossimString::toString(value) << ", ";
                     }
                     else
                     {
                        out << ossimString::toString(value);
                     }
                  }
                  else if(typeSize == 8)
                  {
                     ossim_float64 value = *reinterpret_cast<const ossim_float64*>(dataPtr);
                     if(order!=ossim::byteOrder())
                     {
                        endian.swap(value);
                     }
                     if((idx + 1) <nArrayElements)
                     {
                        out << ossimString::toString(value) << ", ";
                     }
                     else
                     {
                        out << ossimString::toString(value);
                     }
                  }
                  dataPtr+=typeSize;
               }
               break;
            }
            default:
            {
               break;
            }
         }
         out<<")\n";

      }
   }
}

void ossim_hdf5::printCompound(H5::DataSet& dataset, 
                               const std::string& prefix,
                               std::ostream& out)
{
   H5::CompType compound(dataset);
   H5::DataSpace dataspace = dataset.getSpace();
   ossim_uint32 dimensions = dataspace.getSimpleExtentNdims();
   ossim_uint32 nElements   = dataspace.getSimpleExtentNpoints();
   ossim_int32 nMembers    = compound.getNmembers();
   ossim_uint64 size       = compound.getSize();
   ossim_int32 memberIdx   = 0;
   ossim_int32 elementIdx   = 0;
   H5::DataType compType = dataset.getDataType();
   std::vector<char> compData(size*nElements);
   dataset.read((void*)&compData.front(),compType);
   char* compDataPtr = &compData.front();

   if(dimensions!=1)
   {
      return;
   }

   for(;elementIdx<nElements;++elementIdx)
   {
      std::string elementPrefix = prefix;//+"."+"element"+ossimString::toString(elementIdx);
      //std::cout << "ELEMENTS: " << elements << std::endl;
      for(memberIdx=0;memberIdx < nMembers;++memberIdx)
      {
         H5::DataType dataType = compound.getMemberDataType(memberIdx);
         H5std_string memberName = compound.getMemberName(memberIdx);
         ossim_uint32 memberOffset = compound.getMemberOffset(memberIdx) ;
         std::string newPrefix = elementPrefix + "."+ memberName;
         switch(dataType.getClass())
         {
            case H5T_COMPOUND:
            {
              ossim_hdf5::printCompound(dataset, newPrefix, out);
              break;            
            }
            case H5T_INTEGER:
            {
               H5::IntType dataType = compound.getMemberIntType(memberIdx);
               ossim_hdf5::printIntType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               break;            
            }
            case H5T_FLOAT:
            {
               H5::FloatType dataType = compound.getMemberFloatType(memberIdx);
               ossim_hdf5::printFloatType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               break;            
            }
            case H5T_TIME:
            case H5T_STRING:
            {
               H5::StrType dataType = compound.getMemberStrType(memberIdx);
               ossim_hdf5::printStrType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               break;            
            }
            case H5T_BITFIELD:
            {
               out << newPrefix << ": <H5T_BITFIELD NOT HANDLED>\n"; 
               break;            
            }
            case H5T_OPAQUE:
            {
               out << newPrefix << ": <H5T_OPAQUE NOT HANDLED>\n"; 
               break;            
            }
            case H5T_REFERENCE:
            {
               out << newPrefix << ": <H5T_REFERENCE NOT HANDLED>\n"; 
               break;            
            }
            case H5T_ENUM:
            {
               H5::EnumType dataType = compound.getMemberEnumType(memberIdx);
               ossim_hdf5::printEnumType(dataset, dataType, newPrefix, out);
               break;            
            }
            case H5T_VLEN:
            {
               out << newPrefix << ": <H5T_VLEN NOT HANDLED>\n"; 
               break;            
            }
            case H5T_ARRAY:
            {
                H5::ArrayType dataType = compound.getMemberArrayType(memberIdx);
                ossim_hdf5::printArrayType(dataset, dataType, &compDataPtr[memberOffset], newPrefix, out);
               break;            
            }
            case H5T_NO_CLASS:
            default:
            {
               out<< newPrefix << ".class_type: " 
                  << ossim_hdf5::getDatatypeClassType( dataType.getClass() ) << "\n";
               out << newPrefix << ": <NOT HANDLED>\n"; 
               break;            
            }
         }
      }
      compDataPtr+=size;
   }
}


bool ossim_hdf5::getGroupAttributeValue( H5::H5File* file,
                                         const std::string& group,
                                         const std::string& key,
                                         std::string& value )
{
   static const char MODULE[] = "ossim_hdf5::getGroupAttributeValue";
   
   bool result = false;
   
   if (  file )
   {
      try // HDF5 library throws exceptions so wrap with try{}catch...
      {
         // Open the root group:
         H5::Group* h5Group = new H5::Group( file->openGroup( group ) );
         
         // Lookw for key:
         H5::Attribute attr      = h5Group->openAttribute( key );
         std::string   name      = attr.getName();
         H5::DataType  type      = attr.getDataType();
         H5T_class_t   typeClass = attr.getTypeClass();
         
         if ( ( name == key ) && ( typeClass == H5T_STRING ) )
         {
            attr.read( type, value );
            result = true;
         }

         // Cleanup:
         attr.close();
         h5Group->close();
         delete h5Group;
         h5Group = 0;
      }
      catch( const H5::Exception& e )
      {
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << " WARNING: Caught exception!\n"
               << e.getDetailMsg() << std::endl;
         }
      }
      catch( ... )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " WARNING: Caught unknown exception!" << std::endl;
      }      
   }

   return result;
   
} // End: ossim_hdf5::getGroupAttributeValue

bool ossim_hdf5::getDatasetAttributeValue( H5::H5File* file,
                                           const std::string& objectName,
                                           const std::string& key,
                                           std::string& value )
{
   static const char MODULE[] = "ossim_hdf5::getDatasetAttributeValue";

   bool result = false;
   
   if (  file )
   {
      try // HDF5 library throws exceptions so wrap with try{}catch...
      {
         // Open the dataset:
         H5::DataSet dataset = file->openDataSet( objectName );
         
         // Lookw for key:
         H5::Attribute attr = dataset.openAttribute( key );

         std::string  name = attr.getName();
         H5::DataType type = attr.getDataType();
         H5T_class_t  typeClass = attr.getTypeClass();
         
         if ( ( name == key ) && ( typeClass == H5T_STRING ) )
         {
            attr.read( type, value );
            result = true;
         }

         // Cleanup:
         attr.close();
         dataset.close();
      }
      catch( const H5::Exception& e )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " WARNING: Caught exception!\n"
            << e.getDetailMsg() << std::endl;
      }
      catch( ... )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " WARNING: Caught unknown exception!" << std::endl;
      }      
   }

   return result;
   
} // End: ossim_hdf5::getDatasetAttributeValue

void ossim_hdf5::printAttribute( const H5::Attribute& attr,
                                 const std::string& prefix,
                                 std::ostream& out )
{
   std::string  name      = attr.getName();
   H5::DataType type      = attr.getDataType();
   H5T_class_t  typeClass = attr.getTypeClass();
   size_t       size      = type.getSize();
   
   std::string  value; // Initialized below.

   if ( ( typeClass == H5T_INTEGER ) || ( typeClass == H5T_FLOAT ) )
   {
      bool isSigned = false;
      if ( typeClass == H5T_INTEGER )
      {
         H5::IntType intType = attr.getIntType();
         isSigned = intType.getSign() == H5T_SGN_NONE ? false : true;
      }

      ossimScalarType scalar = ossim_hdf5::getScalarType( typeClass, size, isSigned );
      ossimByteOrder order = ossim_hdf5::getByteOrder( &attr );
      ossimEndian* endian = 0;
      if ( ( size > 1 ) && ( order != ossim::byteOrder() ) )
      {
         endian = new ossimEndian(); // If set used as flag to byte swap.
      }
      
      if ( typeClass == H5T_INTEGER )
      {
         switch ( scalar )
         {
            case OSSIM_UINT8:
            {
               if ( size == 1 )
               {
                  ossim_uint8 i;
                  attr.read( type, (void*)&i );
                  value = ossimString::toString( ossim_int32(i) ).string();
               }
               break;
            }
            case OSSIM_SINT8:
            {
               if ( size == 1 )
               {
                  ossim_sint8 i;
                  attr.read( type, (void*)&i );
                  value = ossimString::toString( ossim_int32(i) ).string();
               }
               break;
            }
            case OSSIM_UINT16:            
            {
               if ( size == 2 )
               {
                  ossim_uint16 i;
                  attr.read( type, (void*)&i );
                  if ( endian )
                  {
                     endian->swap( i );
                  }  
                  value = ossimString::toString( i ).string();
               }
               break;
            }
            case OSSIM_SINT16:
            {
               if ( size == 2 )
               {
                  ossim_sint16 i;
                  attr.read( type, (void*)&i );
                  if ( endian )
                  {
                     endian->swap( i );
                  }
                  value = ossimString::toString( i ).string();
               }
               break;
            }
            case OSSIM_UINT32:        
            {
               if ( size == 4 )
               {
                  ossim_uint32 i;
                  attr.read( type, (void*)&i );
                  if ( endian )
                  {
                     endian->swap( i );
                  }  
                  value = ossimString::toString( i ).string();
               }
               break;
            }
            case OSSIM_SINT32:
            {
               if ( size == 4 )
               {
                  ossim_sint32 i;
                  attr.read( type, (void*)&i );
                  if ( endian )
                  {
                     endian->swap( i );
                  }
                  value = ossimString::toString( i ).string();
               }
               break;
            }
            case OSSIM_UINT64:        
            {
               if ( size == 8 )
               {
                  ossim_uint64 i;
                  attr.read( type, (void*)&i );
                  if ( endian )
                  {
                     endian->swap( i );
                  }  
                  value = ossimString::toString( i ).string();
               }
               break;
            }
            case OSSIM_SINT64:
            {
               if ( size == 8 )
               {
                  ossim_sint32 i;
                  attr.read( type, (void*)&i );
                  if ( endian )
                  {
                     endian->swap( i );
                  }
                  value = ossimString::toString( i ).string();
               }
               break;
            }
            default:
               break;
         }
      }
      else if ( typeClass == H5T_FLOAT )
      {
         if ( scalar == OSSIM_FLOAT32 )
         {
            if ( size == 4 )
            {
               ossim_float32 f;
               attr.read( type, (void*)&f );
               if ( endian )
               {
                  endian->swap( f );
               }
               value = ossimString::toString( f ).string();
            }
         }
         if ( scalar == OSSIM_FLOAT64 )
         {
            if ( size == 8 )
            {
               ossim_float64 f;
               attr.read( type, (void*)&f );
               if ( endian )
               {
                  endian->swap( f );
               }
               value = ossimString::toString( f ).string();
            }
         }
      }

      if ( endian )
      {
         delete endian;
         endian = 0;
      }
   }
   else if ( typeClass == H5T_STRING )
   {
      attr.read( type, value );
   }
   else
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimH5Util::printAttribute WARN: Unhandled type class: " << ossim_hdf5::getDatatypeClassType( typeClass ) << "\n"
            << std::endl;
      }
   }
   
   out << prefix << "." << name << ": " << value << std::endl;

   type.close();
   
} // End: ossim_hdf5::printAttribute

void ossim_hdf5::combine( const std::string& left,
                          const std::string& right,
                          char separator,
                          std::string& result )
{
   if ( left.size() && right.size() )
   {
      result = left;
      if ( ( left[ left.size()-1 ] != separator ) && ( right[0] != separator ) )
      {
         result.push_back(separator);
      }
      result += right;
   }
}

void ossim_hdf5::getDatasetNames(H5::H5File* file,
                                 std::vector<std::string>& datasetNames )
{
   datasetNames.clear();
   
   if ( file )
   {
      std::string groupName = "/";
      ossim_uint32 recurseCount = 0;
      ossim_hdf5::iterateGroupForDatasetNames( file, groupName, datasetNames, recurseCount );
#if 0
      std::vector<std::string>::const_iterator i = datasetNames.begin();
      while ( i != datasetNames.end() )
      {
         std::cout << "dataset: " << (*i) << std::endl;
         ++i;
      }
#endif
   }
}

std::string ossim_hdf5::getDatatypeClassType( ossim_int32 type )
{
   H5T_class_t classType = (H5T_class_t)type;
   
   std::string result;
   switch ( classType )
   {
      case H5T_INTEGER:
         result = "H5T_INTEGER";
         break;
      case H5T_FLOAT:
         result = "H5T_FLOAT";
         break;         
      case H5T_TIME:
         result = "H5T_TIME";
         break;
      case H5T_STRING:
         result = "H5T_STRING";
         break;
      case H5T_BITFIELD:
         result = "H5T_BITFIELD";
         break;
      case H5T_OPAQUE:
         result = "H5T_OPAQUE";
         break;         
      case H5T_COMPOUND:
         result = "H5T_COMPOUND";
         break;
      case H5T_REFERENCE:
         result = "H5T_REFERENCE";
         break;         
      case H5T_ENUM:
         result = "H5T_ENUM";
         break;
      case H5T_VLEN:
         result = "H5T_VLEN";
         break;         
      case H5T_ARRAY:
         result = "H5T_ARRAY";
         break;
      case H5T_NO_CLASS:
      default:
         result = "H5T_NO_CLASS";
         break;
   }
   return result;
}

bool ossim_hdf5::isLoadableAsImage( H5::H5File* file, const std::string& datasetName )
{
   bool result = false;

   // std::cout << "isLoadable entered..." << std::endl;
   if ( file && datasetName.size() )
   {
      if ( ossimH5Options::instance()->isDatasetRenderable(datasetName)) //isExcludedDataset( datasetName ) == false )
      {
         H5::DataSet dataset = file->openDataSet( datasetName );
         
         // Get the class of the datatype that is used by the dataset.
         H5T_class_t type_class = dataset.getTypeClass();
         // std::cout << "Class type: " << ossim_hdf5::getDatatypeClassType( type_class )
         // << std::endl;
         
         if ( ( type_class == H5T_INTEGER ) || ( type_class == H5T_FLOAT ) )
         {
            // Get dataspace of the dataset.
            // H5::DataSpace dataspace = dataset.getSpace();
            
            // Get the extents:
            std::vector<ossim_uint32> extents;
            ossim_hdf5::getExtents( &dataset, extents );
            
            if ( extents.size() >= 2 )
            {
               if ( ( extents[0] > 1 ) && ( extents[1] > 1 ) )
               {
                  // std::cout << "Accepting dataset: " << datasetName << std::endl;
                  result = true;
               }     
            }
            
         }
         
         dataset.close();
      }
   }
      
   // std::cout << "isLoadable exit status: " << (result?"true":"false") << std::endl;
   
   return result;
}

#if 0
bool ossim_hdf5::isExcludedDataset( const std::string& datasetName )
{
   bool result = false;
   
   ossimFilename f = datasetName;
   f = f.file();
#if 1
   if ( f != "Radiance" )
   {
      result = true;
   }
#endif
   
#if 0
   if ( ( f == "Latitude" )        ||
        ( f == "Longitude" )       ||
        ( f == "SCAttitude" )      ||
        ( f == "SCPosition" )      ||
        ( f == "SCVelocity" ) )
   {
      result = true;
   }
#endif
   
   return result;
}
#endif

void ossim_hdf5::iterateGroupForDatasetNames(  H5::H5File* file,
                                               const std::string& groupName,
                                               std::vector<std::string>& datasetNames,
                                               ossim_uint32& recursedCount )
{
   if ( file && groupName.size() )
   {
      ++recursedCount;
      
      // std::cout << "iterateGroup: " << groupName << std::endl;
      
      H5::Group* group = new H5::Group( file->openGroup(groupName) );
      
      const hsize_t OBJ_COUNT = group->getNumObjs();
      
      for ( hsize_t i = 0; i < OBJ_COUNT; ++i )
      {
         std::string objName = group->getObjnameByIdx(i);

         if ( objName.size() )
         {
            char separator = '/';
            std::string combinedName;
            combine( groupName, objName, separator, combinedName );
            
            H5G_obj_t objType   = group->getObjTypeByIdx(i);

#if 0
            std::cout << "combinedName: " << combinedName
                      << "\ngetObjnameByIdx[" << i << "]: " << objName
                      << "\ngetObjTypeByIdx[" << i << "]: " << objType
                      << std::endl;
#endif

            if ( objType == H5G_GROUP )
            {
               // Recursive call:
               if ( recursedCount < ossimH5Options::instance()->getMaxRecursionLevel())
               {
                  ossim_hdf5::iterateGroupForDatasetNames(
                     file, combinedName, datasetNames, recursedCount );
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "ossim_hdf5::iterateGroupForDatasetNames WARNING!"
                     << "\nMax iterations reached!" << std::endl;
               }
            }
            else if ( objType == H5G_DATASET )
            {
               datasetNames.push_back( combinedName );
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                     << "ossim_hdf5::iterateGroupForDatasetNames WARNING!"
                     << "\nUnhandled object type: " << objType << std::endl;
            }
         }
      }
      
      group->close();
      delete group;
      group = 0;
      --recursedCount;
      
   } // Matches: if ( file )
   
} // End: void ossim_hdf5::iterateGroupForDatasetNames

void ossim_hdf5::getExtents( const H5::DataSet* dataset,
                             std::vector<ossim_uint32>& extents )
{
   extents.clear();
   
   if ( dataset )
   {
      // Get dataspace of the dataset.
      H5::DataSpace dataspace = dataset->getSpace();
      
      // Number of dimensions:
      int ndims = dataspace.getSimpleExtentNdims();
      if ( ndims )
      {
         //hsize_t dims_out[ndims];
         std::vector<hsize_t> dims_out(ndims);
         dataspace.getSimpleExtentDims( &dims_out.front(), 0 );
         for ( ossim_int32 i = 0; i < ndims; ++i )
         {
            extents.push_back(static_cast<ossim_uint32>(dims_out[i]));
         }
      }

      dataspace.close();
   }
}

ossimScalarType ossim_hdf5::getScalarType( const H5::DataSet* dataset )
{
   // cout << "typeClass: " << typeClass << "\nid: " << id << endl;

   ossimScalarType scalar = OSSIM_SCALAR_UNKNOWN;

   if ( dataset )
   {
      ossim_int32 typeClass = dataset->getTypeClass();
      
      if ( ( typeClass == H5T_INTEGER ) || ( typeClass == H5T_FLOAT ) )
      {
         // hid_t mem_type_id = H5Dget_type(id);
         hid_t mem_type_id = H5Dget_type( dataset->getId() );
         
         // cout << "mem_type_id: " << mem_type_id << endl;
         
         if( mem_type_id > -1 )
         {
            hid_t native_type = H5Tget_native_type(mem_type_id, H5T_DIR_DEFAULT);
            // hid_t native_type = H5Tget_native_type(id, H5T_DIR_DEFAULT);
            
            if( H5Tequal(H5T_NATIVE_CHAR, native_type) )
            {
               scalar = OSSIM_SINT8;
            }
            else if ( H5Tequal( H5T_NATIVE_UCHAR, native_type) )
            {
               scalar = OSSIM_UINT8;
            }
            else if( H5Tequal( H5T_NATIVE_SHORT, native_type) ) 
            {
               scalar = OSSIM_SINT16;
            }
            else if(H5Tequal(H5T_NATIVE_USHORT, native_type)) 
            {
               scalar = OSSIM_UINT16;
            }
            else if(H5Tequal( H5T_NATIVE_INT, native_type)) 
            {
               scalar = OSSIM_SINT32;
            }
            else if(H5Tequal( H5T_NATIVE_UINT, native_type ) ) 
            {
               scalar = OSSIM_UINT32;
            }
            else if(H5Tequal( H5T_NATIVE_LONG, native_type)) 
            {
               scalar = OSSIM_SINT32;
            }
            else if(H5Tequal( H5T_NATIVE_ULONG, native_type))
            {
               scalar = OSSIM_UINT32;
            }
            else if(H5Tequal( H5T_NATIVE_LLONG, native_type)) 
            {
               scalar = OSSIM_SINT64;
            }
            else if(H5Tequal( H5T_NATIVE_ULLONG, native_type))
            {
               scalar = OSSIM_UINT64;
            }
            else if(H5Tequal( H5T_NATIVE_FLOAT, native_type)) 
            {
               scalar = OSSIM_FLOAT32;
            }
            else if(H5Tequal( H5T_NATIVE_DOUBLE, native_type)) 
            {
               scalar = OSSIM_FLOAT64;
            }
         }
         
      } // Matches: if ( ( typeClass == ...
      
   } // Matches: if ( dataset ){
   
   return scalar;
   
} // End: ossim_hdf5::getScalarType( const H5::DataSet* dataset )

ossimScalarType ossim_hdf5::getScalarType( ossim_int32 typeClass,
                                           size_t size,
                                           bool isSigned )
{
   ossimScalarType scalar = OSSIM_SCALAR_UNKNOWN;

   H5T_class_t h5tClassType = (H5T_class_t)typeClass;

   if ( h5tClassType == H5T_INTEGER )
   {
      if ( size == 1 )
      {
         if (!isSigned)
         {
            scalar = OSSIM_UINT8;
         }
         else
         {
            scalar = OSSIM_SINT8;
         }
      }
      else if ( size == 2 )
      {
         if (!isSigned)
         {
            scalar = OSSIM_UINT16;
         }
         else
         {
            scalar = OSSIM_SINT16;
         }
      }
      else if ( size == 4 )
      {
         if (!isSigned)
         {
            scalar = OSSIM_UINT32;
         }
         else
         {
            scalar = OSSIM_SINT32;
         }
      }
      else if ( size == 8 )
      {
         if (!isSigned)
         {
            scalar = OSSIM_UINT64;
         }
         else
         {
            scalar = OSSIM_SINT64;
         }
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "unhandled scalar size: " << size << endl;
      }
   }
   else if ( h5tClassType == H5T_FLOAT )
   {
      if ( size == 4 )
      {
         scalar = OSSIM_FLOAT32;
      }
      else
      {
         scalar = OSSIM_FLOAT64;
      }
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
            << "unhandled type class: " << h5tClassType << endl;
   }
   
   return scalar;
}

// commenting this out.  This is broke and we should use the one that takes the class_type, size, and isSigned 
// arguments.
#if 0
ossimScalarType ossim_hdf5::getScalarType( ossim_int32 id )
{
   std::cout << "ossim_hdf5::getScalarType: entered...................\n";
   ossimScalarType scalar = OSSIM_SCALAR_UNKNOWN;
   
   H5T_class_t type_class = H5Tget_class(id);
   size_t      size       = H5Tget_size(id);
   H5T_sign_t  sign       = H5Tget_sign(id);

   if ( type_class == H5T_INTEGER )
   {
      if ( size == 1 && sign == H5T_SGN_2)
      {
         scalar = OSSIM_SINT8;
      }
      else if ( size == 2 && sign == H5T_SGN_2)
      {
         scalar = OSSIM_SINT16;
      }
      else if ( size == 4 && sign == H5T_SGN_2)
      {
         scalar = OSSIM_SINT32;
      }
      else if ( size == 8 && sign == H5T_SGN_2)
      {
         scalar = OSSIM_SINT64;
      }
      else if ( size == 1 && sign == H5T_SGN_NONE)
      {
         scalar = OSSIM_UINT8;
      }
      else if ( size == 2 && sign == H5T_SGN_NONE)
      {
         scalar = OSSIM_UINT16;
      }
      else if ( size == 4 && sign == H5T_SGN_NONE)
      {
         scalar = OSSIM_UINT32;
      }
      else if ( size == 8 && sign == H5T_SGN_NONE)
      {
         scalar = OSSIM_UINT64;
      }
   }
   else if ( type_class == H5T_FLOAT )
   {
      if ( size == 4)
      {
         scalar = OSSIM_FLOAT32;
      }
      else if ( size == 8)
      {
         scalar = OSSIM_FLOAT64;
      }
   }
   
   return scalar;
}
#endif

ossimByteOrder ossim_hdf5::getByteOrder( const H5::DataType* dataType )
{
   ossimByteOrder byteOrder = ossim::byteOrder();

   if(dataType)
   {
      const H5::AtomType* atomType = dynamic_cast<const H5::AtomType*>(dataType);

      if(atomType)
      {
         H5T_order_t h5order = atomType->getOrder();
         
         if(h5order == H5T_ORDER_LE)
         {
            byteOrder = OSSIM_LITTLE_ENDIAN;
         }  
         else if(h5order == H5T_ORDER_BE)
         {
            byteOrder = OSSIM_BIG_ENDIAN;
         }
      }
   }   

   return byteOrder;
}

ossimByteOrder ossim_hdf5::getByteOrder( const H5::AbstractDs* obj )
{
   ossimByteOrder byteOrder = ossim::byteOrder();
   if ( obj )
   {
      // Get the class of the datatype that is used by the dataset.
      H5T_class_t typeClass = obj->getTypeClass();

      H5T_order_t order = H5T_ORDER_NONE;
      
      if ( typeClass == H5T_INTEGER )
      {
         H5::IntType intype = obj->getIntType();
         order = intype.getOrder();
      }
      else if ( typeClass == H5T_FLOAT )
      {
         H5::FloatType floatType = obj->getFloatType();
         order = floatType.getOrder(); 
      }
      
      if ( order == H5T_ORDER_LE )
      {
         byteOrder = OSSIM_LITTLE_ENDIAN;
      }
      else if ( order == H5T_ORDER_BE )
      {
         byteOrder = OSSIM_BIG_ENDIAN;
      }
   }
   return byteOrder;
}

bool ossim_hdf5::getValidBoundingRect( H5::DataSet& dataset,
                                       const std::string& name,
                                       ossimIrect& rect )
{
   bool result = false;
   H5::DataSpace imageDataspace = dataset.getSpace();
   const ossim_int32 IN_DIM_COUNT = imageDataspace.getSimpleExtentNdims();
         
   if ( IN_DIM_COUNT == 2 )
   {
      // Get the extents. Assuming dimensions are same for lat lon dataset. 
      std::vector<hsize_t> dimsOut(IN_DIM_COUNT);
      imageDataspace.getSimpleExtentDims( &dimsOut.front(), 0 );

      if ( dimsOut[0] && dimsOut[1] )
      {
         
         //---
         // Capture the rectangle:
         // dimsOut[0] is height, dimsOut[1] is width:
         //---
         rect = ossimIrect( 0, 0,
                            static_cast<ossim_int32>( dimsOut[1]-1 ),
                            static_cast<ossim_int32>( dimsOut[0]-1 ) );
         
         const ossim_int32 WIDTH  = rect.width();
               
         std::vector<hsize_t> inputCount(IN_DIM_COUNT);
         std::vector<hsize_t> inputOffset(IN_DIM_COUNT);
         
         inputOffset[0] = 0;
         inputOffset[1] = 0;
         
         inputCount[0] = 1;
         inputCount[1] = WIDTH;
         
         // Output dataspace dimensions.
         const ossim_int32 OUT_DIM_COUNT = 3;
         std::vector<hsize_t> outputCount(OUT_DIM_COUNT);
         outputCount[0] = 1;     // single band
         outputCount[1] = 1;     // single line
         outputCount[2] = WIDTH; // whole line
               
         // Output dataspace offset.
         std::vector<hsize_t> outputOffset(OUT_DIM_COUNT);
         outputOffset[0] = 0;
         outputOffset[1] = 0;
         outputOffset[2] = 0;
               
         ossimScalarType scalar = ossim_hdf5::getScalarType( &dataset );
         if ( scalar == OSSIM_FLOAT32 )
         {
            // See if we need to swap bytes:
            ossimEndian* endian = 0;
            if ( ( ossim::byteOrder() != ossim_hdf5::getByteOrder( &dataset ) ) )
            {
               endian = new ossimEndian();
            }

            // Native type:
            H5::DataType datatype = dataset.getDataType();
                  
            // Output dataspace always the same one line.
            H5::DataSpace bufferDataSpace( OUT_DIM_COUNT, &outputCount.front());
            bufferDataSpace.selectHyperslab( H5S_SELECT_SET,
                                             &outputCount.front(),
                                             &outputOffset.front() );

            //---
            // Dataset sample has NULL lines at the end so scan for valid rect.
            // Use "<= -999" for test as per NOAA as it seems the NULL value is
            // fuzzy.  e.g. -999.3.
            //---
            const ossim_float32 NULL_VALUE = -999.0;

            //---
            // VIIRS Radiance data has a -1.5e-9 in the first column.
            // Treat this as a null.
            //---
            const ossim_float32 NULL_VALUE2 = ( name == "/All_Data/VIIRS-DNB-SDR_All/Radiance" )
               ? -1.5e-9 : NULL_VALUE;
            const ossim_float32 TOLERANCE = 0.1e-9; // For ossim::almostEqual()

            // Hold one line:
            std::vector<ossim_float32> values( WIDTH );

            // Find the ul pixel:
            ossimIpt ulIpt = rect.ul();
            bool found = false;
                  
            // Line loop to find upper left pixel:
            while ( ulIpt.y <= rect.lr().y )
            {
               inputOffset[0] = static_cast<hsize_t>(ulIpt.y);
               imageDataspace.selectHyperslab( H5S_SELECT_SET,
                                               &inputCount.front(),
                                               &inputOffset.front() );
               
               // Read data from file into the buffer.
               dataset.read( (void*)&values.front(), datatype, bufferDataSpace, imageDataspace );
               
               if ( endian )
               {
                  // If the endian pointer is initialized(not zero) swap the bytes.
                  endian->swap( scalar, (void*)&values.front(), WIDTH );
               }
               
               // Sample loop:
               ulIpt.x = rect.ul().x;
               ossim_int32 index = 0;
               while ( ulIpt.x <= rect.lr().x )
               {
                  if ( !ossim::almostEqual(values[index], NULL_VALUE2, TOLERANCE) &&
                       ( values[index] > NULL_VALUE ) )
                  {
                     found = true; // Found valid pixel.
                     break;
                  }
                  ++ulIpt.x;
                  ++index;
                     
               } // End: sample loop
                     
               if ( found )
               {
                  break;
               }

               ++ulIpt.y;
                     
            } // End line loop to find ul pixel:

            // Find the lower right pixel:
            ossimIpt lrIpt = rect.lr();
            found = false;
                  
            // Line loop to find last pixel:
            while ( lrIpt.y >= rect.ul().y )
            {
               inputOffset[0] = static_cast<hsize_t>(lrIpt.y);
               imageDataspace.selectHyperslab( H5S_SELECT_SET,
                                               &inputCount.front(),
                                               &inputOffset.front() );
               
               // Read data from file into the buffer.
               dataset.read( (void*)&values.front(), datatype, bufferDataSpace, imageDataspace );

               if ( endian )
               {
                  // If the endian pointer is initialized(not zero) swap the bytes.
                  endian->swap( scalar, (void*)&values.front(), WIDTH );
               }
            
               // Sample loop:
               lrIpt.x = rect.lr().x;
               ossim_int32 index = WIDTH-1;
               
               while ( lrIpt.x >= rect.ul().x )
               {
                  if ( !ossim::almostEqual(values[index], NULL_VALUE2, TOLERANCE) &&
                       ( values[index] > NULL_VALUE ) )
                  {
                     found = true; // Found valid pixel.
                     break;
                  }
                  --lrIpt.x;
                  --index;
                     
               } // End: sample loop
                     
               if ( found )
               {
                  break;
               }

               --lrIpt.y;
                     
            } // End line loop to find lower right pixel.

            rect = ossimIrect( ulIpt, lrIpt );

            // Cleanup:
            if ( endian )
            {
               delete endian;
               endian = 0;
            }

            result = true;
            
         } 
         else // Matches: if ( scalar == OSSIM_FLOAT32 ){...}
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossim_hdf5::getBoundingRect WARNING!"
               << "\nUnhandled scalar type: "
               << ossimScalarTypeLut::instance()->getEntryString( scalar )
               << std::endl;
         }
               
      } // Matches: if ( dimsOut...
            
   } // Matches: if ( IN_DIM_COUNT == 2 )
         
   imageDataspace.close();

   return result;
   
} // End: ossim_hdf5::getBoundingRect(...)

bool ossim_hdf5::crossesDateline( H5::DataSet& dataset, const ossimIrect& validRect )
{
   bool result = false;

   H5::DataSpace dataspace = dataset.getSpace();
         
   // Number of dimensions of the input dataspace:
   const ossim_int32 DIM_COUNT = dataspace.getSimpleExtentNdims();
         
   if ( DIM_COUNT == 2 )
   {
      const ossim_uint32 ROWS = validRect.height();
      const ossim_uint32 COLS = validRect.width();
      
      // Get the extents. Assuming dimensions are same for lat lon dataset. 
      std::vector<hsize_t> dimsOut(DIM_COUNT);
      dataspace.getSimpleExtentDims( &dimsOut.front(), 0 );

      if ( (ROWS <= dimsOut[0]) && (COLS <= dimsOut[1]) )
      {
         std::vector<hsize_t> inputCount(DIM_COUNT);
         std::vector<hsize_t> inputOffset(DIM_COUNT);
         
         inputCount[0] = 1;    // row
         inputCount[1] = COLS; // col
         
         // Output dataspace dimensions.
         const ossim_int32 OUT_DIM_COUNT = 3;
         std::vector<hsize_t> outputCount(OUT_DIM_COUNT);
         outputCount[0] = 1; // single band
         outputCount[1] = 1; // single line
         outputCount[2] = COLS; // single sample
               
         // Output dataspace offset.
         std::vector<hsize_t> outputOffset(OUT_DIM_COUNT);
         outputOffset[0] = 0;
         outputOffset[1] = 0;
         outputOffset[2] = 0;
               
         ossimScalarType scalar = ossim_hdf5::getScalarType( &dataset );
         if ( scalar == OSSIM_FLOAT32 )
         {
            // See if we need to swap bytes:
            ossimEndian* endian = 0;
            if ( ( ossim::byteOrder() != ossim_hdf5::getByteOrder( &dataset ) ) )
            {
               endian = new ossimEndian();
            }

            // Native type:
            H5::DataType datatype = dataset.getDataType();
                  
            // Output dataspace always the same one line.
            H5::DataSpace bufferDataSpace( OUT_DIM_COUNT, &outputCount.front());
            bufferDataSpace.selectHyperslab( H5S_SELECT_SET,
                                             &outputCount.front(),
                                             &outputOffset.front() );

            //---
            // Dataset sample has NULL lines at the end so scan for valid rect.
            // Use "<= -999" for test as per NOAA as it seems the NULL value is
            // fuzzy.  e.g. -999.3.
            //---

            // Buffer to hold a line:
            std::vector<ossim_float32> lineBuffer(validRect.width());

            // Read the first line:
            inputOffset[0] = static_cast<hsize_t>(validRect.ul().y);
            inputOffset[1] = static_cast<hsize_t>(validRect.ul().x);
            dataspace.selectHyperslab( H5S_SELECT_SET,
                                       &inputCount.front(),
                                       &inputOffset.front() );
            dataset.read( &(lineBuffer.front()), datatype, bufferDataSpace, dataspace );

            if ( endian )
            {
               // If the endian pointer is initialized(not zero) swap the bytes.
               endian->swap( &(lineBuffer.front()), COLS );
            }

            // Test the first line:
            result = ossim_hdf5::crossesDateline( lineBuffer );

            if ( !result )
            {
               // Test the last line:
               inputOffset[0] = static_cast<hsize_t>(validRect.ll().y);
               inputOffset[1] = static_cast<hsize_t>(validRect.ll().x);
               dataspace.selectHyperslab( H5S_SELECT_SET,
                                          &inputCount.front(),
                                          &inputOffset.front() );
               dataset.read( &(lineBuffer.front()), datatype, bufferDataSpace, dataspace );

               result = ossim_hdf5::crossesDateline( lineBuffer );
            }

            if ( endian )
            {
               delete endian;
               endian = 0;
            }
         }
         else // Matches: if ( scalar == OSSIM_FLOAT32 ){...}
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossim_hdf5::crossesDateline WARNING!"
               << "\nUnhandled scalar type: "
               << ossimScalarTypeLut::instance()->getEntryString( scalar )
               << std::endl;
         }
         
      } // Matches: if ( dimsOut...
      
   } // Matches: if ( IN_DIM_COUNT == 2 )
   
   dataspace.close();
   
   return result;
   
} // End: ossim_hdf5::crossesDateline(...)

bool ossim_hdf5::crossesDateline( const std::vector<ossim_float32>& lineBuffer)
{
   bool result = false;

   ossim_int32 longitude = 0;
   bool found179 = false;
   bool found181 = false;

   for ( ossim_uint32 i = 0; i < lineBuffer.size(); ++i)
   {
      longitude = (ossim_int32)lineBuffer[i]; // Cast to integer.

      // look for 179 -> -179...
      if ( !found179 )
      {
         if ( longitude == 179 )
         {
            found179 = true;
            continue;
         }
      }
      else // found179 == true
      {
         if ( longitude == 178 )
         {
            break; // Going West, 179 -> 178
         }
         else if ( longitude == -179 )
         {
            result = true;
            break;
         }
      }
      
      // look for -179 -> 179...
      if ( !found181 )
      {
         if ( longitude == -179 )
         {
            found181 = true;
            continue;
         }
      }
      else // found181 == true
      {
         if ( longitude == -178 )
         {
            break; // Going East -179 -> -178
         }
         else if ( longitude == 179 )
         {
            result = true;
            break;
         }
      }
   }
   
   return result;
   
} // End: ossim_hdf5::crossesDateline( lineBuffer )

ossimRefPtr<ossimProjection> ossim_hdf5::getBilinearProjection(
   H5::DataSet& latDataSet, H5::DataSet& lonDataSet, const ossimIrect& validRect )
{
   ossimRefPtr<ossimProjection> proj = 0;

   // Get dataspace of the dataset.
   H5::DataSpace latDataSpace = latDataSet.getSpace();
   H5::DataSpace lonDataSpace = lonDataSet.getSpace();
         
   // Number of dimensions of the input dataspace:
   const ossim_int32 DIM_COUNT = latDataSpace.getSimpleExtentNdims();
         
   if ( DIM_COUNT == 2 )
   {
      // Get the extents. Assuming dimensions are same for lat lon dataset. 
      std::vector<hsize_t> dimsOut(DIM_COUNT);
      latDataSpace.getSimpleExtentDims( &dimsOut.front(), 0 );

      if ( dimsOut[0] && dimsOut[1] )
      {
         std::vector<hsize_t> inputCount(DIM_COUNT);
         std::vector<hsize_t> inputOffset(DIM_COUNT);
               
         inputOffset[0] = 0;
         inputOffset[1] = 0;
               
         inputCount[0] = 1;
         inputCount[1] = 1;
               
         // Output dataspace dimensions.
         const ossim_int32 OUT_DIM_COUNT = 3;
         std::vector<hsize_t> outputCount(OUT_DIM_COUNT);
         outputCount[0] = 1; // single band
         outputCount[1] = 1; // single line
         outputCount[2] = 1; // single sample
               
         // Output dataspace offset.
         std::vector<hsize_t> outputOffset(OUT_DIM_COUNT);
         outputOffset[0] = 0;
         outputOffset[1] = 0;
         outputOffset[2] = 0;
               
         ossimScalarType scalar = ossim_hdf5::getScalarType( &latDataSet );
         if ( scalar == OSSIM_FLOAT32 )
         {
            // See if we need to swap bytes:
            ossimEndian* endian = 0;
            if ( ( ossim::byteOrder() != ossim_hdf5::getByteOrder( &latDataSet ) ) )
            {
               endian = new ossimEndian();
            }

            // Native type:
            H5::DataType latDataType = latDataSet.getDataType();
            H5::DataType lonDataType = lonDataSet.getDataType();
                  
            std::vector<ossimDpt> ipts;
            std::vector<ossimGpt> gpts;
            ossimGpt gpt(0.0, 0.0, 0.0); // Assuming WGS84...
            ossim_float32 latValue = 0.0;
            ossim_float32 lonValue = 0.0;
                  
            // Only grab every 256th value.:
            const ossim_int32 GRID_SIZE = 256;
                  
            // Output dataspace always the same one pixel.
            H5::DataSpace bufferDataSpace( OUT_DIM_COUNT, &outputCount.front());
            bufferDataSpace.selectHyperslab( H5S_SELECT_SET,
                                             &outputCount.front(),
                                             &outputOffset.front() );

            //---
            // Dataset sample has NULL lines at the end so scan for valid rect.
            // Use "<= -999" for test as per NOAA as it seems the NULL value is
            // fuzzy.  e.g. -999.3.
            //---
            const ossim_float32 NULL_VALUE = -999.0;

            //---
            // Get the tie points within the valid rect:
            //---
            ossimDpt ipt = validRect.ul();
            while ( ipt.y <= validRect.lr().y )
            {
               inputOffset[0] = static_cast<hsize_t>(ipt.y);
               
               // Sample loop:
               ipt.x = validRect.ul().x;
               while ( ipt.x <= validRect.lr().x )
               {
                  inputOffset[1] = static_cast<hsize_t>(ipt.x);
                  
                  latDataSpace.selectHyperslab( H5S_SELECT_SET,
                                                &inputCount.front(),
                                                &inputOffset.front() );
                  lonDataSpace.selectHyperslab( H5S_SELECT_SET,
                                                &inputCount.front(),
                                                &inputOffset.front() );
                  
                  // Read data from file into the buffer.
                  latDataSet.read( &latValue, latDataType, bufferDataSpace, latDataSpace );
                  lonDataSet.read( &lonValue, lonDataType, bufferDataSpace, lonDataSpace );
                  
                  if ( endian )
                  {
                     // If the endian pointer is initialized(not zero) swap the bytes.
                     endian->swap( latValue );
                     endian->swap( lonValue );  
                  }
                  
                  if ( ( latValue > NULL_VALUE ) && ( lonValue > NULL_VALUE ) )
                  {
                     gpt.lat = latValue;
                     gpt.lon = lonValue;
                     gpts.push_back( gpt );
                     
                     // Add the image point subtracting the image offset.
                     ossimIpt shiftedIpt = ipt - validRect.ul();
                     ipts.push_back( shiftedIpt );
                  }

                  // Go to next point:
                  if ( ipt.x < validRect.lr().x )
                  {
                     ipt.x += GRID_SIZE;
                     if ( ipt.x > validRect.lr().x )
                     {
                        ipt.x = validRect.lr().x; // Clamp to last sample.
                     }
                  }
                  else
                  {  
                     break; // At the end:
                  }
                  
               } // End sample loop.

               if ( ipt.y < validRect.lr().y )
               {
                  ipt.y += GRID_SIZE;
                  if ( ipt.y > validRect.lr().y )
                  {
                     ipt.y = validRect.lr().y; // Clamp to last line.
                  }
               }
               else
               {  
                  break; // At the end:
               }
               
            } // End line loop.
                  
            if ( ipts.size() )
            {
               // Create the projection:
               ossimRefPtr<ossimBilinearProjection> bp = new ossimBilinearProjection();
                     
               // Add the tie points:
               bp->setTiePoints( ipts, gpts );
                     
               // Assign to output projection:
               proj = bp.get();
            }

            // Cleanup:
            if ( endian )
            {
               delete endian;
               endian = 0;
            }
         }
         else // Matches: if ( scalar == OSSIM_FLOAT32 ){...}
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossim_hdf5::getBilinearProjection WARNING!"
               << "\nUnhandled scalar type: "
               << ossimScalarTypeLut::instance()->getEntryString( scalar )
               << std::endl;
         }
               
      } // Matches: if ( dimsOut...
            
   } // Matches: if ( IN_DIM_COUNT == 2 )
         
   latDataSpace.close();
   lonDataSpace.close();
   
   return proj;
   
} // End: ossim_hdf5::getBilinearProjection()

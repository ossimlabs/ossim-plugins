//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su, Harsh Govind
//
// Description: OSSIM KmlSuperOverlay writer class declaration.
//
//----------------------------------------------------------------------------
// $Id: ossimKmlSuperOverlayWriter.h 2178 2011-02-17 18:38:30Z ming.su $

#ifndef ossimKmlSuperOverlayWriter_HEADER
#define ossimKmlSuperOverlayWriter_HEADER 1

//ossim includes
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>

//minizip includes
#include <minizip/zip.h>

//std includes
#include <iosfwd>
#include <vector>

class ossimKeywordlist;
class ossimMapProjection;

class ossimKmlSuperOverlayWriter : public ossimImageFileWriter
{
public:

   /** @brief default constructor */
   ossimKmlSuperOverlayWriter();

   /* @brief virtual destructor */
   virtual ~ossimKmlSuperOverlayWriter();

   /** @return "ossim_kmlsuperoverlay_writer" */
   virtual ossimString getShortName() const;

   /** @return "ossim kmlsuperoverlay writer" */
   virtual ossimString getLongName()  const;

   /** @return "ossimKmlSuperOverlayWriter" */
   virtual ossimString getClassName()    const;
   
   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * This writer has type "kml" or "kmz".
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

   /**
   * Returns a 3-letter extension from the image type descriptor 
   * (theOutputImageType) that can be used for image file extensions.
   *
   * @param imageType string representing image type.
   *
   * @return the 3-letter string extension.
   */
   virtual ossimString getExtension() const;

   bool hasImageType(const ossimString& imageType) const;

   /**
   * @brief Method to write the image to a stream.
   *
   * @return true on success, false on error.
   */
   virtual bool writeStream();

protected:
   /**
    * @brief Writes the file to disk or a stream.
    * @return true on success, false on error.
    */
   virtual bool writeFile(); 

   bool generateRootKml(ossimString filename, 
                       ossim_float64 north, 
                       ossim_float64 south, 
                       ossim_float64 east, 
                       ossim_float64 west, 
                       ossim_int32 tilesize);

   bool generateChildKml(ossimString filename, 
                         ossim_int32 zoom, 
                         ossim_int32 yloop,
                         ossim_int32 ix, 
                         ossim_int32 iy, 
                         ossim_int32 dxsize, 
                         ossim_int32 dysize, 
                         ossim_int32 xsize,
                         ossim_int32 ysize, 
                         ossim_int32 maxzoom, 
                         ossimRefPtr<ossimMapProjection> proj,
                         ossimString fileExt);

   void generateTile(ossimString filename, 
                     ossim_int32 ix, 
                     ossim_int32 iy, 
                     ossim_int32 dxsize, 
                     ossim_int32 dysize);

   bool zipWithMinizip(std::vector<ossimString> srcFiles, 
                       ossimString srcDirectory, 
                       ossimString targetFile);

   /**
    * @brief Sends a view interface visitor to the input connetion.  This will
    * find all view interfaces, set the view, and send a property event to
    * output so that the combiner, if present, can reinitialize.
    */
   void propagateViewChange();

   ossimRefPtr<ossimImageFileWriter> m_imageWriter;
   ossimRefPtr<ossimMapProjection>   m_mapProjection;
   bool m_isKmz;
   bool m_isPngFormat;

 TYPE_DATA
};

#endif

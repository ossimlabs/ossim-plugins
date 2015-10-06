
//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc.
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su, Harsh Govind
//
// Description:
//
// Contains class definition for the class "ossimKmlSuperOverlayReader".
// ossimKmlSuperOverlayReader is derived from ImageHandler which is derived from
// TileSource. It is for displaying the information from kml super overlay.
//
//*******************************************************************
//  $Id: ossimKmlSuperOverlayReader.h 2178 2011-02-17 18:38:30Z ming.su $

#ifndef ossimKmlSuperOverlayReader_HEADER
#define ossimKmlSuperOverlayReader_HEADER

//Ossim Includes
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimDrect.h>

class ossimProjection;
class ossimMapProjection;
class ossimXmlDocument;

class ossimKmlSuperOverlayReader : public ossimImageHandler
{

public:

   ossimKmlSuperOverlayReader();
   virtual ~ossimKmlSuperOverlayReader();

   virtual void close();

   /**
    *  @return Returns true on success, false on error.
    *
    *  @note This method relies on the data member ossimImageData::theImageFile
    *  being set.  Callers should do a "setFilename" prior to calling this
    *  method or use the ossimImageHandler::open that takes a file name and an
    *  entry index.
    */      
   virtual bool open();

   /*!
    *  Returns a null pointer for kml super overlay.
    *  Satisfies pure virtual from ImageHandler class.
    * 
    */
   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& tileRect,
                                               ossim_uint32 resLevel=0);

   /*!
    *  Returns the number 0 for bands for kml super overlay
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfInputBands() const;

   /*!
    *  Returns the number 0 for bands for kml super overlay
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfOutputBands() const;
  
   /*!
    *  Returns the number of bands in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 reduced_res_level = 0) const;

   /*!
    *  Returns the number of bands available from an image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 reduced_res_level = 0) const;

   /*!
    * Returns the zero based image rectangle for the reduced resolution data
    * set (rrds) passed in.  Note that rrds 0 is the highest resolution rrds.
    */
   virtual ossimIrect getImageRectangle(ossim_uint32 reduced_res_level = 0) const;

   /*!
    * Method to save the state of an object to a keyword list.
    * Return true if ok or false on error.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;

   /*!
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);

    /*!
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileWidth() const;

   /*!
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileHeight() const;   
   
   //! Returns the image geometry object associated with this tile source or NULL if non defined.
   //! The geometry contains full-to-local image transform as well as projection (image-to-world)
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry() const;

   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

   ossimScalarType getOutputScalarType() const;

   virtual bool isOpen()const;
   
   ossimMapProjection* createDefaultProj(ossim_float64 north, ossim_float64 south, ossim_float64 east, ossim_float64 west);

private:

   bool getCoordinate(std::vector<ossimRefPtr<ossimXmlNode> > nodes,
                      ossim_float64& north,
                      ossim_float64& south,
                      ossim_float64& east,
                      ossim_float64& west);

   ossimRefPtr<ossimXmlNode> findLatLonNode(std::vector<ossimRefPtr<ossimXmlNode> >nodes);

   bool isKmlSuperOverlayFile(std::vector<ossimRefPtr<ossimXmlNode> > nodes);

   bool getTopLevelKmlFileInfo();

   ossimRefPtr<ossimXmlDocument>       m_xmlDocument;
   ossimRefPtr<ossimImageGeometry>     theImageGeometry;
   ossimDrect                          theImageBound;

TYPE_DATA
};

#endif

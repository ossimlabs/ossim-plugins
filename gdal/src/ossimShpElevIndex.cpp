#include <ossimShpElevIndex.h>


ossimShpElevIndex::ossimShpElevIndex( std::string shpFileName ) : poMemDS( 0 ) 
{
    GDALDriver *poMemDriver = GetGDALDriverManager()->GetDriverByName( "MEMORY" );

    poMemDS = poMemDriver->Create( "memData", 0, 0, 0, GDT_Unknown, NULL );
    poMemDS->Open( "memData", 1 );

    GDALDataset *poShpDS = (GDALDataset*) GDALOpenEx( shpFileName.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL );

    if( poShpDS == NULL )
    {
        std::cerr <<  "Open failed for: " << shpFileName << std::endl;
    }
    else 
    {
        OGRLayer  *poShpLayer = poShpDS->GetLayer( 0 );

        poMemDS->CopyLayer( poShpLayer, poShpLayer->GetName(), NULL );
        std::cout << poMemDS->GetLayer(0)->GetFeatureCount() << std::endl;
        GDALClose( poShpDS );
    }
}

ossimShpElevIndex::~ossimShpElevIndex() 
{
    GDALClose( poMemDS );
}

std::string ossimShpElevIndex::searchShapefile(double lon, double lat) const
{
   std::string filename; 

   ossimShpElevIndex::myMutex.lock();
   OGRLayer  *poLayer = poMemDS->GetLayer( 0 );
   OGRFeature *poFeature;
   OGRPoint* ogrPoint = new OGRPoint( lon, lat );

   poLayer->ResetReading();
   poLayer->SetSpatialFilter( ogrPoint );

   while( ( poFeature = poLayer->GetNextFeature() ) != NULL )
   {
      filename = poFeature->GetFieldAsString( poFeature->GetFieldIndex( "filename" ) );
      OGRFeature::DestroyFeature( poFeature );
   }

   delete ogrPoint;
   ossimShpElevIndex::myMutex.unlock();

   return filename;
}

std::atomic<ossimShpElevIndex*> ossimShpElevIndex::instance;
std::mutex ossimShpElevIndex::myMutex;

void ossimShpElevIndex::getBoundingRect(ossimGrect& rect) const
{
   // The bounding rect is the North up rectangle.  So if the underlying image projection is not
   // a geographic projection and there is a rotation this will include null coverage area.
   rect.makeNan();

   ossimShpElevIndex::myMutex.lock();
   OGRLayer  *poLayer = poMemDS->GetLayer( 0 );
   OGREnvelope *poEnvelope = new OGREnvelope();
   OGRErr status = poLayer->GetExtent( poEnvelope );   
   ossimShpElevIndex::myMutex.unlock();

   ossimGrect newRect( poEnvelope->MaxY, poEnvelope->MinX, poEnvelope->MinY, poEnvelope->MaxX );

   rect.expandToInclude(newRect);

    delete poEnvelope;

}

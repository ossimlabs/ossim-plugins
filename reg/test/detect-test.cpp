/*****************************************************************************
 *                                                                            *
 *                                 O S S I M                                  *
 *            Open Source, Geospatial Image Processing Project                *
 *          License: MIT, see LICENSE at the top-level directory              *
 *                                                                            *
 *****************************************************************************/

// This test code was adapted from the opencv sample "matchmethod_orb_akaze_brisk"

#include <opencv/cv.hpp>
#include <vector>
#include <iostream>
#include <string>

using namespace std;
using namespace cv;

static void help(const char* appname)
{
   cout <<"\nThis program demonstrates how to detect and compute ORB BRISK and AKAZE descriptors \n"
         "\nUsage: "<<appname<<" <image> \n"
         "\nPress a key on active image window to proceed to the following descriptor.\n"
         <<endl;
}

int main(int argc, char *argv[])
{
   vector<String> typeDesc;
   String fileName;

   if (argc < 2)
   {
      help(argv[0]);
      return 0;
   }

   // This descriptor are going to be detect and compute
   typeDesc.push_back("AKAZE-DESCRIPTOR_KAZE_UPRIGHT"); // see http://docs.opencv.org/trunk/d8/d30/classcv_1_1AKAZE.html
   typeDesc.push_back("AKAZE");    // see http://docs.opencv.org/trunk/d8/d30/classcv_1_1AKAZE.html
   typeDesc.push_back("ORB");      // see http://docs.opencv.org/trunk/de/dbf/classcv_1_1BRISK.html
   typeDesc.push_back("BRISK");    // see http://docs.opencv.org/trunk/db/d95/classcv_1_1ORB.html
   typeDesc.push_back("CORNER");    // see http://docs.opencv.org/trunk/db/d95/classcv_1_1ORB.html

   const String keys =  "{@image1 | | Reference image   } {help h  | | }";
   CommandLineParser parser(argc, argv, keys);
   if (parser.has("help"))
   {
      help(argv[0]);
      return 0;
   }
   fileName = parser.get<string>((int) 0);

   try
   {
      Mat img1 = imread(fileName, IMREAD_GRAYSCALE);
      if (img1.rows * img1.cols <= 0)
      {
         cout << "Image " << fileName << " is empty or cannot be found\n";
         return (0);
      }

      vector<double> desMethCmp;
      Ptr<Feature2D> b;

      // Descriptor loop
      vector<String>::iterator itDesc;
      for (itDesc = typeDesc.begin(); itDesc != typeDesc.end(); ++itDesc)
      {
         // keypoint  for img1
         vector<KeyPoint> keypoints;

         // Descriptor for img1
         if (*itDesc == "AKAZE-DESCRIPTOR_KAZE_UPRIGHT")
            b = AKAZE::create(AKAZE::DESCRIPTOR_KAZE_UPRIGHT);
         else if (*itDesc == "AKAZE")
            b = AKAZE::create();
         else if (*itDesc == "ORB")
            b = ORB::create();
         else if (*itDesc == "BRISK")
            b = BRISK::create();
         else if (*itDesc == "CORNER")
            b.release();
         else
         {
            cout << "\nUnknown descriptor type requested: <"<<*itDesc<<">\n"<<endl;
            return 0;
         }

         Mat descImg1;
         if (b)
         {
            // We can detect keypoint with detect method and then compute their descriptors:
            b->detect(img1, keypoints);
            b->compute(img1, keypoints, descImg1);

         }
         else
         {
            // Next demo goodFaturesToTrack:
            vector<Point2f> features;
            int maxCorners = 1000;
            double qualityLevel = 0.001;
            double minDistance = 5.0;
            goodFeaturesToTrack(img1, features, maxCorners, qualityLevel, minDistance);
            KeyPoint::convert(features, keypoints, 10.0);
         }

         // Draw:
         drawKeypoints(img1, keypoints, descImg1, Scalar::all(-1),
                       DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
         namedWindow(*itDesc, WINDOW_AUTOSIZE);
         imshow(*itDesc, descImg1);
      }
      waitKey();
   }
   catch (Exception& e)
   {
      cout << "\nHit exception: "<<e.msg << endl;
      return 0;
   }
   return 0;
}

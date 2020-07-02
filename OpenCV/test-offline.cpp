#include <stdio.h>
#include <string>
// #include "xiApiPlusOcv.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <chrono>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include </home/terrin/support/cpp_support/include/real_time_graph.h>
#include <opencv/cv.hpp>
#include <math.h>
#include <graphics.h>

using namespace cv;
using namespace std;
using namespace std::chrono;
using namespace boost::filesystem;

// display data-type of the image
string type2str(int type) 
{
  string r;
  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);
  switch ( depth ) 
  {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }
  r += "C";
  r += (chans+'0');
  return r;
}

int main(int argc, char* argv[])
{
    namedWindow( "image_analysis", WINDOW_NORMAL );
    resizeWindow("image_analysis", 600,200);
    
//     char* trackbar_type = "Image type: \n 0: Binary \n 1: Binary Inverted \n 2: Truncate \n 3: To Zero \n 4: To Zero Inverted";
//     int image_type = 0;
//     int const max_image_type = 4;
//     createTrackbar( trackbar_type,"image_analysis", &image_type,max_image_type );
    
    int const max_binary_value = 255;
    
    int l_threshold_value = 10;
    createTrackbar( "Lower_threshold","image_analysis", &l_threshold_value,max_binary_value );

    int u_threshold_value = 150;
    createTrackbar( "Upper_threshold","image_analysis", &u_threshold_value,max_binary_value );
    
    int canny_value = 0;
    int const max_canny_value = 100;
    createTrackbar( "Canny:", "image_analysis", &canny_value, max_canny_value );

    int erosion_size = 6;
    int const max_erosion_size = 10;
    createTrackbar( "Erode:", "image_analysis", &erosion_size, max_erosion_size );
    
    int area_limit =2000;
    int const max_area_limit = 50000;
    createTrackbar( "elipse area limit:", "image_analysis", &area_limit, max_area_limit );

    path path(argv[1]);
    directory_iterator end_itr;
    vector<string> names;

// 	ellp_area = translateImg(ellp_area,2,0);

        for ( directory_iterator itr( path );itr != end_itr;++itr )
            names.push_back(itr->path().c_str());
	    std::sort(names.begin(),names.end());

        high_resolution_clock::time_point t1 = high_resolution_clock::now();
        high_resolution_clock::time_point t2;
	

        Mat intial_image,image,thre_mask,res,threshold_img,canny_img,dilate_img,erode_img,element,image_w_elp;
        for ( auto name:names)
        {
            t2 = t1;
	    
            intial_image = imread(name,CV_8UC1);
// 	    string ty =  type2str( intial_image.type() );
// 	    printf("Image details: %s %dx%d \n", ty.c_str(), intial_image.cols, intial_image.rows );
	    
            Mat mask = Mat::zeros(Size(336,336),CV_8UC1);
            circle(mask, Point(mask.rows/2, mask.cols/2), mask.cols*(9.0/20.0) , Scalar(255,255,255), -1, -1, 0);
            intial_image.copyTo(image, mask);
	    
	    inRange(image, l_threshold_value, u_threshold_value, thre_mask);
	    bitwise_and(image,thre_mask,res);
	    
// 	    threshold( image, threshold_img, threshold_value,max_threshold_value,trackbar_type);
            
	    Canny(res, canny_img, canny_value, canny_value*3); 
	    element = getStructuringElement(MORPH_CROSS,Size(2 * erosion_size + 1, 2 * erosion_size + 1),Point(erosion_size, erosion_size) );
	    dilate(canny_img,dilate_img,element);
	    erode(dilate_img,erode_img,element);
	    
	    vector<vector<Point> > contours;
	    vector<Vec4i> hierarchy;
	    findContours( erode_img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	    
	    res.copyTo(image_w_elp);
	    
	    RotatedRect elp;
	    double ellipse_area, ellipse_width;
// 	    Mat drawing = Mat::zeros( canny_img.size(), CV_8UC3 );
	    
	    for( int i = 0; i< contours.size(); i++ )
	      {
// 	    	double area = contourArea(contours[i]); // range 0 - 20 
		if (contours[i].size() < 10) // less than 5 points cant form an ellipse
		  continue;
		
		elp = fitEllipse(contours[i]);
		
		if (elp.size.area() < area_limit)
		  continue;
		
		ellipse_width = elp.size.width;
		ellipse_area = elp.size.area();
// 		ellipse(drawing, elp.center, elp.size*0.5f, elp.angle, 0, 360, Scalar(0,255,255), 2, LINE_AA);
		ellipse(image_w_elp, elp.center, elp.size*0.5f, elp.angle, 0, 360, Scalar(255,255,255), 2, LINE_AA);
	      }
// 		imshow("int_image",int_image);
// 		imshow("image",image);
// 		imshow("thre_mask",thre_mask);
// 		imshow("res",res);
// 		imshow("canny",canny_img);
// 		imshow("dilate",dilate_img);
// 		imshow("erode",erode_img);
// 		imshow("ellipse",drawing);
	    imshow("image_w_elp",image_w_elp);
	    
	    real_time_graph(ellipse_area,"ellipse area");
 
	    t1 = high_resolution_clock::now(); // process end time
             cvWaitKey(1);
             cout << "Freq : "<< 1/duration_cast<duration<double>>(t1 - t2).count() <<" Hz" << endl;
        }
        printf("Done\n");
        cvWaitKey(500);

    return 0;
}


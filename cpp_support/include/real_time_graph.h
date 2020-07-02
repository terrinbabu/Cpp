#ifndef __REAL_TIME_GRAPH___H__
#define __REAL_TIME_GRAPH___H

#include <stdio.h>
#include <string>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <opencv/cv.hpp>

using namespace cv;
using namespace std;

Mat graph_area( 600, 1000, CV_8UC3);
int once_1 = 0, once_2 = 0;
int graph_max_value_1,graph_max_value_2;

void real_time_graph(double val_1, string val_1_name = "value 1",
		     double val_2 = 0, string val_2_name = "value 2",
		     string graph_name = "graph",
		     double max_value_1 = 0, double max_value_2 = 0)
{
    if (val_1 > 0.001)
    {
    if (max_value_1 == 0 && once_1 == 0)
    {
      graph_max_value_1 = val_1*2;
      once_1 = 1;
    }
    else if(max_value_1 != 0)
      graph_max_value_1 = max_value_1;
    
    circle(graph_area, Point(0,(graph_area.rows-(val_1*graph_area.rows/graph_max_value_1))), 1, Scalar( 255, 0, 0),2,8,0);
    
    if (val_2 != 0)
    {
      if (once_2 == 0)
      {
	graph_max_value_2 = val_2*2;
	once_2 = 1;
      }
      circle(graph_area, Point(0,(graph_area.rows-(val_2*graph_area.rows/graph_max_value_2))), 1, Scalar( 0, 255, 0),2,8,0);
    }
    
    cv::imshow(graph_name,graph_area);

    Mat min1_del( 20,40, CV_8UC3, Scalar(0,0,0)); // creating the space in graph for min value 1
    min1_del.copyTo(graph_area.rowRange(560,580).colRange(10,50));
    
    Mat max1_del( 20,100, CV_8UC3, Scalar(0,0,0)); // max value 1
    max1_del.copyTo(graph_area.rowRange(20,40).colRange(10,110));
    
    if (val_2 != 0)
    {
	Mat min2_del( 20,100, CV_8UC3, Scalar(0,0,0)); // min value 2
	min2_del.copyTo(graph_area.rowRange(560,580).colRange(890,990));
	
	Mat max2_del( 20,100, CV_8UC3, Scalar(0,0,0)); // max value 2
	max2_del.copyTo(graph_area.rowRange(20,40).colRange(890,990));
    }
    
    Mat legend_del( 60,200, CV_8UC3, Scalar(0,0,0)); // legend
    legend_del.copyTo(graph_area.rowRange(60,120).colRange(800,1000));
    
    Mat trans_mat = (Mat_<double>(2,3) << 1, 0, 2, 0, 1, 0); // translateImg
    warpAffine(graph_area,graph_area,trans_mat,graph_area.size());
    
    Mat min1( 20,40, CV_8UC3, Scalar(0,0,0)); // showing min value 1
    putText(min1, "0", Point(0,15), FONT_HERSHEY_COMPLEX_SMALL,1,Scalar( 255, 0, 0),1,CV_AA);
    min1.copyTo(graph_area.rowRange(560,580).colRange(10,50));
    
    Mat max1( 20,100, CV_8UC3, Scalar(0,0,0)); // max value 1
    putText(max1, to_string(graph_max_value_1), Point(0,15), FONT_HERSHEY_COMPLEX_SMALL,1,Scalar( 255, 0, 0),1,CV_AA);
    max1.copyTo(graph_area.rowRange(20,40).colRange(10,110));
    
    Mat legend( 60,200, CV_8UC3, Scalar(0,0,0)); // legend 1
    putText(legend, val_1_name, Point(0,20), FONT_HERSHEY_COMPLEX_SMALL,1,Scalar( 255, 0, 0),1,CV_AA);

    if (val_2 != 0) // value 2
    {
      Mat min2( 20,100, CV_8UC3, Scalar(0,0,0)); 
      putText(min2, "0", Point(0,15), FONT_HERSHEY_COMPLEX_SMALL,1,Scalar( 0, 255, 0),1,CV_AA);
      min2.copyTo(graph_area.rowRange(560,580).colRange(890,990));
      
      Mat max2( 20,100, CV_8UC3, Scalar(0,0,0)); // max value 2
      putText(max2, to_string(graph_max_value_2), Point(0,15), FONT_HERSHEY_COMPLEX_SMALL,1,Scalar( 0, 255, 0),1,CV_AA);
      max2.copyTo(graph_area.rowRange(20,40).colRange(890,990));
      
      putText(legend, val_2_name, Point(0,50), FONT_HERSHEY_COMPLEX_SMALL,1,Scalar( 0, 255, 0),1,CV_AA);
    }
    
    legend.copyTo(graph_area.rowRange(60,120).colRange(800,1000));
}}

#endif

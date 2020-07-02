#include <stdio.h>
#include "xiApiPlusOcv.hpp"
#include <chrono>
#include <iostream>

using namespace cv;
using namespace std;
using namespace std::chrono;

int main(int argc, char* argv[])
{
    try
    {
        // Sample for XIMEA OpenCV
        xiAPIplusCameraOcv cam;
        Mat cv_mat_image;
        // Retrieving a handle to the camera device
        printf("Opening first camera...\n");
        cam.OpenFirst();

        // Set exposure
        cam.SetExposureTime(100); //10000 us = 10 ms
        // Note: The default parameters of each camera might be different in different API versions
        std::cout <<"max framerate: " << cam.GetFrameRate_Maximum() << std::endl;	
        printf("Starting acquisition...\n");
        cam.StartAcquisition();

        XI_IMG_FORMAT format = cam.GetImageDataFormat();
        high_resolution_clock::time_point t1= high_resolution_clock::now();
        high_resolution_clock::time_point t2= high_resolution_clock::now();

        #define EXPECTED_IMAGES 4000
        for (int images=0;images < EXPECTED_IMAGES;images++)
        {
            cout << cv_mat_image.type() << endl;
            t2 = t1;
            cv_mat_image = cam.GetNextImageOcvMat();
            if (format == XI_RAW16 || format == XI_MONO16) 
                normalize(cv_mat_image, cv_mat_image, 0, 65536, NORM_MINMAX, -1, Mat()); // 0 - 65536, 16 bit unsigned integer range
            imshow("Image",cv_mat_image);
            t1 = high_resolution_clock::now();
            cvWaitKey(1);
            std::cout << 1.0/duration_cast<duration<double>>(t1 - t2).count() <<" HZ"  << std::endl;
        }

        cam.StopAcquisition();
        cam.Close();
        printf("Done\n");

        cvWaitKey(500);
        }
    catch(xiAPIplus_Exception& exp)
    {
        printf("Error:\n");
        exp.PrintError();

        cvWaitKey(2000);
        return -1;
    }
    return 0;
}


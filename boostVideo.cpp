//#define _CRT_SECURE_NO_WARNINGS 

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <fstream>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <string>  // string
#include <chrono>
#include <cstdlib>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic/atomic.hpp>

#include "UEyeOpenCV.hpp"
#include <ueye.h>


using namespace std;
using namespace cv;




	
int producerCount = 0   ;
const int RAW8    = 1   ;
const int BGR8    = 1   ;
int img_width     = 1936; 
int img_height    = 1216;
bool boostGain		;
double ActualExposure   ;	

											
stringstream TargetPath;
boost::atomic_int consumerCount(0);
boost::lockfree::spsc_queue<Mat, boost::lockfree::capacity<1024> > spscQueue;
boost::lockfree::spsc_queue<string, boost::lockfree::capacity<1024> > stampsQueue;
boost::atomic<bool> done(false);

int iterations      ; 
bool showOutputImage; 
bool auto_exposure  ;																
bool autoGain       ;
int masterGain      ; 
int redGain         ; 
int greenGain       ; 
int blueGain        ; 
double exposure_ms  ; 			
double FPS          ;

bool int2bool(int input){
	
	if(input == 1){
		return true;	
	}
	if(input == 0){
		return false;
	}
	cout << "Wronrg arguments." << endl;
}

stringstream getTimeStamp() {

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (now.time_since_epoch()) -
       	  	  std::chrono::duration_cast<std::chrono::seconds>      (now.time_since_epoch());

	std::stringstream currTimeStamp;
	std::stringstream folderName;	
	currTimeStamp << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H-%M-%S-") << ms.count() << ".bin";
	return currTimeStamp;

}

stringstream createTargetFolder(int exposureTime , int FPS) {
	
	string TargetFolderPath = "/home/odroid/Desktop/outputImage/";
	std::stringstream currTimeStamp;
	std::stringstream Path		   ;
	std::stringstream folderName   ;
	std::stringstream command      ;

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	currTimeStamp.str("");  //reset timestamp
	currTimeStamp.clear();  //reset timestamp
	currTimeStamp << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H-%M-%S");
	
	if(auto_exposure == 0)
	{
		folderName << FPS << "fps-" << exposureTime << "ms-" << currTimeStamp.str();
	}
	else
	{
		folderName << FPS << "fps-AutoExposure-" << currTimeStamp.str();
	}
	command << "mkdir -p " << TargetFolderPath << folderName.str() ;
	system(command.str().c_str());
	Path << TargetFolderPath <<	folderName.str();
	return Path;
}



void producer(void) {
															
	UeyeOpencvCam uEyeCamera(img_width, img_height);
	int SetExposureReturn = uEyeCamera.setExposure(auto_exposure, exposure_ms) ;
	double newFPS = uEyeCamera.setFPS(FPS);  
	ActualExposure = uEyeCamera.getExposure();
	int gainSet = uEyeCamera.setGain( autoGain, masterGain, redGain,greenGain, blueGain, boostGain);
    		                
			               
	cout << "Actual Exposure Time: " << ActualExposure << endl;
	cout << "Actual FPS: " << newFPS << endl;

	

	Mat rawInputImage;
	Mat rgbInputImage;
	Mat rgbInputImageSmall;
	std::stringstream currStamp;
	
	for (int i = 0; i != iterations; ++i) { 
		++producerCount;
		currStamp.str("");  //reset timestamp
		currStamp.clear();  //reset timestamp
		rawInputImage = uEyeCamera.getFrame();
		
		if (!rawInputImage.empty()){
			currStamp = getTimeStamp();
			while (!spscQueue.push(rawInputImage) || !stampsQueue.push(currStamp.str()))
			;
			if(showOutputImage == true){
				//resize & format converting:
				cvtColor(rawInputImage, rgbInputImage, CV_BayerBG2BGR );
				resize(rgbInputImage,rgbInputImageSmall , cv::Size(img_width/2,img_height/2));		
				imshow("Input Image", rgbInputImageSmall);
				waitKey(10);
			}
		}
		else {
			cout << "Skipped image" << endl;			
			waitKey(100);
			continue;		
		}
	}
	//uEyeCamera.~UeyeOpencvCam();
}


void consumer(void) {

	Mat currImage;
	int imageWidth;
	int imageHeight;
	string currStamp;
	
	
	while (!done) {
		while (spscQueue.pop(currImage) && stampsQueue.pop(currStamp)) {
			string currFileName = TargetPath.str() + "/" + currStamp;
			FILE* pFile = fopen( currFileName.c_str(), "wb");  
			if (pFile == NULL) {
				cout << "Can't write to file" << endl;
				continue;
			}
			imageWidth = currImage.cols;
			imageHeight = currImage.rows;
			fwrite(currImage.data, sizeof(unsigned char), imageWidth * imageHeight, pFile);
			fclose(pFile);
			++consumerCount;
		}
	}

	while (spscQueue.pop(currImage)&& stampsQueue.pop(currStamp)) {

		string currFileName = TargetPath.str() + "/" + currStamp;
		FILE* pFile = fopen( currFileName.c_str(), "wb");

		if (pFile == NULL) {
				cout << "Can't write to file" << endl;
				continue;
		} 
		imageWidth = currImage.cols;
		imageHeight = currImage.rows;
		fwrite(currImage.data, sizeof(unsigned char), imageWidth * imageHeight, pFile);
		fclose(pFile);
		++consumerCount;
	}
}




int main(int argc, char* argv[]) {

	iterations      = atof(argv[1])			 ; 
	showOutputImage = int2bool(atoi(argv[2])); 
	auto_exposure   = int2bool(atoi(argv[3]));														
	autoGain        = int2bool(atoi(argv[4]));
	masterGain      = atoi(argv[5])			 ; 
	redGain         = atoi(argv[6])          ; 
	greenGain       = atoi(argv[7])          ; 
	blueGain        = atoi(argv[8])          ; 
	exposure_ms     = atof(argv[9])          ; 	
	FPS             = atof(argv[10])         ;

	TargetPath = createTargetFolder(exposure_ms , FPS);
	
	cout << "boost::lockfree::queue is ";
	if (!spscQueue.is_lock_free()) cout << "not ";
	cout << "lockfree" << endl;

	cout << "Target Folder:" << TargetPath.str() << endl;
	
	auto producerThreadStart = std::chrono::high_resolution_clock::now();
	auto consumerThreadStart = std::chrono::high_resolution_clock::now();
	boost::thread producerThread(producer);
	boost::thread consumerThread(consumer);

	producerThread.join();
	auto producerThreadEnd = std::chrono::high_resolution_clock::now();
	auto producerThreadTime = producerThreadEnd - producerThreadStart;
	std::cout << "Producer Thread Time " << std::chrono::duration_cast<std::chrono::milliseconds>(producerThreadTime).count() << "ms to run.\n";

	done = true;

	consumerThread.join();
	auto consumerThreadEnd = std::chrono::high_resolution_clock::now();
	auto consumerThreadTime = consumerThreadEnd - consumerThreadStart;
	std::cout << "Consumer Thread Time " << std::chrono::duration_cast<std::chrono::milliseconds>(consumerThreadTime).count() << "ms to run.\n";

	cout << "produced " << producerCount << " objects." << endl;
	cout << "consumed " << consumerCount << " objects." << endl;

	cout << "Press Enter to Continue";
	cin.ignore();


}

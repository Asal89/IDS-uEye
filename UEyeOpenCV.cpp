//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        ueyeOpencv
// File:           UEyeOpenCV.cpp
// Description:    Wrapper class of UEye camera to support OpenCV Mat using the UEye SDK
// Author:         Wouter Langerak
// Notes:          For more functionalities use the SDK of UEye, the purpose of this project is to make it compatible with OpenCV Mat.
//
// License:        GNU GPL v3
//
// This file is part of ueyeOpencv.
//
// ueyeOpencv is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ueyeOpencv is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ueyeOpencv.  If not, see <http://www.gnu.org/licenses/>.
//******************************************************************************

#include "UEyeOpenCV.hpp"
#include <iostream>
#include <ueye.h>

using namespace cv;
using namespace std;


UeyeOpencvCam::UeyeOpencvCam(int wdth, int heigh) {

	width = wdth;
	height = heigh;
	using std::cout;
	using std::endl;
	mattie = cv::Mat(height, width, CV_8UC1);
	hCam = 0;
	char* ppcImgMem;
	int pid;
	INT nAOISupported = 0;
	double on = 1;
	//double empty;
	int retInt = IS_SUCCESS;
	

	retInt = is_InitCamera(&hCam, 0); //establishes the connection to camera
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}

	retInt = is_SetColorMode(hCam,IS_CM_SENSOR_RAW8);//sets the color mode to be used
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}


	retInt = is_ImageFormat(hCam, IMGFRMT_CMD_GET_ARBITRARY_AOI_SUPPORTED, (void*) &nAOISupported, sizeof(nAOISupported));
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}

	retInt = is_AllocImageMem(hCam, width, height, 8, &ppcImgMem, &pid);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}

	retInt = is_SetImageMem(hCam, ppcImgMem, pid);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}

	retInt = is_SetExternalTrigger (hCam,IS_SET_TRIGGER_OFF);  // trigger mode  IS_SET_TRIGGER_LO_HI IS_SET_TRIGGER_OFF
	if (retInt != IS_SUCCESS) {							
		throw UeyeOpenCVException(hCam, retInt);
	}

	retInt = is_EnableEvent(hCam, IS_SET_EVENT_FRAME);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}

	retInt = is_CaptureVideo(hCam, IS_WAIT);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}

	UINT pixelClockMHz = 118;
    retInt = is_PixelClock(hCam,IS_PIXELCLOCK_CMD_SET, (void*)&pixelClockMHz, sizeof(pixelClockMHz));
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}


}

UeyeOpencvCam::~UeyeOpencvCam() {
	int retInt = is_ExitCamera(hCam);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}
}

cv::Mat UeyeOpencvCam::getFrame() {
	getFrame(mattie);
	return mattie;
}

void UeyeOpencvCam::getFrame(cv::Mat& mat) {
	VOID* pMem;
	INT nRet = is_WaitEvent(hCam, IS_SET_EVENT_FRAME, 1000);
	if (nRet == IS_TIMED_OUT){
  		mat = Mat::zeros(width , height ,  CV_8UC1);
	}
	else if (nRet == IS_SUCCESS){
		int retInt = is_GetImageMem(hCam, &pMem);
		if (retInt != IS_SUCCESS) {
			throw UeyeOpenCVException(hCam, retInt);
		}
		mat = Mat(height, width, CV_8UC1, pMem);
	}



//	if (mat.cols == width && mat.rows == height && mat.depth() == 3) {
		//memcpy(mat.ptr(), pMem, width * height * 3);
	
//	} else {
//		throw UeyeOpenCVException(hCam, -1337);
//	}
	//double* dblFPS;
	//int actualFramePerSec = is_GetFramesPerSecond (hCam , dblFPS); //indicates the actual fps
	//cout << actualFramePerSec <<endl;
}

HIDS UeyeOpencvCam::getHIDS() {
	return hCam;
}

void UeyeOpencvCam::setAutoWhiteBalance(bool set) {
	double empty;
	double on = set ? 1 : 0;
	int retInt = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_WHITEBALANCE, &on, &empty);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}
}

void UeyeOpencvCam::setAutoGain(bool set) {
	double empty;
	double on = set ? 1 : 0;
	int retInt = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_GAIN, &on, &empty);
	if (retInt != IS_SUCCESS) {
		throw UeyeOpenCVException(hCam, retInt);
	}
}

INT UeyeOpencvCam::setExposure(bool& auto_exposure, double& exposure_ms) {
			
  INT is_err = IS_SUCCESS;

  double minExposure, maxExposure;

  // Set auto exposure

  double pval1 = auto_exposure, pval2 = 0;
  if ((is_err = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_SENSOR_SHUTTER,  &pval1, &pval2)) != IS_SUCCESS) {
			
		 if ((is_err = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_SHUTTER,  &pval1, &pval2)) != IS_SUCCESS) {
				auto_exposure = false;
    	 }
  }

  // Set manual exposure timing
  if (!auto_exposure) {
		
    // Make sure that user-requested exposure rate is achievable
    if (((is_err = is_Exposure(hCam, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MIN, (void*) &minExposure, sizeof(minExposure))) != IS_SUCCESS) ||
        ((is_err = is_Exposure(hCam, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MAX, (void*) &maxExposure, sizeof(maxExposure))) != IS_SUCCESS)) {

			cout << "Failed to query valid exposure range from camera" << endl;
			return is_err;
    }
    if (exposure_ms > minExposure && exposure_ms < maxExposure) {

		// Update exposure
		if ((is_err = is_Exposure(hCam, IS_EXPOSURE_CMD_SET_EXPOSURE, (void*) &(exposure_ms), sizeof(exposure_ms))) != IS_SUCCESS) {
		  cout << "Failed to set exposure to camera" << endl;
		  return is_err;
		}
	}
	else {
		cout << "Desired Exposure Time Is Out Of Range." << endl;
	}
  }	

  return is_err;
}

double UeyeOpencvCam::getExposure() {

	double exposureVal;
	INT error = is_Exposure (hCam, IS_EXPOSURE_CMD_GET_EXPOSURE, (void*)&exposureVal, sizeof(exposureVal));
	return exposureVal;
}

INT UeyeOpencvCam::setGain(bool& auto_gain, 
			               INT& master_gain_prc , 
			               INT& red_gain_prc	,
    		               INT& green_gain_prc	, 
			               INT& blue_gain_prc	, bool& gain_boost) {

  INT is_err = IS_SUCCESS;

  // Validate arguments
  if  ((master_gain_prc >= 0 && master_gain_prc <= 100) &
       (red_gain_prc	>= 0 && red_gain_prc	<= 100) &
       (green_gain_prc  >= 0 && green_gain_prc  <= 100) &
       (blue_gain_prc   >= 0 && blue_gain_prc   <= 100)) {

		double pval1 = 0, pval2 = 0;

		if (auto_gain) {
		  // Set auto gain
		  pval1 = 1;
		  if ((is_err = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_SENSOR_GAIN,  &pval1, &pval2)) != IS_SUCCESS) {
		    if ((is_err = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_GAIN, &pval1, &pval2)) != IS_SUCCESS) {
		      cout << "Failed Auto Gain" << endl;
		      auto_gain = false;
		    }
		  }
		} 
		else {
		  // Disable auto gain
		  if ((is_err = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_SENSOR_GAIN, &pval1, &pval2)) != IS_SUCCESS) {
		    if ((is_err = is_SetAutoParameter(hCam, IS_SET_ENABLE_AUTO_GAIN, &pval1, &pval2)) != IS_SUCCESS) {
		      cout << "Failed Auto Gain" << endl;
		    }
		  }

		  // Set gain boost
		  if (is_SetGainBoost(hCam, IS_GET_SUPPORTED_GAINBOOST) != IS_SET_GAINBOOST_ON) {
		    gain_boost = false;
		  } 
			else {
		    if ((is_err = is_SetGainBoost(hCam, (gain_boost) ? IS_SET_GAINBOOST_ON : IS_SET_GAINBOOST_OFF)) != IS_SUCCESS) {
					cout << "Failed Gain Boost" << endl;
		    }
		  }

		  // Set manual gain parameters
		  if ((is_err = is_SetHardwareGain(hCam, master_gain_prc, red_gain_prc, green_gain_prc, blue_gain_prc)) != IS_SUCCESS) {
					cout << "Failed Manual Gain Parameters" << endl;
		  }
		}
  }

  return is_err;
}

INT UeyeOpencvCam::getMasterGain() {
	INT nRed = 0, nGreen = 0, nBlue = 0;
	 return is_SetHardwareGain (hCam,IS_GET_MASTER_GAIN, nRed, nGreen, nBlue);
}	
INT UeyeOpencvCam::getRedGain() {
	INT nRed = 0, nGreen = 0, nBlue = 0;
	 return is_SetHardwareGain (hCam,IS_GET_RED_GAIN, nRed, nGreen, nBlue);

}	
INT UeyeOpencvCam::getBlueGain() {
	INT nRed = 0, nGreen = 0, nBlue = 0;
	 return is_SetHardwareGain (hCam,IS_GET_BLUE_GAIN, nRed, nGreen, nBlue);
}	
INT UeyeOpencvCam::getGreenGain() {
	INT nRed = 0, nGreen = 0, nBlue = 0;
	return is_SetHardwareGain (hCam,IS_GET_GREEN_GAIN, nRed, nGreen, nBlue);

}

double UeyeOpencvCam::setFPS(INT FPS) {	
	double newFPS = 0;
	is_SetFrameRate (hCam, FPS, &newFPS);
	return newFPS;
}
	

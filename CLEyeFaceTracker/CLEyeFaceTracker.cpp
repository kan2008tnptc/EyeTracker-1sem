//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This file is part of CL-EyeMulticam SDK
//
// C++ CLEyeFaceTracker Sample Application
//
// For updates and file downloads go to: http://codelaboratories.com
//
// Copyright 2008-2012 (c) Code Laboratories, Inc. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "cv.h"
//#include "tchar.h"
// Sample camera capture and processing class
#include "constants.h"
#include "findEyeCenter.h"
#include "findEyeCorner.h"
//CHAR _windowName[256];
class CLEyeCameraCapture
{
	CHAR _windowName[256];
	GUID _cameraGUID;
	CLEyeCameraInstance _cam;
	CLEyeCameraColorMode _mode;
	CLEyeCameraResolution _resolution;
	float _fps;
	HANDLE _hThread;
	bool _running;
	//cv::Mat window;
public:
	CLEyeCameraCapture(LPSTR windowname, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps) :
	_cameraGUID(cameraGUID), _cam(NULL), _mode(mode), _resolution(resolution), _fps(fps), _running(false)
	{
		strcpy(_windowName, windowname);
	}
	bool StartCapture()
	{
		_running = true;
		cvNamedWindow(_windowName, CV_WINDOW_AUTOSIZE);
		// Start CLEye image capture thread
		_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
		if(_hThread == NULL)
		{
			//MessageBox(NULL,"Could not create capture thread","CLEyeMulticamTest", MB_ICONEXCLAMATION);
			return false;
		}
		return true;
	}
	void StopCapture()
	{
		if(!_running)	return;
		_running = false;
		WaitForSingleObject(_hThread, 1000);
		//findEyes(_wind
		cvDestroyWindow(_windowName);
	}
	void IncrementCameraParameter(int param)
	{
		if(!_cam)	return;
		CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)+10);
	}
	void DecrementCameraParameter(int param)
	{
		if(!_cam)	return;
		CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)-10);
	}
	void Run()
	{
		int w, h;
		IplImage *pCapImage;
		PBYTE pCapBuffer = NULL;
		// Create camera instance
		_cam = CLEyeCreateCamera(_cameraGUID, _mode, _resolution, _fps);
		if(_cam == NULL)		return;
		// Get camera frame dimensions
		CLEyeCameraGetFrameDimensions(_cam, w, h);
		// Depending on color mode chosen, create the appropriate OpenCV image
		if(_mode == CLEYE_COLOR_PROCESSED || _mode == CLEYE_COLOR_RAW)
			pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
		else
			pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);

		// Set some camera parameters
		CLEyeSetCameraParameter(_cam, CLEYE_GAIN, 10);
		CLEyeSetCameraParameter(_cam, CLEYE_EXPOSURE, 100);

		// Start capturing
		CLEyeCameraStart(_cam);

		CvMemStorage* storage = cvCreateMemStorage(0);
		// Get the current app path
		//char strPathName[_MAX_PATH];
		//char* face_cascade_name = "haarcascade_frontalface_alt2.xml";
		//GetModuleFileName(NULL, strPathName, _MAX_PATH);
		//*(strrchr(strPathName, '\\') + 1) = '\0';
		// append the xml file name
		//strcat(strPathName, "haarcascade_frontalface_alt2.xml");
		CvHaarClassifierCascade* cascade = cvLoadHaarClassifierCascade("haarcascade_frontalface_alt2.xml", cvSize(24, 24));
		IplImage* image = cvCreateImage(cvSize(pCapImage->width, pCapImage->height), IPL_DEPTH_8U, 3);
		IplImage* temp = cvCreateImage(cvSize(pCapImage->width >> 1, pCapImage->height >> 1), IPL_DEPTH_8U, 3);
		// image capturing loop
		while(_running)
		{
			cvGetImageRawData(pCapImage, &pCapBuffer);
			CLEyeCameraGetFrame(_cam, pCapBuffer);
			cvConvertImage(pCapImage, image);
			cvPyrDown(image, temp, CV_GAUSSIAN_5x5);
			cvClearMemStorage(storage);

			if(cascade)
			{
				CvSeq* faces = cvHaarDetectObjects(temp, cascade, storage, 1.2, 2, CV_HAAR_DO_CANNY_PRUNING);
				for(int i = 0; i < faces->total; i++)
				{
					CvPoint pt1, pt2;
					CvRect* r = (CvRect*)cvGetSeqElem(faces, i);

					pt1.x = r->x * 2;
					pt2.x = (r->x + r->width) * 2;
					pt1.y = r->y * 2;
					pt2.y = (r->y + r->height) * 2;
					cvRectangle(image, pt1, pt2, CV_RGB(0, 0, 0), 3);
				}
			}
			cvShowImage(_windowName, image);
		}
		cvReleaseImage(&temp);
		cvReleaseImage(&image);

		// Stop camera capture
		CLEyeCameraStop(_cam);
		// Destroy camera object
		CLEyeDestroyCamera(_cam);
		// Destroy the allocated OpenCV image
		cvReleaseImage(&pCapImage);
		_cam = NULL;
	}
	static DWORD WINAPI CaptureThread(LPVOID instance)
	{
		// seed the rng with current tick count and thread id
		srand(GetTickCount() + GetCurrentThreadId());
		// forward thread to Capture function
		CLEyeCameraCapture *pThis = (CLEyeCameraCapture *)instance;
		pThis->Run();
		return 0;
	}
};

void findEyes(cv::Mat frame_gray, cv::Rect face) {
  cv::Mat faceROI = frame_gray(face);
           // vector of rectangles for eyes
        
  cv::Mat debugFace = faceROI;
          if (kSmoothFaceImage) {
              double sigma = kSmoothFaceFactor * face.width;
              GaussianBlur( faceROI, faceROI, cv::Size( 0, 0 ), sigma);
          }
        //-- Find eye regions and draw them
        int eye_region_width = face.width * (kEyePercentWidth/100.0);
        int eye_region_height = face.width * (kEyePercentHeight/100.0);
        int eye_region_top = face.height * (kEyePercentTop/100.0);
        cv::Rect leftEyeRegion(face.width*(kEyePercentSide/100.0),eye_region_top,eye_region_width,eye_region_height);
        cv::Rect rightEyeRegion(face.width - eye_region_width - face.width*(kEyePercentSide/100.0),eye_region_top,eye_region_width,eye_region_height);
      
  //-- Find Eye Centers
  cv::Point leftPupil = findEyeCenter(faceROI,leftEyeRegion,"Left Eye");
  cv::Point rightPupil = findEyeCenter(faceROI,rightEyeRegion,"Right Eye");
  // get corner regions
  cv::Rect leftRightCornerRegion(leftEyeRegion);
  leftRightCornerRegion.width -= leftPupil.x;
  leftRightCornerRegion.x += leftPupil.x;
  leftRightCornerRegion.height /= 2;
  leftRightCornerRegion.y += leftRightCornerRegion.height / 2;
  cv::Rect leftLeftCornerRegion(leftEyeRegion);
  leftLeftCornerRegion.width = leftPupil.x;
  leftLeftCornerRegion.height /= 2;
  leftLeftCornerRegion.y += leftLeftCornerRegion.height / 2;
  cv::Rect rightLeftCornerRegion(rightEyeRegion);
  rightLeftCornerRegion.width = rightPupil.x;
  rightLeftCornerRegion.height /= 2;
  rightLeftCornerRegion.y += rightLeftCornerRegion.height / 2;
  cv::Rect rightRightCornerRegion(rightEyeRegion);
  rightRightCornerRegion.width -= rightPupil.x;
  rightRightCornerRegion.x += rightPupil.x;
  rightRightCornerRegion.height /= 2;
  rightRightCornerRegion.y += rightRightCornerRegion.height / 2;
  rectangle(debugFace,leftRightCornerRegion,200);
  rectangle(debugFace,leftLeftCornerRegion,200);
  rectangle(debugFace,rightLeftCornerRegion,200);
  rectangle(debugFace,rightRightCornerRegion,200);
  // change eye centers to face coordinates
  rightPupil.x += rightEyeRegion.x;
  rightPupil.y += rightEyeRegion.y;
  leftPupil.x += leftEyeRegion.x;
  leftPupil.y += leftEyeRegion.y;
  // draw eye centers
  circle(debugFace, rightPupil, 3, 1234);
  circle(debugFace, leftPupil, 3, 1234);

  //-- Find Eye Corners
  if (kEnableEyeCorner) {
    cv::Point2f leftRightCorner = findEyeCorner(faceROI(leftRightCornerRegion), true, false);
    leftRightCorner.x += leftRightCornerRegion.x;
    leftRightCorner.y += leftRightCornerRegion.y;
    cv::Point2f leftLeftCorner = findEyeCorner(faceROI(leftLeftCornerRegion), true, true);
    leftLeftCorner.x += leftLeftCornerRegion.x;
    leftLeftCorner.y += leftLeftCornerRegion.y;
    cv::Point2f rightLeftCorner = findEyeCorner(faceROI(rightLeftCornerRegion), false, true);
    rightLeftCorner.x += rightLeftCornerRegion.x;
    rightLeftCorner.y += rightLeftCornerRegion.y;
    cv::Point2f rightRightCorner = findEyeCorner(faceROI(rightRightCornerRegion), false, false);
    rightRightCorner.x += rightRightCornerRegion.x;
    rightRightCorner.y += rightRightCornerRegion.y;
    circle(faceROI, leftRightCorner, 3, 200);
    circle(faceROI, leftLeftCorner, 3, 200);
    circle(faceROI, rightLeftCorner, 3, 200);
    circle(faceROI, rightRightCorner, 3, 200);
  }
  //cvShowImage(_windowName, faceROI);
  //imshow(_windowName, faceROI);
  }

int _tmain(int argc, _TCHAR* argv[])
{
	CLEyeCameraCapture *cam[3] = {NULL};
	// Query for number of connected cameras
	int numCams = CLEyeGetCameraCount();
	if(numCams == 0)
	{
		printf("No PS3Eye cameras detected\n");
		return -1;
	}
	printf( "Found %d cameras\n", numCams );

	for ( int i = 0; i < numCams; i++ ) {
	char windowName[64];
	// Query unique camera uuid
	GUID guid = CLEyeGetCameraUUID(i);
	printf("Camera GUID: [%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x]\n", i+1,
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	//sprintf(windowName, "Face Tracker Window",i);
	sprintf(windowName, "Face Tracker Window %d",i+1);
	// Create camera capture object
	// Randomize resolution and color mode
	//cam[i] = new CLEyeCameraCapture(windowName, guid, CLEYE_COLOR_PROCESSED, CLEYE_VGA, 30);
	cam[i] = new CLEyeCameraCapture( windowName, guid, rand()<(RAND_MAX >> 1) ? CLEYE_COLOR_PROCESSED : CLEYE_MONO_PROCESSED,
			rand()<(RAND_MAX >> 1 ) ? CLEYE_VGA : CLEYE_QVGA, 15 );	
	printf("Starting capture\n");
	cam[i]->StartCapture();

	}

	/*printf("Use the following keys to change camera parameters:\n"
		"\t'g' - select gain parameter\n"
		"\t'e' - select exposure parameter\n"
		"\t'z' - select zoom parameter\n"
		"\t'r' - select rotation parameter\n"
		"\t'+' - increment selected parameter\n"
		"\t'-' - decrement selected parameter\n");*/
	printf( "Just for test!\nUse the following keys to change camera parameters:\n"
		"\t'1' - select camera 1\n"
		"\t'2' - select camera 2\n"
		"\t<ESC> key will exit the program.\n");
	// The <ESC> key will exit the program
	CLEyeCameraCapture *pCam = NULL;
	int param = -1, key;
	while((key = cvWaitKey(0)) != 0x1b)
	{
		switch(key)
		{
		//case 'g':	case 'G':	printf("Parameter Gain\n");		param = CLEYE_GAIN;		break;
		//case 'e':	case 'E':	printf("Parameter Exposure\n");	param = CLEYE_EXPOSURE;	break;
		//case 'z':	case 'Z':	printf("Parameter Zoom\n");		param = CLEYE_ZOOM;		break;
		//case 'r':	case 'R':	printf("Parameter Rotation\n");	param = CLEYE_ROTATION;	break;
		//case '+':	if(cam)		cam[0]->IncrementCameraParameter(param);					break;
		//case '-':	if(cam)		cam[1]->DecrementCameraParameter(param);					break;
		case '1': printf( "Selected camera 1\n");	pCam = cam[0];	cam[1]->StopCapture();	break;
		case '2': printf( "Selected camera 2\n");	pCam = cam[1];	cam[1]->StopCapture();	break;
		}
	}
	for ( int i = 0; i < numCams; i++ ) {
	cam[i]->StopCapture();
	delete cam[i];
	}
	return 0;
}


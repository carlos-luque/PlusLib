/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __vtkInfraredSeekCam_h
#define __vtkInfraredSeekCam_h

#include "vtkPlusDataCollectionExport.h"
#include "vtkPlusDevice.h"

// OpenCV includes
#include <opencv2/videoio.hpp>

// Seek Pro
#include <seek.h>

/*!
\class vtkInfraredSeekCam
\brief Class for interfacing an OpenCVC capture device and recording frames into a Plus buffer

Requires the PLUS_USE_OpenCVCapture_VIDEO option in CMake.
Requires OpenCV with FFMPEG built (for RTSP support)

\ingroup PlusLibDataCollection
*/

namespace cv
{
  class VideoCapture;
  class Mat;
}

class vtkPlusDataCollectionExport vtkInfraredSeekCam : public vtkPlusDevice
{
public:
  static vtkInfraredSeekCam* New();
  vtkTypeMacro(vtkInfraredSeekCam, vtkPlusDevice);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /*! Read configuration from xml data */
  PlusStatus ReadConfiguration(vtkXMLDataElement* config);
  /*! Write configuration to xml data */
  PlusStatus WriteConfiguration(vtkXMLDataElement* config);

  /*! Manage device frozen state */
  PlusStatus FreezeDevice(bool freeze);

  /*! Is this device a tracker */
  bool IsTracker() const { return false; }

  /*! Get an update from the tracking system and push the new transforms to the tools. This function is called by the tracker thread.*/
  PlusStatus InternalUpdate();

  /*! Verify the device is correctly configured */
  virtual PlusStatus NotifyConfigured();

  vtkGetStdStringMacro(VideoURL);
  vtkSetStdStringMacro(VideoURL);

protected:
  vtkInfraredSeekCam();
  ~vtkInfraredSeekCam();

  virtual PlusStatus InternalConnect();
  virtual PlusStatus InternalDisconnect();

  cv::VideoCaptureAPIs CaptureAPIFromString(const std::string& apiString);
  std::string StringFromCaptureAPI(cv::VideoCaptureAPIs api);

protected:
  std::string                       VideoURL;
  std::shared_ptr<cv::VideoCapture> Capture;
  std::shared_ptr<cv::Mat>          Frame;
  cv::VideoCaptureAPIs              RequestedCaptureAPI;

  LibSeek::SeekThermalPro           SeekPro;
};

#endif // __vtkInfraredSeekCam_h
/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

// Local includes
#include "PlusConfigure.h"
#include "vtkInfraredSeekCam.h"
#include "vtkPlusChannel.h"
#include "vtkPlusDataSource.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

// OpenCV includes
#include <opencv2/imgproc/imgproc.hpp>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkInfraredSeekCam);

//----------------------------------------------------------------------------
vtkInfraredSeekCam::vtkInfraredSeekCam()
  : VideoURL("")
  , RequestedCaptureAPI(cv::CAP_ANY)
{
  this->RequireImageOrientationInConfiguration = true;
  this->StartThreadForInternalUpdates = true;
}

//----------------------------------------------------------------------------
vtkInfraredSeekCam::~vtkInfraredSeekCam()
{
}

//----------------------------------------------------------------------------
void vtkInfraredSeekCam::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "StreamURL: " << this->VideoURL << std::endl;
}

//-----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::ReadConfiguration(vtkXMLDataElement* rootConfigElement)
{
  LOG_TRACE("vtkInfraredSeekCam::ReadConfiguration");
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_READING(deviceConfig, rootConfigElement);
  XML_READ_STRING_ATTRIBUTE_REQUIRED(VideoURL, deviceConfig);
  std::string captureApi;
  XML_READ_STRING_ATTRIBUTE_NONMEMBER_OPTIONAL("CaptureAPI", captureApi, deviceConfig);
  if (!captureApi.empty())
  {
    this->RequestedCaptureAPI = CaptureAPIFromString(captureApi);
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::WriteConfiguration(vtkXMLDataElement* rootConfigElement)
{
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_WRITING(deviceConfig, rootConfigElement);
  XML_WRITE_STRING_ATTRIBUTE_REMOVE_IF_EMPTY(VideoURL, rootConfigElement);
  if (this->RequestedCaptureAPI != cv::CAP_ANY)
  {
    rootConfigElement->SetAttribute("CaptureAPI", StringFromCaptureAPI(this->RequestedCaptureAPI).c_str());
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::FreezeDevice(bool freeze)
{
  if (freeze)
  {
    this->Disconnect();
  }
  else
  {
    this->Connect();
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::InternalConnect()
{
  this->Capture = std::make_shared<cv::VideoCapture>(this->VideoURL, this->RequestedCaptureAPI);
  this->Frame = std::make_shared<cv::Mat>();

  if (!this->Capture->isOpened())
  {
    LOG_ERROR("Unable to open stream at " << this->VideoURL);
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::InternalDisconnect()
{
  this->Capture = nullptr; // automatically closes resources/connections
  this->Frame = nullptr;

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::InternalUpdate()
{
  LOG_TRACE("vtkInfraredSeekCam::InternalUpdate");

  if (!this->Capture->isOpened())
  {
    // No need to update if we're not able to read data
    return PLUS_SUCCESS;
  }

  // Capture one frame from the OpenCV capture device
  if (!this->Capture->read(*this->Frame))
  {
    LOG_ERROR("Unable to receive frame");
    return PLUS_FAIL;
  }

  // BGR -> RGB color
  cv::cvtColor(*this->Frame, *this->Frame, cv::COLOR_BGR2RGB);

  vtkPlusDataSource* aSource(nullptr);
  if (this->GetFirstActiveOutputVideoSource(aSource) == PLUS_FAIL || aSource == nullptr)
  {
    LOG_ERROR("Unable to grab a video source. Skipping frame.");
    return PLUS_FAIL;
  }

  if (aSource->GetNumberOfItems() == 0)
  {
    // Init the buffer with the metadata from the first frame
    aSource->SetImageType(US_IMG_RGB_COLOR);
    aSource->SetPixelType(VTK_UNSIGNED_CHAR);
    aSource->SetNumberOfScalarComponents(3);
    aSource->SetInputFrameSize(this->Frame->cols, this->Frame->rows, 1);
  }

  // Add the frame to the stream buffer
  int frameSize[3] = { this->Frame->cols, this->Frame->rows, 1 };
  if (aSource->AddItem(this->Frame->data, aSource->GetInputImageOrientation(), frameSize, VTK_UNSIGNED_CHAR, 3, US_IMG_RGB_COLOR, 0, this->FrameNumber) == PLUS_FAIL)
  {
    return PLUS_FAIL;
  }

  this->FrameNumber++;

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
cv::VideoCaptureAPIs vtkInfraredSeekCam::CaptureAPIFromString(const std::string& apiString)
{
  if (apiString.compare("CAP_ANY") == 0)
  {
    return cv::CAP_ANY;
  }
  else if (apiString.compare("CAP_VFW") == 0)
  {
    return cv::CAP_VFW;
  }
  else if (apiString.compare("CAP_V4L") == 0)
  {
    return cv::CAP_V4L;
  }
  else if (apiString.compare("CAP_V4L2") == 0)
  {
    return cv::CAP_V4L2;
  }
  else if (apiString.compare("CAP_FIREWIRE") == 0)
  {
    return cv::CAP_FIREWIRE;
  }
  else if (apiString.compare("CAP_FIREWARE") == 0)
  {
    return cv::CAP_FIREWARE;
  }
  else if (apiString.compare("CAP_IEEE1394") == 0)
  {
    return cv::CAP_IEEE1394;
  }
  else if (apiString.compare("CAP_DC1394") == 0)
  {
    return cv::CAP_DC1394;
  }
  else if (apiString.compare("CAP_CMU1394") == 0)
  {
    return cv::CAP_CMU1394;
  }
  else if (apiString.compare("CAP_QT") == 0)
  {
    return cv::CAP_QT;
  }
  else if (apiString.compare("CAP_UNICAP") == 0)
  {
    return cv::CAP_UNICAP;
  }
  else if (apiString.compare("CAP_DSHOW") == 0)
  {
    return cv::CAP_DSHOW;
  }
  else if (apiString.compare("CAP_PVAPI") == 0)
  {
    return cv::CAP_PVAPI;
  }
  else if (apiString.compare("CAP_OPENNI") == 0)
  {
    return cv::CAP_OPENNI;
  }
  else if (apiString.compare("CAP_OPENNI_ASUS") == 0)
  {
    return cv::CAP_OPENNI_ASUS;
  }
  else if (apiString.compare("CAP_ANDROID") == 0)
  {
    return cv::CAP_ANDROID;
  }
  else if (apiString.compare("CAP_XIAPI") == 0)
  {
    return cv::CAP_XIAPI;
  }
  else if (apiString.compare("CAP_AVFOUNDATION") == 0)
  {
    return cv::CAP_AVFOUNDATION;
  }
  else if (apiString.compare("CAP_GIGANETIX") == 0)
  {
    return cv::CAP_GIGANETIX;
  }
  else if (apiString.compare("CAP_MSMF") == 0)
  {
    return cv::CAP_MSMF;
  }
  else if (apiString.compare("CAP_WINRT") == 0)
  {
    return cv::CAP_WINRT;
  }
  else if (apiString.compare("CAP_INTELPERC") == 0)
  {
    return cv::CAP_INTELPERC;
  }
  else if (apiString.compare("CAP_OPENNI2") == 0)
  {
    return cv::CAP_OPENNI2;
  }
  else if (apiString.compare("CAP_OPENNI2_ASUS") == 0)
  {
    return cv::CAP_OPENNI2_ASUS;
  }
  else if (apiString.compare("CAP_GPHOTO2") == 0)
  {
    return cv::CAP_GPHOTO2;
  }
  else if (apiString.compare("CAP_GSTREAMER") == 0)
  {
    return cv::CAP_GSTREAMER;
  }
  else if (apiString.compare("CAP_FFMPEG") == 0)
  {
    return cv::CAP_FFMPEG;
  }
  else if (apiString.compare("CAP_IMAGES") == 0)
  {
    return cv::CAP_IMAGES;
  }
  else if (apiString.compare("CAP_ARAVIS") == 0)
  {
    return cv::CAP_ARAVIS;
  }

  LOG_WARNING("Unable to match requested API " << apiString << ". Defaulting to CAP_ANY");
  return cv::CAP_ANY;
}

#define _StringFromEnum(x) std::string(#x)
//----------------------------------------------------------------------------
std::string vtkInfraredSeekCam::StringFromCaptureAPI(cv::VideoCaptureAPIs api)
{
  switch (api)
  {
    case cv::CAP_ANY:
      return _StringFromEnum(CAP_ANY);
    case cv::CAP_VFW:
      return _StringFromEnum(CAP_VFW);
    case cv::CAP_FIREWIRE:
      return _StringFromEnum(CAP_FIREWIRE);
    case cv::CAP_QT:
      return _StringFromEnum(CAP_QT);
    case cv::CAP_UNICAP:
      return _StringFromEnum(CAP_UNICAP);
    case cv::CAP_DSHOW:
      return _StringFromEnum(CAP_DSHOW);
    case cv::CAP_PVAPI:
      return _StringFromEnum(CAP_PVAPI);
    case cv::CAP_OPENNI:
      return _StringFromEnum(CAP_OPENNI);
    case cv::CAP_OPENNI_ASUS:
      return _StringFromEnum(CAP_OPENNI_ASUS);
    case cv::CAP_ANDROID:
      return _StringFromEnum(CAP_ANDROID);
    case cv::CAP_XIAPI:
      return _StringFromEnum(CAP_XIAPI);
    case cv::CAP_AVFOUNDATION:
      return _StringFromEnum(CAP_AVFOUNDATION);
    case cv::CAP_GIGANETIX:
      return _StringFromEnum(CAP_GIGANETIX);
    case cv::CAP_MSMF:
      return _StringFromEnum(CAP_MSMF);
    case cv::CAP_WINRT:
      return _StringFromEnum(CAP_WINRT);
    case cv::CAP_INTELPERC:
      return _StringFromEnum(CAP_INTELPERC);
    case cv::CAP_OPENNI2:
      return _StringFromEnum(CAP_OPENNI2);
    case cv::CAP_OPENNI2_ASUS:
      return _StringFromEnum(CAP_OPENNI2_ASUS);
    case cv::CAP_GPHOTO2:
      return _StringFromEnum(CAP_GPHOTO2);
    case cv::CAP_GSTREAMER:
      return _StringFromEnum(CAP_GSTREAMER);
    case cv::CAP_FFMPEG:
      return _StringFromEnum(CAP_FFMPEG);
    case cv::CAP_IMAGES:
      return _StringFromEnum(CAP_IMAGES);
    case cv::CAP_ARAVIS:
      return _StringFromEnum(CAP_ARAVIS);
    default:
      return "CAP_ANY";
  }
}
#undef _StringFromEnum

//----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::NotifyConfigured()
{
  if (this->OutputChannels.size() > 1)
  {
    LOG_WARNING("vtkInfraredSeekCam is expecting one output channel and there are " << this->OutputChannels.size() << " channels. First output channel will be used.");
  }

  if (this->OutputChannels.empty())
  {
    LOG_ERROR("No output channels defined for vtkInfraredSeekCam. Cannot proceed.");
    this->CorrectlyConfigured = false;
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}
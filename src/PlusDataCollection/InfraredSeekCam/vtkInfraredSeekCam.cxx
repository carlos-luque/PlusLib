/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

// Local includes
#include "PlusConfigure.h"
#include "vtkPlusChannel.h"
#include "vtkPlusDataSource.h"
#include "vtkInfraredSeekCam.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

// OpenCV includes
//#include <opencv2/imgproc/imgproc.hpp>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkInfraredSeekCam);

//----------------------------------------------------------------------------
vtkInfraredSeekCam::vtkInfraredSeekCam()
{
}

//----------------------------------------------------------------------------
vtkInfraredSeekCam::~vtkInfraredSeekCam()
{
}

//----------------------------------------------------------------------------
void vtkInfraredSeekCam::PrintSelf(ostream& os, vtkIndent indent)
{
  //TODO
  // this->Superclass::PrintSelf(os, indent);

  // os << indent << "StreamURL: " << this->VideoURL << std::endl;
}

//-----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::ReadConfiguration(vtkXMLDataElement* rootConfigElement)
{
  //TODO
  LOG_TRACE("vtkInfraredSeekCam::ReadConfiguration");
  // XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_READING(deviceConfig, rootConfigElement);
  // XML_READ_STRING_ATTRIBUTE_REQUIRED(VideoURL, deviceConfig);
  // std::string captureApi;
  // XML_READ_STRING_ATTRIBUTE_NONMEMBER_OPTIONAL("CaptureAPI", captureApi, deviceConfig);
  // if (!captureApi.empty())
  // {
  //   this->RequestedCaptureAPI = CaptureAPIFromString(captureApi);
  // }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
PlusStatus vtkInfraredSeekCam::WriteConfiguration(vtkXMLDataElement* rootConfigElement)
{
  //TODO
  // XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_WRITING(deviceConfig, rootConfigElement);
  // XML_WRITE_STRING_ATTRIBUTE_REMOVE_IF_EMPTY(VideoURL, rootConfigElement);
  // if (this->RequestedCaptureAPI != cv::CAP_ANY)
  // {
  //   rootConfigElement->SetAttribute("CaptureAPI", StringFromCaptureAPI(this->RequestedCaptureAPI).c_str());
  // }
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
  this->Capture = std::make_shared<LibSeek::SeekThermalPro>();
  this->Frame = std::make_shared<cv::Mat>();

  if (!this->Capture->open())
  {
    LOG_ERROR("Failed to open seek pro");
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

  if (!this->Capture->open())
  {
    // No need to update if we're not able to read data
    return PLUS_SUCCESS;
  }

  // Capture one frame from the SeekPro capture device
  if (!this->Capture->read(*this->Frame))
  {
    LOG_ERROR("Unable to receive frame");
    return PLUS_FAIL;
  }

  // TODO: check these sentences
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
/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/ 

#include "vtkTrackedFrameList.h" 
#include "PlusMath.h"

#include <math.h>
#include "vtkObjectFactory.h"
#include "vtksys/SystemTools.hxx"
#include "vtksys/RegularExpression.hxx"
#include "vtkXMLUtilities.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "vtkMetaImageSequenceIO.h"
#include "itkFixedArray.h"
#include "itkCastImageFilter.h"


//----------------------------------------------------------------------------
// ************************* TrackedFrame ************************************
//----------------------------------------------------------------------------
TrackedFrame::TrackedFrame()
{
  this->Timestamp = 0; 
  this->FrameSize[0] = 0; 
  this->FrameSize[1] = 0; 
  this->FiducialPointsCoordinatePx = NULL; 
}

//----------------------------------------------------------------------------
TrackedFrame::~TrackedFrame()
{
  this->SetFiducialPointsCoordinatePx(NULL); 
}

//----------------------------------------------------------------------------
TrackedFrame::TrackedFrame(const TrackedFrame& frame)
{
  this->Timestamp = 0; 
  this->FrameSize[0] = 0; 
  this->FrameSize[1] = 0; 
  this->FiducialPointsCoordinatePx = NULL; 

  *this = frame; 
}

//----------------------------------------------------------------------------
TrackedFrame& TrackedFrame::operator=(TrackedFrame const&trackedFrame)
{
  // Handle self-assignment
  if (this == &trackedFrame)
  {
    return *this;
  }

  this->CustomFrameFields = trackedFrame.CustomFrameFields; 
  this->ImageData = trackedFrame.ImageData; 
  this->Timestamp = trackedFrame.Timestamp;
  this->FrameSize[0] = trackedFrame.FrameSize[0]; 
  this->FrameSize[1] = trackedFrame.FrameSize[1]; 
  this->SetFiducialPointsCoordinatePx(trackedFrame.FiducialPointsCoordinatePx);

  return *this;
}

//----------------------------------------------------------------------------
int* TrackedFrame::GetFrameSize()
{
  this->ImageData.GetFrameSize(this->FrameSize);
  return this->FrameSize; 
}

//----------------------------------------------------------------------------
void TrackedFrame::GetFrameSize(int dim[2])
{
  this->ImageData.GetFrameSize(this->FrameSize);
  dim[0] = this->FrameSize[0];
  dim[1] = this->FrameSize[1];
}

//----------------------------------------------------------------------------
void TrackedFrame::SetImageData(const PlusVideoFrame &value)
{
  this->ImageData = value;

  // Set frame size too
  this->ImageData.GetFrameSize(this->FrameSize);
}

//----------------------------------------------------------------------------
int TrackedFrame::GetNumberOfBitsPerPixel()
{
  int numberOfBitsPerPixel(0); 
  numberOfBitsPerPixel = this->ImageData.GetNumberOfBytesPerPixel()*8;
  return numberOfBitsPerPixel; 
}

//----------------------------------------------------------------------------
void TrackedFrame::SetCustomFrameField( std::string name, std::string value )
{
  this->CustomFrameFields[name]=value;
}

//----------------------------------------------------------------------------
const char* TrackedFrame::GetCustomFrameField(const char* fieldName)
{
  if (fieldName == NULL )
  {
    LOG_ERROR("Unable to get custom frame field: field name is NULL!"); 
    return NULL; 
  }

  FieldMapType::iterator fieldIterator; 
  fieldIterator = this->CustomFrameFields.find(fieldName); 
  if ( fieldIterator != this->CustomFrameFields.end() )
  {
    return fieldIterator->second.c_str(); 
  }
  return NULL; 
}

//----------------------------------------------------------------------------
bool TrackedFrame::IsCustomFrameTransformNameDefined(PlusTransformName& transformName)
{
  std::string toolTransformName; 
  if ( transformName.GetTransformName(toolTransformName) != PLUS_SUCCESS )
  {
    return false; 
  }
  return this->IsCustomFrameFieldDefined(toolTransformName.c_str()); 
}

//----------------------------------------------------------------------------
bool TrackedFrame::IsCustomFrameFieldDefined(const char* fieldName)
{
  if (fieldName == NULL )
  {
    LOG_ERROR("Unable to find custom frame field: field name is NULL!"); 
    return false; 
  }

  FieldMapType::iterator fieldIterator; 
  fieldIterator = this->CustomFrameFields.find(fieldName); 
  if ( fieldIterator != this->CustomFrameFields.end() )
  {
    // field is found
    return true; 
  }
  // field is undefined
  return false; 
}

//----------------------------------------------------------------------------
PlusStatus TrackedFrame::GetCustomFrameTransform(PlusTransformName& frameTransformName, double transform[16]) 
{
  std::string transformName; 
  if ( frameTransformName.GetTransformName(transformName) != PLUS_SUCCESS )
  {
    LOG_ERROR("Unable to get custom transform, transform name is wrong!"); 
    return PLUS_FAIL; 
  }
  
  // Append Transform to the end of the transform name
  transformName.append("Transform"); 

  const char* frameTransformStr = GetCustomFrameField(transformName.c_str());
  if (frameTransformStr == NULL )
  {
    LOG_ERROR("Unable to get custom transform from name: " << transformName); 
    return PLUS_FAIL; 
  }

  // Find default frame transform 
  std::istringstream transformFieldValue(frameTransformStr); 
  double item; 
  int i = 0; 
  while ( transformFieldValue>>item && i<16)
  {
    transform[i++] = item; 
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus TrackedFrame::GetCustomFrameTransform( PlusTransformName& frameTransformName, vtkMatrix4x4* transformMatrix )
{
  double transform[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  const PlusStatus retValue = this->GetCustomFrameTransform(frameTransformName, transform); 
  transformMatrix->DeepCopy(transform); 

  return retValue; 
}

//----------------------------------------------------------------------------
PlusStatus TrackedFrame::GetCustomFrameTransformStatus(PlusTransformName& frameTransformName, TrackerStatus& status)
{
  status = TR_MISSING; 
  std::string transformName; 
  if ( frameTransformName.GetTransformName(transformName) != PLUS_SUCCESS )
  {
    LOG_ERROR("Unable to get custom transform status, transform name is wrong!"); 
    return PLUS_FAIL; 
  }
  
  std::string toolStatusFrameFieldName = transformName + std::string("TransformStatus");
  const char* strStatus = this->GetCustomFrameField(toolStatusFrameFieldName.c_str()); 
  if (strStatus == NULL )
  {
    LOG_ERROR("Unable to get custom transform status from name: " << toolStatusFrameFieldName); 
    return PLUS_FAIL; 
  }

  status = TrackedFrame::GetStatusFromString( strStatus );

  return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
PlusStatus TrackedFrame::SetCustomFrameTransformStatus(PlusTransformName& frameTransformName, std::string status)
{
  std::string transformStatusName; 
  if ( frameTransformName.GetTransformName(transformStatusName) != PLUS_SUCCESS )
  {
    LOG_ERROR("Unable to set custom transform status, transform name is wrong!"); 
    return PLUS_FAIL; 
  }
  
  // Append TransformStatus to the end of the transform name
  transformStatusName.append("TransformStatus");
  this->SetCustomFrameField(transformStatusName, status ); 

  return PLUS_SUCCESS; 
}
//----------------------------------------------------------------------------
PlusStatus TrackedFrame::SetCustomFrameTransform(PlusTransformName& frameTransformName, double transform[16]) 
{
  std::ostringstream strTransform; 
  for ( int i = 0; i < 16; ++i )
  {
    strTransform << transform[ i ] << " ";
  }

  std::string transformName; 
  if ( frameTransformName.GetTransformName(transformName) != PLUS_SUCCESS )
  {
    LOG_ERROR("Unable to get custom transform, transform name is wrong!"); 
    return PLUS_FAIL; 
  }

  // Append Transform to the end of the transform name
  transformName.append("Transform"); 

  SetCustomFrameField(transformName, strTransform.str());

  return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
PlusStatus TrackedFrame::SetCustomFrameTransform(PlusTransformName& frameTransformName, vtkMatrix4x4* transform) 
{
  double dTransform[ 16 ];
  vtkMatrix4x4::DeepCopy( dTransform, transform );
  return SetCustomFrameTransform(frameTransformName, dTransform); 
}

//----------------------------------------------------------------------------
TrackerStatus TrackedFrame::GetStatusFromString(const char* statusStr) 
{
  if ( statusStr == NULL ) 
  {
    LOG_ERROR("Failed to get status from string if it's NULL!"); 
    return TR_MISSING; 
  }

  TrackerStatus status = TR_OK;
  std::string strFlag(statusStr); 
  if ( strFlag.find("OK") != std::string::npos )
  {
    status = TR_OK;
  }
  else if ( strFlag.find("TR_MISSING") != std::string::npos )
  {
    status = TR_MISSING;
  }
  else if ( strFlag.find("TR_OUT_OF_VIEW") != std::string::npos )
  {
    status = TR_OUT_OF_VIEW;
  }
  else if ( strFlag.find("TR_OUT_OF_VOLUME") != std::string::npos )
  {
    status = TR_OUT_OF_VOLUME;
  }
  else if ( strFlag.find("TR_REQ_TIMEOUT") != std::string::npos )
  {
    status = TR_REQ_TIMEOUT;
  }
  return status;
}

//----------------------------------------------------------------------------
PlusStatus TrackedFrame::WriteToFile(std::string &filename, vtkMatrix4x4* mImageToTracker)
{
  typedef unsigned char			PixelType; 
  typedef itk::Image< PixelType, 2 > Image2dType; 
  typedef itk::Image< PixelType, 3 > Image3dType; 

  typedef itk::CastImageFilter< Image2dType, Image3dType> CastFilterType;
  CastFilterType::Pointer castFilter = CastFilterType::New(); 
  castFilter->SetInput(this->ImageData.GetDisplayableImage());
  castFilter->Update();
  Image3dType::Pointer image=castFilter->GetOutput();     

  double origin[3]=
  {
    mImageToTracker->Element[0][3],
    mImageToTracker->Element[1][3],
    mImageToTracker->Element[2][3],
  };
  image->SetOrigin(origin);

  Image3dType::DirectionType dir;
  for (int r=0; r<3; r++)
  {
    for (int c=0; c<3; c++)
    {
      dir.GetVnlMatrix().put(r,c, mImageToTracker->Element[r][c]);
    }
  }
  image->SetDirection(dir);

  typedef itk::ImageFileWriter< Image3dType > WriterType; 
  WriterType::Pointer writeImage = WriterType::New();  

  writeImage->SetFileName(filename.c_str());  
  writeImage->SetInput( image );
  try
  {
    writeImage->Update(); 
  }
  catch (itk::ExceptionObject & err) 
  {		
    LOG_ERROR(" Exception! writer did not update. Error: "<< err); 
    return PLUS_FAIL;
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
void TrackedFrame::GetCustomFrameFieldNameList(std::vector<std::string> &fieldNames)
{
  fieldNames.clear();
  for ( FieldMapType::const_iterator it = this->CustomFrameFields.begin(); it != this->CustomFrameFields.end(); it++) 
  {
    fieldNames.push_back(it->first); 
  }  
}

//----------------------------------------------------------------------------
void TrackedFrame::GetCustomFrameTransformNameList(std::vector<PlusTransformName> &transformNames)
{
  transformNames.clear();
  for ( FieldMapType::const_iterator it = this->CustomFrameFields.begin(); it != this->CustomFrameFields.end(); it++) 
  {
    vtksys::RegularExpression isTransform("(.*)Transform$"); 
    if ( isTransform.find(it->first) )
    {
      PlusTransformName trName; 
      trName.SetTransformName(isTransform.match(1).c_str()); 
      transformNames.push_back(trName); 
    }
  }
}

//----------------------------------------------------------------------------
// ************************* vtkTrackedFrameList *****************************
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkTrackedFrameList, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkTrackedFrameList); 

//----------------------------------------------------------------------------
vtkTrackedFrameList::vtkTrackedFrameList()
{
  this->CustomFields["DefaultFrameTransformName"] = "ToolToTrackerTransform"; 
  this->CustomFields["UltrasoundImageOrientation"] = "MF"; 

  this->SetNumberOfUniqueFrames(5); 

  this->MinRequiredTranslationDifferenceMm=0.0;
  this->MinRequiredAngleDifferenceDeg=0.0;
  this->MaxAllowedTranslationSpeedMmPerSec=0.0;
  this->MaxAllowedRotationSpeedDegPerSec=0.0;
  this->ValidationRequirements = 0; 

}

//----------------------------------------------------------------------------
vtkTrackedFrameList::~vtkTrackedFrameList()
{
  this->Clear(); 
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::RemoveTrackedFrame( int frameNumber )
{
  if ( frameNumber <= 0 || frameNumber > this->GetNumberOfTrackedFrames() )
  {
    LOG_WARNING("Failed to remove tracked frame from list - invalid frame number: " << frameNumber ); 
    return PLUS_FAIL; 
  }

  delete this->TrackedFrameList[frameNumber]; 
  this->TrackedFrameList.erase(this->TrackedFrameList.begin()+frameNumber); 

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
void vtkTrackedFrameList::Clear()
{
  for ( int i = 0; i < this->TrackedFrameList.size(); i++ )
  {
    if (this->TrackedFrameList[i] != NULL )
    {
      delete this->TrackedFrameList[i]; 
      this->TrackedFrameList[i] = NULL; 
    }
  }
  this->TrackedFrameList.clear(); 
}

//----------------------------------------------------------------------------
void vtkTrackedFrameList::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of frames = " << GetNumberOfTrackedFrames();
  for ( FieldMapType::const_iterator it = this->CustomFields.begin(); it != this->CustomFields.end(); it++) 
  {
    os << indent << it->first << " = " << it->second << std::endl; 
  }  
}

//----------------------------------------------------------------------------
TrackedFrame* vtkTrackedFrameList::GetTrackedFrame(int frameNumber)
{
  if ( frameNumber >= this->GetNumberOfTrackedFrames() )
  {
    LOG_ERROR("vtkTrackedFrameList::GetTrackedFrame requested a non-existing frame (framenumber="<<frameNumber);
    return NULL; 
  }
  return this->TrackedFrameList[frameNumber];   
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::AddTrackedFrameList(vtkTrackedFrameList* inTrackedFrameList)
{
  PlusStatus status = PLUS_SUCCESS; 
  for ( int i = 0; i < inTrackedFrameList->GetNumberOfTrackedFrames(); ++i )
  {
    if ( this->AddTrackedFrame( inTrackedFrameList->GetTrackedFrame(i) ) != PLUS_SUCCESS )
    {
      LOG_ERROR("Failed to add tracked frame to the list!"); 
      status = PLUS_FAIL; 
      continue; 
    }
  }

  return status; 
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::AddTrackedFrame(TrackedFrame *trackedFrame, InvalidFrameAction action/*=ADD_INVALID_FRAME_AND_REPORT_ERROR*/ )
{
  bool isFrameValid = this->ValidateData(trackedFrame); 

  if ( !isFrameValid )
  {
    switch(action)
    {
    case ADD_INVALID_FRAME_AND_REPORT_ERROR: 
      LOG_ERROR("A similar frame is already found in the tracked frame list, but invalid frame added to the list."); 
      break; 
    case ADD_INVALID_FRAME: 
      LOG_DEBUG("A similar frame is already found in the tracked frame list, but invalid frame added to the list.");
      break; 
    case SKIP_INVALID_FRAME_AND_REPORT_ERROR: 
      LOG_ERROR("A similar frame is already found in the tracked frame list, invalid frame skipped."); 
      return PLUS_FAIL;
    case SKIP_INVALID_FRAME: 
      LOG_DEBUG("A similar frame is already found in the tracked frame list, invalid frame skipped.");
      return PLUS_SUCCESS; 
    }
  }

  // Make a copy and add frame to the list 
  TrackedFrame* pTrackedFrame = new TrackedFrame(*trackedFrame); 
  this->TrackedFrameList.push_back(pTrackedFrame); 
  return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
bool vtkTrackedFrameList::ValidateData(TrackedFrame* trackedFrame )
{
  if ( this->ValidationRequirements == 0 )
  {
    // If we don't want to validate return immediately
    return true; 
  }

  if ( this->ValidationRequirements & REQUIRE_UNIQUE_TIMESTAMP )
  {
    if (! this->ValidateTimestamp(trackedFrame))
    {
      LOG_DEBUG("Validation failed - timestamp is not unique!"); 
      return false;
    }
  }

  if ( this->ValidationRequirements & REQUIRE_TRACKING_OK )
  {
    if (! this->ValidateStatus(trackedFrame))
    {
      LOG_DEBUG("Validation failed - tracking status in not OK!"); 
      return false;
    }
  }
  
  if ( this->ValidationRequirements & REQUIRE_CHANGED_TRANSFORM )
  {
    if (! this->ValidateTransform(trackedFrame))
    {
      LOG_DEBUG("Validation failed - transform is not changed!"); 
      return false;
    }
  }

  if ( this->ValidationRequirements & REQUIRE_CHANGED_ENCODER_POSITION )
  {
    if (! this->ValidateEncoderPosition(trackedFrame))
    {
      LOG_DEBUG("Validation failed - encoder position is not changed!"); 
      return false;
    }
  }


  if ( this->ValidationRequirements & REQUIRE_SPEED_BELOW_THRESHOLD )
  {
    if (! this->ValidateSpeed(trackedFrame))
    {
      LOG_DEBUG("Validation failed - speed is higher than threshold!"); 
      return false;
    }
  }

  return true; 
}

//----------------------------------------------------------------------------
bool vtkTrackedFrameList::ValidateTimestamp(TrackedFrame* trackedFrame)
{
  if ( this->TrackedFrameList.size() > 0 )
  {
    const bool isTimestampUnique = std::find_if(this->TrackedFrameList.begin(), this->TrackedFrameList.end(), TrackedFrameTimestampFinder(trackedFrame) ) == this->TrackedFrameList.end(); 

    if ( !isTimestampUnique )
    {
      LOG_DEBUG("Tracked frame timestamp validation result: we've already inserted this frame to container!"); 
      return false; 
    }

    return true; 
  }

  return true; 
}

//----------------------------------------------------------------------------
bool vtkTrackedFrameList::ValidateEncoderPosition( TrackedFrame* trackedFrame )
{
  TrackedFrameListType::iterator searchIndex; 
  const int containerSize = this->TrackedFrameList.size(); 
  if (containerSize < this->NumberOfUniqueFrames )
  {
    searchIndex = this->TrackedFrameList.begin(); 
  }
  else
  {
    searchIndex =this->TrackedFrameList.end() - this->NumberOfUniqueFrames; 
  }

  if (std::find_if(searchIndex, this->TrackedFrameList.end(), TrackedFrameEncoderPositionFinder(trackedFrame, 
    this->MinRequiredTranslationDifferenceMm, this->MinRequiredAngleDifferenceDeg) ) != this->TrackedFrameList.end() )
  {
    // We've already inserted this frame 
    LOG_DEBUG("Tracked frame encoder position validation result: we've already inserted this frame to container!"); 
    return false; 
  }
  return true; 	
}

//----------------------------------------------------------------------------
bool vtkTrackedFrameList::ValidateTransform(TrackedFrame* trackedFrame)
{
  TrackedFrameListType::iterator searchIndex; 
  const int containerSize = this->TrackedFrameList.size(); 
  if (containerSize < this->NumberOfUniqueFrames )
  {
    searchIndex = this->TrackedFrameList.begin(); 
  }
  else
  {
    searchIndex =this->TrackedFrameList.end() - this->NumberOfUniqueFrames; 
  }

  if (std::find_if(searchIndex, this->TrackedFrameList.end(), TrackedFrameTransformFinder(trackedFrame, this->FrameTransformNameForValidation,
    this->MinRequiredTranslationDifferenceMm, this->MinRequiredAngleDifferenceDeg) ) != this->TrackedFrameList.end() )
  {
    // We've already inserted this frame 
    LOG_DEBUG("Tracked frame transform validation result: we've already inserted this frame to container!"); 
    return false; 
  }

  return true; 	
}

//----------------------------------------------------------------------------
bool vtkTrackedFrameList::ValidateStatus(TrackedFrame* trackedFrame)
{
  TrackerStatus status = TR_MISSING;
  std::string transformName; 
  if ( this->FrameTransformNameForValidation.GetTransformName(transformName) != PLUS_SUCCESS )
  {
    LOG_WARNING("Failed to validate transform status - transform name for validation is incorrect!"); 
    return false; 
  }

  std::string toolStatusFrameFieldName = transformName + std::string("Status");
  status = TrackedFrame::GetStatusFromString( trackedFrame->GetCustomFrameField( toolStatusFrameFieldName.c_str() ) );

  if ( status != TR_OK )
  {
    LOG_DEBUG("Tracked frame status validation result: tracked frame status invalid for tool " << transformName); 
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkTrackedFrameList::ValidateSpeed(TrackedFrame* trackedFrame)
{
  if ( this->TrackedFrameList.size() < 1 ) {
    return true;
  }

  TrackedFrameListType::iterator latestFrameInList = this->TrackedFrameList.end() - 1;

  // Compute difference between the last two timestamps
  double diffTimeSec = fabs( trackedFrame->GetTimestamp() - (*latestFrameInList)->GetTimestamp() );
  if (diffTimeSec < 0.0001) 
  { 
    // the frames are almost acquired at the same time, cannot compute speed reliably
    // better to invalidate the frame
    return false;
  }

  // Get default frame transform of the input frame and the latest frame in the list
  vtkSmartPointer<vtkTransform> inputTransform = vtkSmartPointer<vtkTransform>::New(); 
  double inputTransformVector[16]={0}; 
  if ( trackedFrame->GetCustomFrameTransform(this->FrameTransformNameForValidation, inputTransformVector) )
  {
    inputTransform->SetMatrix(inputTransformVector); 
  }
  else
  {
    LOG_ERROR("Unable to get default frame transform for input frame!");
    return false;
  }

  vtkSmartPointer<vtkTransform> latestTransform = vtkSmartPointer<vtkTransform>::New(); 
  double latestTransformVector[16]={0}; 
  if ( (*latestFrameInList)->GetCustomFrameTransform(this->FrameTransformNameForValidation, latestTransformVector) )
  {
    latestTransform->SetMatrix(latestTransformVector); 
  }
  else
  {
    LOG_ERROR("Unable to get default frame transform for latest frame!");
    return false;
  }
 
  if ( this->MaxAllowedTranslationSpeedMmPerSec>0)
  {
    // Compute difference between the last two positions
    double diffPosition = PlusMath::GetPositionDifference( inputTransform->GetMatrix(), latestTransform->GetMatrix() );
    double velocityPositionMmPerSec = fabs( diffPosition / diffTimeSec );
    if (velocityPositionMmPerSec > this->MaxAllowedTranslationSpeedMmPerSec)
    {
      LOG_DEBUG("Tracked frame speed validation result: tracked frame position change too fast (VelocityPosition = " << velocityPositionMmPerSec << ">" << this->MaxAllowedTranslationSpeedMmPerSec << " mm/sec)"); 
      return false; 
    }
  }

  if ( this->MaxAllowedRotationSpeedDegPerSec>0)
  {
    // Compute difference between the last two orientations
    double diffOrientationDeg = PlusMath::GetOrientationDifference( inputTransform->GetMatrix(), latestTransform->GetMatrix() );
    double velocityOrientationDegPerSec = fabs( diffOrientationDeg / diffTimeSec );
    if (velocityOrientationDegPerSec > this->MaxAllowedRotationSpeedDegPerSec )
    {
      LOG_DEBUG("Tracked frame speed validation result: tracked frame angle change too fast VelocityOrientation = " << velocityOrientationDegPerSec << ">" << this->MaxAllowedRotationSpeedDegPerSec << " deg/sec)"); 
      return false; 
    }
  }

  return true; 
}

//----------------------------------------------------------------------------
int vtkTrackedFrameList::GetNumberOfBitsPerPixel()
{
  int numberOfBitsPerPixel = 0; 
  if ( this->GetNumberOfTrackedFrames() > 0 )
  {
    numberOfBitsPerPixel = this->GetTrackedFrame(0)->GetNumberOfBitsPerPixel(); 
  }
  else
  {
    LOG_WARNING("Unable to get frame size: there is no frame in the tracked frame list!"); 
  }

  return numberOfBitsPerPixel; 
}

//----------------------------------------------------------------------------
PlusTransformName vtkTrackedFrameList::GetDefaultFrameTransformName()
{
  PlusTransformName defaultTransformName; 
  if( defaultTransformName.SetTransformName(this->CustomFields["DefaultFrameTransformName"].c_str()) != PLUS_SUCCESS )
  {
    LOG_WARNING("Default transform name is not valid: " << this->CustomFields["DefaultFrameTransformName"] ); 
  }
  return defaultTransformName;
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::SetDefaultFrameTransformName(PlusTransformName& name)
{
  std::string transformName; 
  if ( name.GetTransformName(transformName) != PLUS_SUCCESS )
  {
    LOG_ERROR("Failed to set default frame transform name - name is invalid (From: '" << name.From() << "'  To: '" << name.To() << "')" ); 
    return PLUS_FAIL; 
  }

  this->CustomFields["DefaultFrameTransformName"]=transformName;
  return PLUS_SUCCESS;
} 

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::ReadFromSequenceMetafile(const char* trackedSequenceDataFileName)
{
  vtkSmartPointer<vtkMetaImageSequenceIO> reader=vtkSmartPointer<vtkMetaImageSequenceIO>::New();
  reader->SetFileName(trackedSequenceDataFileName);
  reader->SetTrackedFrameList(this);
  if (reader->Read()!=PLUS_SUCCESS)
  {		
    LOG_ERROR("Couldn't read sequence metafile: " <<  trackedSequenceDataFileName ); 
    return PLUS_FAIL;
  }	
  
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::SaveToSequenceMetafile(const char* outputFolder, const char* sequenceDataFileName, SEQ_METAFILE_EXTENSION extension /*=SEQ_METAFILE_MHA*/ , bool useCompression /*=true*/)
{
  vtkSmartPointer<vtkMetaImageSequenceIO> writer=vtkSmartPointer<vtkMetaImageSequenceIO>::New();
  std::string trackedSequenceDataFileName=std::string(outputFolder)+std::string("\\")+std::string(sequenceDataFileName);
  if (extension==SEQ_METAFILE_MHA)
  {
    trackedSequenceDataFileName+=".mha";
  }
  else
  {
    trackedSequenceDataFileName+=".mhd";
  }
  writer->SetFileName(trackedSequenceDataFileName.c_str());
  writer->SetTrackedFrameList(this);
  writer->SetUseCompression(useCompression);
  if (writer->Write()!=PLUS_SUCCESS)
  {		
    LOG_ERROR("Couldn't write sequence metafile: " <<  trackedSequenceDataFileName ); 
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//-----------------------------------------------------------------------------
PlusCommon::ITKScalarPixelType vtkTrackedFrameList::GetPixelType()
{
  if ( this->GetNumberOfTrackedFrames() < 1 )
  {
    LOG_ERROR("Unable to get pixel type size: there is no frame in the tracked frame list!"); 
    return itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
  }

  return this->GetTrackedFrame(0)->GetImageData()->GetITKScalarPixelType();
}

//----------------------------------------------------------------------------
const char* vtkTrackedFrameList::GetCustomString( const char* fieldName )
{
  FieldMapType::iterator fieldIterator; 
  fieldIterator = this->CustomFields.find(fieldName); 
  if ( fieldIterator != this->CustomFields.end() )
  {
    return fieldIterator->second.c_str(); 
  }
  return NULL; 
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::GetCustomTransform( const char* frameTransformName, vtkMatrix4x4* transformMatrix )
{
  double transform[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  const PlusStatus retValue = this->GetCustomTransform(frameTransformName, transform); 
  transformMatrix->DeepCopy(transform); 

  return retValue; 
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::GetCustomTransform( const char* frameTransformName, double* transformMatrix )
{
  if (frameTransformName==NULL)
  {
    LOG_ERROR("Invalid frame transform name");
    return PLUS_FAIL; 
  }
  const char* customString = this->GetCustomString(frameTransformName); 
  if ( customString == NULL )
  {
    LOG_ERROR("Cannot find frame transform "<<frameTransformName);
    return PLUS_FAIL; 
  }

  std::istringstream transformFieldValue(customString); 
  double item; 
  int i = 0; 
  while ( transformFieldValue >> item )
  {
    transformMatrix[i++] = item; 
  }

  return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
void vtkTrackedFrameList::SetCustomTransform( const char* frameTransformName, vtkMatrix4x4* transformMatrix )
{
  double transform[16]; 
  for ( int i = 0; i < 4; i++ ) 
  {
    for ( int j = 0; j < 4; j++ ) 
    {
      transform[i*4+j] = transformMatrix->GetElement(i, j); 
    }
  }

  this->SetCustomTransform(frameTransformName, transform); 
}

//----------------------------------------------------------------------------
void vtkTrackedFrameList::SetCustomTransform( const char* frameTransformName, double* transformMatrix )
{
  std::ostringstream transform; 

  transform	<< transformMatrix[0]  << " " << transformMatrix[1]  << " " << transformMatrix[2]  << " " << transformMatrix[3]  << " " 
    << transformMatrix[4]  << " " << transformMatrix[5]  << " " << transformMatrix[6]  << " " << transformMatrix[7]  << " " 
    << transformMatrix[8]  << " " << transformMatrix[9]  << " " << transformMatrix[10] << " " << transformMatrix[11] << " " 
    << transformMatrix[12] << " " << transformMatrix[13] << " " << transformMatrix[14] << " " << transformMatrix[15] << " "; 

  this->SetCustomString(frameTransformName, transform.str().c_str()); 
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::SetCustomString(const char* fieldName, const char* fieldValue)
{
  if (fieldName==NULL)
  {
    LOG_ERROR("Field name is invalid");
    return PLUS_FAIL;
  }
  if (fieldValue==NULL)
  {
    this->CustomFields.erase(fieldName);
    return PLUS_SUCCESS;
  }
  this->CustomFields[fieldName]=fieldValue;
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
void vtkTrackedFrameList::GetCustomFieldNameList(std::vector<std::string> &fieldNames)
{
  fieldNames.clear();
  for ( FieldMapType::const_iterator it = this->CustomFields.begin(); it != this->CustomFields.end(); it++) 
  {
    fieldNames.push_back(it->first); 
  }  
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::GetGlobalTransform(vtkMatrix4x4* globalTransform)
{
  const char* offsetStr=GetCustomString("Offset");
  if (offsetStr==NULL)
  {
    LOG_ERROR("Cannot determine global transform, Offset is undefined");
    return PLUS_FAIL;
  }
  const char* transformStr=GetCustomString("TransformMatrix");
  if (transformStr==NULL)
  {
    LOG_ERROR("Cannot determine global transform, TransformMatrix is undefined");
    return PLUS_FAIL;
  }

  double item;
  int i=0;

  const int offsetLen=3;
  double offset[offsetLen];
  std::istringstream offsetFieldValue(offsetStr); 
  for (i = 0; offsetFieldValue>>item && i<offsetLen; i++)
  {
    offset[i] = item; 
  }
  if (i<offsetLen)
  {
    LOG_ERROR("Not enough elements in the Offset field (expected "<<offsetLen<<", found "<<i<<")");
    return PLUS_FAIL;
  }

  const int transformLen=9;
  double transform[transformLen];
  std::istringstream transformFieldValue(transformStr); 
  for (i = 0; transformFieldValue>>item && i<transformLen; i++)
  {
    transform[i] = item; 
  }
  if (i<transformLen)
  {
    LOG_ERROR("Not enough elements in the TransformMatrix field (expected "<<transformLen<<", found "<<i<<")");
    return PLUS_FAIL;
  }

  globalTransform->Identity();

  globalTransform->SetElement(0, 3, offset[0]);
  globalTransform->SetElement(1, 3, offset[1]);
  globalTransform->SetElement(2, 3, offset[2]);

  globalTransform->SetElement(0, 0, transform[0]);
  globalTransform->SetElement(0, 1, transform[1]);
  globalTransform->SetElement(0, 2, transform[2]);
  globalTransform->SetElement(1, 0, transform[3]);
  globalTransform->SetElement(1, 1, transform[4]);
  globalTransform->SetElement(1, 2, transform[5]);
  globalTransform->SetElement(2, 0, transform[6]);
  globalTransform->SetElement(2, 1, transform[7]);
  globalTransform->SetElement(2, 2, transform[8]);
  
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkTrackedFrameList::SetGlobalTransform(vtkMatrix4x4* globalTransform)
{
  std::ostringstream strOffset;
  strOffset << globalTransform->GetElement(0,3)
    << " " << globalTransform->GetElement(1,3)
    << " " << globalTransform->GetElement(2,3);  
  SetCustomString("Offset", strOffset.str().c_str());

  std::ostringstream strTransform;
  strTransform << globalTransform->GetElement(0,0) << " " << globalTransform->GetElement(0,1) << " " << globalTransform->GetElement(0,2) << " ";  
  strTransform << globalTransform->GetElement(1,0) << " " << globalTransform->GetElement(1,1) << " " << globalTransform->GetElement(1,2) << " ";  
  strTransform << globalTransform->GetElement(2,0) << " " << globalTransform->GetElement(2,1) << " " << globalTransform->GetElement(2,2);  
  SetCustomString("TransformMatrix", strTransform.str().c_str());

  return PLUS_SUCCESS;
}

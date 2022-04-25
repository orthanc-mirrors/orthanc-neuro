/**
 * Neuroimaging plugin for Orthanc
 * Copyright (C) 2021-2022 Sebastien Jodogne, UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "InputDicomInstance.h"

#include "NeuroToolbox.h"

#include <Logging.h>
#include <OrthancException.h>
#include <SerializationToolbox.h>
#include <Toolbox.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#if ORTHANC_ENABLE_DCMTK == 1
#  include <DicomParsing/FromDcmtkBridge.h>
#endif

#include <nifti1_io.h>

static const Orthanc::DicomTag DICOM_TAG_ECHO_TIME(0x0018, 0x0081);
static const Orthanc::DicomTag DICOM_TAG_IN_PLANE_PHASE_ENCODING_DIRECTION(0x0018, 0x1312);
static const Orthanc::DicomTag DICOM_TAG_REPETITION_TIME(0x0018, 0x0080);
static const Orthanc::DicomTag DICOM_TAG_SLICE_SLOPE_PHILIPS(0x2005, 0x100e);
static const Orthanc::DicomTag DICOM_TAG_SLICE_TIMING_SIEMENS(0x0019, 0x1029);
static const Orthanc::DicomTag DICOM_TAG_SPACING_BETWEEN_SLICES(0x0018, 0x0088);

static const std::string CSA_NUMBER_OF_IMAGES_IN_MOSAIC = "NumberOfImagesInMosaic";
static const std::string CSA_SLICE_NORMAL_VECTOR = "SliceNormalVector";


namespace Neuro
{
  static Manufacturer GetManufacturer(const Orthanc::DicomMap& dicom)
  {
    std::string manufacturer = dicom.GetStringValue(Orthanc::DICOM_TAG_MANUFACTURER, "", false);
    Orthanc::Toolbox::ToUpperCase(manufacturer);
      
    if (boost::algorithm::starts_with(manufacturer, "SI"))
    {
      return Manufacturer_Siemens;
    }
    else if (boost::algorithm::starts_with(manufacturer, "GE"))
    {
      return Manufacturer_GE;
    }
    else if (boost::algorithm::starts_with(manufacturer, "HI"))
    {
      return Manufacturer_Hitachi;
    }
    else if (boost::algorithm::starts_with(manufacturer, "ME"))
    {
      return Manufacturer_Mediso;
    }
    else if (boost::algorithm::starts_with(manufacturer, "PH"))
    {
      return Manufacturer_Philips;
    }
    else if (boost::algorithm::starts_with(manufacturer, "TO"))
    {
      return Manufacturer_Toshiba;
    }
    else if (boost::algorithm::starts_with(manufacturer, "CA"))
    {
      return Manufacturer_Canon;
    }
    else if (boost::algorithm::starts_with(manufacturer, "UI"))
    {
      return Manufacturer_UIH;
    }
    else if (boost::algorithm::starts_with(manufacturer, "BR"))
    {
      return Manufacturer_Bruker;
    }
    else
    {
      return Manufacturer_Unknown;
    }
  }
  

  static Modality GetModality(const Orthanc::DicomMap& dicom)
  {
    std::string modality = dicom.GetStringValue(Orthanc::DICOM_TAG_MODALITY, "", false);
    Orthanc::Toolbox::ToUpperCase(modality);
      
    if (boost::algorithm::starts_with(modality, "MR"))
    {
      return Modality_MR;
    }
    else if (boost::algorithm::starts_with(modality, "PT"))
    {
      return Modality_PET;
    }
    else if (boost::algorithm::starts_with(modality, "CT"))
    {
      return Modality_CT;
    }
    else
    {
      return Modality_Unknown;
    }
  }
  

  void InputDicomInstance::ParseImagePositionPatient()
  {
    if (NeuroToolbox::ParseVector(imagePositionPatient_, *tags_, Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT))
    {
      if (imagePositionPatient_.size() != 3)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      imagePositionPatient_.resize(3);
      imagePositionPatient_[0] = 0;
      imagePositionPatient_[1] = 0;
      imagePositionPatient_[2] = 0;
    }
  }


  void InputDicomInstance::ParseImageOrientationPatient()
  {
    if (NeuroToolbox::ParseVector(imageOrientationPatient_, *tags_, Orthanc::DICOM_TAG_IMAGE_ORIENTATION_PATIENT))
    {
      if (imageOrientationPatient_.size() != 6)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      // Set the canonical orientation
      imageOrientationPatient_.resize(6);
      imageOrientationPatient_[0] = 1;
      imageOrientationPatient_[1] = 0;
      imageOrientationPatient_[2] = 0;
      imageOrientationPatient_[3] = 0;
      imageOrientationPatient_[4] = 1;
      imageOrientationPatient_[5] = 0;
    }

    std::vector<double> axisX, axisY;

    axisX.resize(3);
    axisX[0] = imageOrientationPatient_[0];
    axisX[1] = imageOrientationPatient_[1];
    axisX[2] = imageOrientationPatient_[2];
      
    axisY.resize(3);
    axisY[0] = imageOrientationPatient_[3];
    axisY[1] = imageOrientationPatient_[4];
    axisY[2] = imageOrientationPatient_[5];
      
    NeuroToolbox::CrossProduct(normal_, axisX, axisY);
  }


  void InputDicomInstance::ParsePixelSpacing()
  {
    std::vector<double> pixelSpacing;
    if (NeuroToolbox::ParseVector(pixelSpacing, *tags_, Orthanc::DICOM_TAG_PIXEL_SPACING))
    {
      if (pixelSpacing.size() != 2)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        pixelSpacingX_ = pixelSpacing[0];
        pixelSpacingY_ = pixelSpacing[1];
      }
    }
    else
    {
      pixelSpacingX_ = 1;
      pixelSpacingY_ = 1;
    }
  }


  void InputDicomInstance::ParseVoxelSpacingZ()
  {
    std::vector<double> v;
    if (NeuroToolbox::ParseVector(v, *tags_, DICOM_TAG_SPACING_BETWEEN_SLICES))
    {
      if (v.size() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        voxelSpacingZ_ = v[0];
      }
    }
    else if (NeuroToolbox::ParseVector(v, *tags_, Orthanc::DICOM_TAG_SLICE_THICKNESS))
    {
      if (v.size() != 1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        voxelSpacingZ_ = v[0];
      }
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat, "Unable to determine spacing between slices");
    }      
  }


  void InputDicomInstance::ParseRescale()
  {
    std::vector<double> v;
      
    if (NeuroToolbox::ParseVector(v, *tags_, Orthanc::DICOM_TAG_RESCALE_SLOPE))
    {
      if (v.size() == 1)
      {
        rescaleSlope_ = v[0];
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      rescaleSlope_ = 1;
    }

    if (manufacturer_ == Manufacturer_Philips &&
        NeuroToolbox::ParseVector(v, *tags_, DICOM_TAG_SLICE_SLOPE_PHILIPS))
    {
      if (v.size() == 1 &&
          !NeuroToolbox::IsNear(v[0], 0))
      {
        rescaleSlope_ /= v[0];  // cf. PMC3998685
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }

    if (NeuroToolbox::ParseVector(v, *tags_, Orthanc::DICOM_TAG_RESCALE_INTERCEPT))
    {
      if (v.size() == 1)
      {
        rescaleIntercept_ = v[0];
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      rescaleIntercept_ = 0;
    }
  }


  void InputDicomInstance::ParsePhaseEncodingDirection()
  {
    const std::string s = tags_->GetStringValue(DICOM_TAG_IN_PLANE_PHASE_ENCODING_DIRECTION, "", false);

    if (s == "ROW")
    {
      phaseEncodingDirection_ = PhaseEncodingDirection_Row;
    }
    else if (s == "COL")
    {
      phaseEncodingDirection_ = PhaseEncodingDirection_Column;
    }
    else if (s.empty())
    {
      phaseEncodingDirection_ = PhaseEncodingDirection_None;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }


  void InputDicomInstance::ParseSliceTimingSiemens()
  {
    if (!NeuroToolbox::ParseVector(sliceTimingSiemens_, *tags_, DICOM_TAG_SLICE_TIMING_SIEMENS))
    {
      sliceTimingSiemens_.clear();
    }
  }


  void InputDicomInstance::Setup()
  {
    assert(tags_.get() != NULL);

    info_.reset(new Orthanc::DicomImageInformation(*tags_));

    if (!tags_->ParseInteger32(instanceNumber_, Orthanc::DICOM_TAG_INSTANCE_NUMBER))
    {
      LOG(WARNING) << "DICOM instance without an instance number";
    }
    
    manufacturer_ = ::Neuro::GetManufacturer(*tags_);
    modality_ = ::Neuro::GetModality(*tags_);
    hasEchoTime_ = tags_->ParseDouble(echoTime_, DICOM_TAG_ECHO_TIME);
    hasAcquisitionTime_ = tags_->ParseDouble(acquisitionTime_, Orthanc::DICOM_TAG_ACQUISITION_TIME);
    
    ParseImagePositionPatient();
    ParseImageOrientationPatient();
    ParsePixelSpacing();
    ParseVoxelSpacingZ();
    ParseRescale();
    ParseSliceTimingSiemens();
    ParsePhaseEncodingDirection();
  }
  

  double InputDicomInstance::GetImageOrientationPatient(unsigned int index) const
  {
    assert(imageOrientationPatient_.size() == 6);
      
    if (index >= 6)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return imageOrientationPatient_[index];
    }
  }


#if ORTHANC_ENABLE_DCMTK == 1
  void InputDicomInstance::LoadDicom(const Orthanc::ParsedDicomFile& dicom)
  {
    tags_.reset(new Orthanc::DicomMap);
    dicom.ExtractDicomSummary(*tags_, 0);

    std::string csa;
    if (dicom.GetTagValue(csa, DICOM_TAG_SIEMENS_CSA_HEADER))
    {
      csa_.Load(csa);
    }

    DcmSequenceOfItems *sequence = NULL;
    if (const_cast<Orthanc::ParsedDicomFile&>(dicom).GetDcmtkObject().getDataset()->findAndGetSequence(
          DcmTagKey(DICOM_TAG_UIH_MR_VFRAME_SEQUENCE.GetGroup(),
                    DICOM_TAG_UIH_MR_VFRAME_SEQUENCE.GetElement()), sequence).good() &&
        sequence != NULL)
    {
      for (unsigned long i = 0; i < sequence->card(); i++)
      {
        Orthanc::DicomMap m;
        std::set<Orthanc::DicomTag> none;
        Orthanc::FromDcmtkBridge::ExtractDicomSummary(m, *sequence->getItem(i), 0, none);
        AddUIHFrameSequenceItem(m);
      }
    }

    Setup();
  }
#endif


  InputDicomInstance::~InputDicomInstance()
  {
    for (size_t i = 0; i < uihFrameSequence_.size(); i++)
    {
      assert(uihFrameSequence_[i] != NULL);
      delete uihFrameSequence_[i];
    }
  }


  const Orthanc::DicomMap& InputDicomInstance::GetUIHFrameSequenceItem(size_t index) const
  {
    if (index >= uihFrameSequence_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(uihFrameSequence_[index] != NULL);
      return *uihFrameSequence_[index];
    }
  }


  double InputDicomInstance::GetEchoTime() const
  {
    if (hasEchoTime_)
    {
      return echoTime_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }


  double InputDicomInstance::GetAcquisitionTime() const
  {
    if (hasAcquisitionTime_)
    {
      return acquisitionTime_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }

    
  double InputDicomInstance::GetImagePositionPatient(unsigned int index) const
  {
    assert(imagePositionPatient_.size() == 3);
      
    if (index >= 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return imagePositionPatient_[index];
    }
  }


  double InputDicomInstance::GetAxisX(unsigned int index) const
  {
    if (index >= 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return GetImageOrientationPatient(index);
    }
  }

  
  double InputDicomInstance::GetAxisY(unsigned int index) const
  {
    if (index >= 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return GetImageOrientationPatient(3 + index);
    }
  }

  
  double InputDicomInstance::GetNormal(unsigned int index) const
  {
    assert(normal_.size() == 3);
      
    if (index >= 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return normal_[index];
    }
  }

  
  unsigned int InputDicomInstance::GetMultiBandFactor() const
  {
    std::vector<double> v;
    if (NeuroToolbox::ParseVector(v, *tags_, DICOM_TAG_SLICE_TIMING_SIEMENS))
    {
      unsigned int count = 0;

      for (size_t i = 0; i < v.size(); i++)
      {
        if (NeuroToolbox::IsNear(v[i], v[0]))
        {
          count++;
        }
      }

      return count;
    }
    else
    {
      return 0;
    }
  }


  int InputDicomInstance::DetectSiemensSliceCode() const
  {
    size_t countZeros = 0;
    for (size_t i = 0; i < sliceTimingSiemens_.size(); i++)
    {
      if (NeuroToolbox::IsNear(sliceTimingSiemens_[i], 0.0))
      {
        countZeros++;
      }
    }

    const size_t minTimeIndex = std::distance(sliceTimingSiemens_.begin(), std::min_element(sliceTimingSiemens_.begin(), sliceTimingSiemens_.end()));

    if (countZeros < 2)
    {
      const size_t size = sliceTimingSiemens_.size();  // corresponds to "itemsOK"

      if (minTimeIndex == 1)
      {
        return NIFTI_SLICE_ALT_INC2; // e.g. 3,1,4,2
      }
      else if (minTimeIndex == size - 2)
      {
        return NIFTI_SLICE_ALT_DEC2; // e.g. 2,4,1,3 or 5,2,4,1,3
      }
      else if (size >= 3 &&
               minTimeIndex == 0 &&
               sliceTimingSiemens_[1] < sliceTimingSiemens_[2])
      {
        return NIFTI_SLICE_SEQ_INC; // e.g. 1,2,3,4
      }
      else if (size >= 3 &&
               minTimeIndex == 0 &&
               sliceTimingSiemens_[1] > sliceTimingSiemens_[2])
      {
        return NIFTI_SLICE_ALT_INC; //e.g. 1,3,2,4
      }
      else if (size >= 4 &&
               minTimeIndex == size - 1 &&
               sliceTimingSiemens_[size - 3] > sliceTimingSiemens_[size - 2])
      {
        return NIFTI_SLICE_SEQ_DEC; //e.g. 4,3,2,1 or 5,4,3,2,1
      }
      else if (size >= 4 &&
               minTimeIndex == (size - 1) &&
               sliceTimingSiemens_[size - 3] < sliceTimingSiemens_[size - 2])
      {
        return NIFTI_SLICE_ALT_DEC;
      }
      else
      {
        return NIFTI_SLICE_UNKNOWN;
      }
    }
    else
    {
      return NIFTI_SLICE_UNKNOWN;
    }
  }


  bool InputDicomInstance::LookupRepetitionTime(double& value) const
  {
    std::vector<double> repetitionTime;
    if (NeuroToolbox::ParseVector(repetitionTime, *tags_, DICOM_TAG_REPETITION_TIME))
    {
      if (repetitionTime.size() == 1)
      {
        value = repetitionTime[0];
        return true;
      }
      else
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
    }
    else
    {
      return false;
    }
  }


  void InputDicomInstance::ExtractSiemensMosaicSlices(std::list<Slice>& slices,
                                                      size_t instanceIndexInCollection) const
  {
    // https://github.com/malaterre/GDCM/blob/master/Source/MediaStorageAndFileFormat/gdcmSplitMosaicFilter.cxx

    uint32_t numberOfImagesInMosaic;
    if (GetImageInformation().GetNumberOfFrames() != 1 ||
        !GetCSAHeader().ParseUnsignedInteger32(numberOfImagesInMosaic, CSA_NUMBER_OF_IMAGES_IN_MOSAIC) ||
        numberOfImagesInMosaic == 0)
    {
      ExtractGenericSlices(slices, instanceIndexInCollection);
      return;
    }
  
    const unsigned int countPerAxis = static_cast<unsigned int>(std::ceil(sqrtf(numberOfImagesInMosaic)));

    if (GetImageInformation().GetWidth() % countPerAxis != 0 ||
        GetImageInformation().GetHeight() % countPerAxis != 0 ||
        numberOfImagesInMosaic > (countPerAxis * countPerAxis))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    // https://nipy.org/nibabel/dicom/dicom_mosaic.html#dicom-orientation-for-mosaic

    const unsigned int width = GetImageInformation().GetWidth() / countPerAxis;
    const unsigned int height = GetImageInformation().GetHeight() / countPerAxis;

    mat44 sto_xyz;
    
    for (uint8_t i = 0; i < 3; i++)
    {
      sto_xyz.m[i][0] = GetAxisX(i) * GetPixelSpacingX();
      sto_xyz.m[i][1] = GetAxisY(i) * GetPixelSpacingY();
      sto_xyz.m[i][2] = GetNormal(i) * GetVoxelSpacingZ();
      sto_xyz.m[i][3] = GetImagePositionPatient(i);
    }

    {
      const double mc = static_cast<double>(GetImageInformation().GetWidth());
      const double mr = static_cast<double>(GetImageInformation().GetHeight());
      const double nc = static_cast<double>(width);
      const double nr = static_cast<double>(height);
      const double dc = (mc - nc) / 2.0;
      const double dr = (mr - nr) / 2.0;
      sto_xyz.m[0][3] = GetImagePositionPatient(0) + sto_xyz.m[0][0] * dc + sto_xyz.m[0][1] * dr;
      sto_xyz.m[1][3] = GetImagePositionPatient(1) + sto_xyz.m[1][0] * dc + sto_xyz.m[1][1] * dr;
      sto_xyz.m[2][3] = GetImagePositionPatient(2) + sto_xyz.m[2][0] * dc + sto_xyz.m[2][1] * dr;
    }

    std::vector<double> sliceNormalVector;
    if (!GetCSAHeader().GetTag(CSA_SLICE_NORMAL_VECTOR).ParseVector(sliceNormalVector) ||
        sliceNormalVector.size() != 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
    
    {
      unsigned int pos = 0;
      for (unsigned int y = 0; y < countPerAxis; y++)
      {
        for (unsigned int x = 0; x < countPerAxis; x++, pos++)
        {
          if (pos < numberOfImagesInMosaic)
          {
            double z = GetVoxelSpacingZ() * static_cast<double>(pos);
            
            slices.push_back(Slice(instanceIndexInCollection, 0 /* frame index */, GetInstanceNumber(),
                                   x * width, y * height, width, height,
                                   sto_xyz.m[0][3] + z * sliceNormalVector[0],
                                   sto_xyz.m[1][3] + z * sliceNormalVector[1],
                                   sto_xyz.m[2][3] + z * sliceNormalVector[2],
                                   sliceNormalVector[0], sliceNormalVector[1], sliceNormalVector[2]));

            if (HasAcquisitionTime())
            {
              slices.back().SetAcquisitionTime(GetAcquisitionTime());
            }
          }
        }
      }
    }
  }


  void InputDicomInstance::ExtractUIHSlices(std::list<Slice>& slices,
                                            size_t instanceIndexInCollection) const
  {
    // https://github.com/rordenlab/dcm2niix/issues/225#issuecomment-422645183
    const double total = static_cast<double>(GetUIHFrameSequenceSize());
    const double dcols = std::ceil(sqrt(total));
    if (dcols <= 0 ||
        GetImageInformation().GetNumberOfFrames() != 1)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    const unsigned int cols = static_cast<unsigned int>(dcols);
    if (GetImageInformation().GetWidth() % cols != 0 ||
        GetUIHFrameSequenceSize() % cols != 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    const unsigned int rows = static_cast<unsigned int>(GetUIHFrameSequenceSize() / cols);
    assert(cols * rows == GetUIHFrameSequenceSize());

    if (GetImageInformation().GetHeight() % rows != 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    unsigned int width = GetImageInformation().GetWidth() / cols;
    unsigned int height = GetImageInformation().GetHeight() / rows;

    unsigned int pos = 0;
    for (unsigned int y = 0; y < rows; y++)
    {
      for (unsigned int x = 0; x < cols; x++, pos++)
      {
        std::vector<double> origin;
        std::vector<double> acquisitionTime;
        if (!NeuroToolbox::ParseVector(origin, GetUIHFrameSequenceItem(pos), Orthanc::DICOM_TAG_IMAGE_POSITION_PATIENT) ||
            !NeuroToolbox::ParseVector(acquisitionTime, GetUIHFrameSequenceItem(pos), Orthanc::DICOM_TAG_ACQUISITION_TIME) ||
            origin.size() != 3 ||
            acquisitionTime.size() != 1)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
        }
        else
        {
          slices.push_back(Slice(instanceIndexInCollection, 0 /* frame index */, GetInstanceNumber(),
                                 x * width, y * height, width, height,
                                 origin[0], origin[1], origin[2],
                                 GetNormal(0), GetNormal(1), GetNormal(2)));

          slices.back().SetAcquisitionTime(acquisitionTime[0]);
        }
      }
    }
  }
  
  
  void InputDicomInstance::ExtractGenericSlices(std::list<Slice>& slices,
                                                size_t instanceIndexInCollection) const
  {
    unsigned int numberOfFrames = GetImageInformation().GetNumberOfFrames();
    
    if (numberOfFrames != 1)
    {
      // This is the case of RT-DOSE
      std::vector<double> frameOffset;
      if (!NeuroToolbox::ParseVector(frameOffset, GetTags(), Orthanc::DICOM_TAG_GRID_FRAME_OFFSET_VECTOR) ||
          frameOffset.size() != numberOfFrames)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                        "Cannot detect the 3D coordinates in a multiframe instance");
      }
      else
      {
        for (unsigned int frame = 0; frame < numberOfFrames; frame++)
        {
          double z = frameOffset[frame];
          slices.push_back(Slice(instanceIndexInCollection, frame, GetInstanceNumber(),
                                 0, 0, GetImageInformation().GetWidth(),
                                 GetImageInformation().GetHeight(),
                                 GetImagePositionPatient(0) + z * GetNormal(0),
                                 GetImagePositionPatient(1) + z * GetNormal(1),
                                 GetImagePositionPatient(2) + z * GetNormal(2),
                                 GetNormal(0), GetNormal(1), GetNormal(2)));

          if (HasAcquisitionTime())
          {
            slices.back().SetAcquisitionTime(GetAcquisitionTime());
          }
        }
      }
    }
    else
    {
      slices.push_back(Slice(instanceIndexInCollection, 0 /* single frame */, GetInstanceNumber(),
                             0, 0, GetImageInformation().GetWidth(),
                             GetImageInformation().GetHeight(),
                             GetImagePositionPatient(0),
                             GetImagePositionPatient(1),
                             GetImagePositionPatient(2),
                             GetNormal(0), GetNormal(1), GetNormal(2)));

      if (HasAcquisitionTime())
      {
        slices.back().SetAcquisitionTime(GetAcquisitionTime());
      }
    }
  }


  void InputDicomInstance::ExtractSlices(std::list<Slice>& slices,
                                         size_t instanceIndexInCollection) const
  {
    if (GetManufacturer() == Manufacturer_Siemens &&
        GetCSAHeader().HasTag(CSA_NUMBER_OF_IMAGES_IN_MOSAIC))
    {
      ExtractSiemensMosaicSlices(slices, instanceIndexInCollection);
    }
    else if (GetManufacturer() == Manufacturer_UIH &&
             GetUIHFrameSequenceSize() > 0)
    {
      ExtractUIHSlices(slices, instanceIndexInCollection);
    }
    else
    {
      ExtractGenericSlices(slices, instanceIndexInCollection);
    }
  }


  size_t InputDicomInstance::ComputeInstanceNiftiBodySize() const
  {
    Orthanc::PixelFormat format;
    if (!GetImageInformation().ExtractPixelFormat(format, true))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    size_t bytesPerPixel = Orthanc::GetBytesPerPixel(format);

    std::list<Neuro::Slice> slices;
    ExtractSlices(slices, 0 /* unused */);

    unsigned int niftiBodySize = 0;
    for (std::list<Neuro::Slice>::const_iterator it = slices.begin(); it != slices.end(); ++it)
    {
      niftiBodySize += bytesPerPixel * it->GetWidth() * it->GetHeight();
    }

    return niftiBodySize;
  }
}

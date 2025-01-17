/**
 * Neuroimaging plugin for Orthanc
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#pragma once

#include "CSAHeader.h"
#include "NeuroEnumerations.h"
#include "Slice.h"

#include <DicomFormat/DicomImageInformation.h>
#include <DicomFormat/DicomMap.h>

#if !defined(ORTHANC_ENABLE_DCMTK)
#  error The macro ORTHANC_ENABLE_DCMTK must be defined
#endif

#if ORTHANC_ENABLE_DCMTK == 1
#  include <DicomParsing/ParsedDicomFile.h>
#endif


namespace Neuro
{
  class InputDicomInstance : public boost::noncopyable
  {
  private:
    // Inputs
    std::unique_ptr<Orthanc::DicomMap>  tags_;
    CSAHeader                           csa_;
    std::vector<Orthanc::DicomMap*>     uihFrameSequence_;

    // Extracted values
    std::unique_ptr<Orthanc::DicomImageInformation>  info_;    
    int32_t                             instanceNumber_;
    Manufacturer                        manufacturer_;
    Modality                            modality_;
    bool                                hasEchoTime_;
    double                              echoTime_;
    bool                                hasAcquisitionTime_;
    double                              acquisitionTime_;

    // Parsed values
    std::vector<double>                 imagePositionPatient_;
    std::vector<double>                 imageOrientationPatient_;
    std::vector<double>                 normal_;
    double                              pixelSpacingX_;
    double                              pixelSpacingY_;
    double                              voxelSpacingZ_;
    double                              rescaleSlope_;
    double                              rescaleIntercept_;
    PhaseEncodingDirection              phaseEncodingDirection_;
    std::vector<double>                 sliceTimingSiemens_;

    void ParseImagePositionPatient();
    
    void ParseImageOrientationPatient();
    
    void ParsePixelSpacing();
    
    void ParseVoxelSpacingZ();
    
    void ParseRescale();

    void ParsePhaseEncodingDirection();
    
    void ParseSliceTimingSiemens();

    void Setup();

    double GetImageOrientationPatient(unsigned int index) const;
    
#if ORTHANC_ENABLE_DCMTK == 1
    void LoadDicom(const Orthanc::ParsedDicomFile& dicom);
#endif

    void ExtractSiemensMosaicSlices(std::list<Slice>& slices,
                                    size_t instanceIndexInCollection) const;

    void ExtractUIHSlices(std::list<Slice>& slices,
                          size_t instanceIndexInCollection) const;

    void ExtractGenericSlices(std::list<Slice>& slices,
                              size_t instanceIndexInCollection) const;

  public:
    explicit InputDicomInstance(const Orthanc::DicomMap& tags) :
      tags_(tags.Clone())
    {
      Setup();
    }

#if ORTHANC_ENABLE_DCMTK == 1
    explicit InputDicomInstance(const Orthanc::ParsedDicomFile& dicom)
    {
      LoadDicom(dicom);
    }
#endif

    ~InputDicomInstance();

    const Orthanc::DicomMap& GetTags() const
    {
      return *tags_;
    }

    const CSAHeader& GetCSAHeader() const
    {
      return csa_;
    }
    
    CSAHeader& GetCSAHeader()
    {
      return csa_;
    }

    void AddUIHFrameSequenceItem(const Orthanc::DicomMap& item)
    {
      uihFrameSequence_.push_back(item.Clone());
    }

    size_t GetUIHFrameSequenceSize() const
    {
      return uihFrameSequence_.size();
    }

    const Orthanc::DicomMap& GetUIHFrameSequenceItem(size_t index) const;

    const Orthanc::DicomImageInformation& GetImageInformation() const
    {
      return *info_;
    }

    int32_t GetInstanceNumber() const
    {
      return instanceNumber_;
    }

    Manufacturer GetManufacturer() const
    {
      return manufacturer_;
    }

    Modality GetModality() const
    {
      return modality_;
    }

    bool HasEchoTime() const
    {
      return hasEchoTime_;
    }

    double GetEchoTime() const;

    bool HasAcquisitionTime() const
    {
      return hasAcquisitionTime_;
    }

    double GetAcquisitionTime() const;

    double GetImagePositionPatient(unsigned int index) const;
    
    double GetAxisX(unsigned int index) const;

    double GetAxisY(unsigned int index) const;

    double GetNormal(unsigned int index) const;

    double GetPixelSpacingX() const
    {
      return pixelSpacingX_;
    }

    double GetPixelSpacingY() const
    {
      return pixelSpacingY_;
    }

    double GetVoxelSpacingZ() const
    {
      return voxelSpacingZ_;
    }

    double GetRescaleSlope() const
    {
      return rescaleSlope_;
    }

    double GetRescaleIntercept() const
    {
      return rescaleIntercept_;
    }
    
    PhaseEncodingDirection GetPhaseEncodingDirection() const
    {
      return phaseEncodingDirection_;
    }
    
    unsigned int GetMultiBandFactor() const;
    
    int DetectSiemensSliceCode() const;

    bool LookupRepetitionTime(double& value) const;

    void ExtractSlices(std::list<Slice>& slices,
                       size_t instanceIndexInCollection) const;

    size_t ComputeInstanceNiftiBodySize() const;
  };
}

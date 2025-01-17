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


#include "DicomInstancesCollection.h"

#include "NeuroToolbox.h"

#include <OrthancException.h>
#include <SerializationToolbox.h>

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cassert>

static const std::string CSA_PHASE_ENCODING_DIRECTION_POSITIVE = "PhaseEncodingDirectionPositive";


namespace Neuro
{
  namespace
  {
    struct SliceComparator
    {
      bool operator() (const Slice& a,
                       const Slice& b) const
      {
        if (a.GetProjectionAlongNormal() < b.GetProjectionAlongNormal())
        {
          return true;
        }
        else if (a.GetProjectionAlongNormal() > b.GetProjectionAlongNormal())
        {
          return false;
        }
        else
        {
          return a.GetInstanceNumber() < b.GetInstanceNumber();
        }
      }
    };
  }


  namespace
  {
    class DescriptionWriter : public boost::noncopyable
    {
    private:
      std::list<std::string>  content_;
      std::set<std::string>   index_;

    public:
      void AddString(const std::string& key,
                     const std::string& value)
      {
        if (index_.find(key) == index_.end())
        {
          content_.push_back(key + "=" + value);
          index_.insert(key);
        }
        else
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls,
                                          "The description already has this key: " + key);
        }
      }

      void AddDouble(const std::string& key,
                     double value,
                     const std::string& format)
      {
        char buf[64];
        sprintf(buf, format.c_str(), value);
        AddString(key, buf);
      }

      void Write(nifti_image& nifti) const
      {
        std::string s;

        for (std::list<std::string>::const_iterator it = content_.begin(); it != content_.end(); ++it)
        {
          if (!s.empty())
          {
            s += ';';
          }

          s += *it;
        }

        strncpy(nifti.descrip, s.c_str(), sizeof(nifti.descrip) - 1);
      }
    };
  }
  

  void DicomInstancesCollection::ExtractSlices(std::list<Slice>& slices) const
  {
    for (size_t i = 0; i < GetSize(); i++)
    {
      GetInstance(i).ExtractSlices(slices, i);
    }
  }


  static void Compute3DOrientation(nifti_image& nifti,
                                   PhaseEncodingDirection phaseEncoding)
  {
    nifti.sto_xyz.m[3][0] = 0;
    nifti.sto_xyz.m[3][1] = 0;
    nifti.sto_xyz.m[3][2] = 0;
    nifti.sto_xyz.m[3][3] = 1;

    float qb, qc, qd, qx, qy, qz, dx, dy, dz, qfac;
    nifti_mat44_to_quatern(nifti.sto_xyz, &qb, &qc, &qd, &qx, &qy, &qz, &dx, &dy, &dz, &qfac);

    // Normalize the quaternion to positive components
    if (qb <= std::numeric_limits<double>::epsilon() &&
        qc <= std::numeric_limits<double>::epsilon() &&
        qd <= std::numeric_limits<double>::epsilon())
    {
      qb = -qb;
      qc = -qc;
      qd = -qd;
    }
    
    nifti.quatern_b = qb;
    nifti.quatern_c = qc;
    nifti.quatern_d = qd;
    nifti.qoffset_x = qx;
    nifti.qoffset_y = qy;
    nifti.qoffset_z = qz;
    nifti.qfac = qfac;
    nifti.dx = dx;
    nifti.dy = dy;
    nifti.dz = dz;
    nifti.pixdim[0] = qfac;
    nifti.pixdim[1] = dx;
    nifti.pixdim[2] = dy;
    nifti.pixdim[3] = dz;

    // https://github.com/rordenlab/dcm2niix/blob/master/console/nii_dicom.cpp
    // Function "headerDcm2Nii2()"
    switch (phaseEncoding)
    {
      case PhaseEncodingDirection_Row:
        nifti.phase_dim = 1;
        nifti.freq_dim = 2;
        nifti.slice_dim = 3;
        break;
        
      case PhaseEncodingDirection_Column:
        nifti.phase_dim = 2;
        nifti.freq_dim = 1;
        nifti.slice_dim = 3;
        break;

      case PhaseEncodingDirection_None:
        nifti.phase_dim = 0;
        nifti.freq_dim = 0;
        nifti.slice_dim = 0;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }
  }


  static void ConvertDicomToNiftiOrientation(nifti_image& nifti)
  {
    for (int c = 0; c < 2; c++)
    {
      for (int r = 0; r < 4; r++)
      {
        nifti.sto_xyz.m[c][r] = -nifti.sto_xyz.m[c][r];
      }
    }
    
    // "nii_flipY()" in dcm2niix
    
    nifti.sto_xyz.m[0][3] = nifti.sto_xyz.m[0][1] * static_cast<double>(nifti.ny - 1) + nifti.sto_xyz.m[0][3];
    nifti.sto_xyz.m[1][3] = nifti.sto_xyz.m[1][1] * static_cast<double>(nifti.ny - 1) + nifti.sto_xyz.m[1][3];
    nifti.sto_xyz.m[2][3] = nifti.sto_xyz.m[2][1] * static_cast<double>(nifti.ny - 1) + nifti.sto_xyz.m[2][3];

    for (int r = 0; r < 3; r++)
    {
      nifti.sto_xyz.m[r][1] = -nifti.sto_xyz.m[r][1];
    }
  }
  

  static void InitializeNiftiHeader(nifti_image& nifti,
                                    const InputDicomInstance& instance)
  {
    memset(&nifti, 0, sizeof(nifti));
    nifti.scl_slope = instance.GetRescaleSlope();
    nifti.scl_inter = instance.GetRescaleIntercept();
    nifti.xyz_units = NIFTI_UNITS_MM;
    nifti.time_units = NIFTI_UNITS_SEC;
    nifti.nifti_type = 1;  // NIFTI-1 (1 file)
    nifti.qform_code = NIFTI_XFORM_SCANNER_ANAT;
    nifti.sform_code = NIFTI_XFORM_SCANNER_ANAT;

    Orthanc::PixelFormat format;
    if (!instance.GetImageInformation().ExtractPixelFormat(format, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
    
    switch (format)
    {
      case Orthanc::PixelFormat_Grayscale16:
        // In this situation, dcm2niix uses "NIFTI_TYPE_INT16", which is wrong
        nifti.datatype = NIFTI_TYPE_UINT16;
        nifti.nbyper = 2;
        break;

      case Orthanc::PixelFormat_SignedGrayscale16:
        nifti.datatype = NIFTI_TYPE_INT16;
        nifti.nbyper = 2;
        break;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }
  }
  
  
  unsigned int DicomInstancesCollection::GetMultiBandFactor() const
  {
    unsigned int mb = 0;
    
    for (size_t i = 0; i < GetSize(); i++)
    {
      mb = std::max(mb, GetInstance(i).GetMultiBandFactor());
    }

    return mb;
  }
  

  void DicomInstancesCollection::WriteDescription(nifti_image& nifti,
                                                  const std::vector<Slice>& sortedSlices) const
  {
    bool hasAcquisitionTime = false;
    double lowestAcquisitionTime, highestAcquisitionTime;

    for (size_t i = 0; i < sortedSlices.size(); i++)
    {
      if (sortedSlices[i].HasAcquisitionTime())
      {
        double t = sortedSlices[i].GetAcquisitionTime();
        
        if (hasAcquisitionTime)
        {
          lowestAcquisitionTime = std::min(lowestAcquisitionTime, t);
          highestAcquisitionTime = std::max(highestAcquisitionTime, t);
        }
        else
        {
          hasAcquisitionTime = true;
          lowestAcquisitionTime = highestAcquisitionTime = t;
        }
      }
    }
    
    DescriptionWriter description;

    const InputDicomInstance& firstInstance = GetInstance(sortedSlices[0].GetInstanceIndexInCollection());
    
    if (firstInstance.HasEchoTime())
    {
      description.AddDouble("TE", firstInstance.GetEchoTime(), "%.2g");
    }

    if (hasAcquisitionTime)
    {
      if (firstInstance.GetModality() == Modality_PET)
      {
        description.AddDouble("Time", highestAcquisitionTime, "%.3f");
      }
      else
      {
        description.AddDouble("Time", lowestAcquisitionTime, "%.3f");
      }
    }

    uint32_t phaseEncodingDirectionPositive;
    if (firstInstance.GetCSAHeader().ParseUnsignedInteger32(phaseEncodingDirectionPositive, CSA_PHASE_ENCODING_DIRECTION_POSITIVE))
    {
      description.AddString("phase", boost::lexical_cast<std::string>(phaseEncodingDirectionPositive));
    }
      
    const unsigned int multiBandFactor = GetMultiBandFactor();
    if (multiBandFactor > 1)
    {
      description.AddString("mb", boost::lexical_cast<std::string>(multiBandFactor));
    }

    description.Write(nifti);
  }
     
  
  DicomInstancesCollection::~DicomInstancesCollection()
  {
    for (size_t i = 0; i < instances_.size(); i++)
    {
      assert(instances_[i] != NULL);
      delete instances_[i];
    }
  }
  

  void DicomInstancesCollection::AddInstance(InputDicomInstance* instance,
                                             const std::string& orthancId)
  {
    if (instance == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      instances_.push_back(instance);
      orthancIds_.push_back(orthancId);
    }
  }
    

  const InputDicomInstance& DicomInstancesCollection::GetInstance(size_t index) const
  {
    assert(orthancIds_.size() == instances_.size());
    if (index >= instances_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      assert(instances_[index] != NULL);
      return *instances_[index];
    }    
  }


  const std::string& DicomInstancesCollection::GetOrthancId(size_t index) const
  {
    assert(orthancIds_.size() == instances_.size());
    if (index >= instances_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return orthancIds_[index];
    }    
  }
  

  void DicomInstancesCollection::CreateNiftiHeader(nifti_image& nifti /* out */,
                                                   std::vector<Slice>& slices /* out */) const
  {
    // TODO: Sanity check - Verify that all the instances have the
    // same pixel spacing, the same sizes, the same modality, are parallel
      
    std::list<Slice> unsortedSlices;
    ExtractSlices(unsortedSlices);

    std::vector<Slice> sortedSlices;
    sortedSlices.reserve(unsortedSlices.size());

    for (std::list<Slice>::const_iterator it = unsortedSlices.begin();
         it != unsortedSlices.end(); ++it)
    {
      sortedSlices.push_back(*it);
    }
    
    SliceComparator comparator;
    std::sort(sortedSlices.begin(), sortedSlices.end(), comparator);

    size_t numberOfAcquisitions = 1;
    while (numberOfAcquisitions < sortedSlices.size() &&
           NeuroToolbox::IsNear(sortedSlices[0].GetProjectionAlongNormal(),
                                sortedSlices[numberOfAcquisitions].GetProjectionAlongNormal(), 0.0001))
    {
      numberOfAcquisitions++;
    }

    if (sortedSlices.size() % numberOfAcquisitions != 0)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange, "Inconsistent number of acquisitions");
    }

    size_t acquisitionLength = sortedSlices.size() / numberOfAcquisitions;

    for (size_t i = 1; i < acquisitionLength; i++)
    {
      if (NeuroToolbox::IsNear(sortedSlices[(i - 1) * numberOfAcquisitions].GetProjectionAlongNormal(),
                               sortedSlices[i * numberOfAcquisitions].GetProjectionAlongNormal(), 0.0001))
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange, "Ambiguity in the 3D locations");
      }
    }
      
    for (size_t i = 0; i < acquisitionLength; i++)
    {
      for (size_t j = 1; j < numberOfAcquisitions; j++)
      {
        if (sortedSlices[i * numberOfAcquisitions].GetInstanceNumber() ==
            sortedSlices[i * numberOfAcquisitions + j].GetInstanceNumber())
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange, "Ambiguity in the instance numbers");
        }

        if (!NeuroToolbox::IsNear(sortedSlices[i * numberOfAcquisitions].GetProjectionAlongNormal(),
                                  sortedSlices[i * numberOfAcquisitions + j].GetProjectionAlongNormal(), 0.0001))
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange, "Ambiguity in the 3D locations");
        }
      }
    }

    const InputDicomInstance& firstInstance = GetInstance(sortedSlices[0].GetInstanceIndexInCollection());
    
    InitializeNiftiHeader(nifti, firstInstance);

    nifti.dim[1] = nifti.nx = sortedSlices[0].GetWidth();
    nifti.dim[2] = nifti.ny = sortedSlices[0].GetHeight();

    nifti.pixdim[1] = nifti.dx = firstInstance.GetPixelSpacingX();
    nifti.pixdim[2] = nifti.dy = firstInstance.GetPixelSpacingY();

    if (numberOfAcquisitions >= sortedSlices.size())
    {
      nifti.pixdim[3] = nifti.dz = firstInstance.GetVoxelSpacingZ();
    }
    else
    {
      nifti.pixdim[3] = nifti.dz = (sortedSlices[numberOfAcquisitions].GetProjectionAlongNormal() -
                                    sortedSlices[0].GetProjectionAlongNormal());
    }

    assert(nifti.dz > 0);
      
    if (acquisitionLength == 1 ||
        numberOfAcquisitions == 1)
    {
      nifti.dim[0] = nifti.ndim = 3;
      nifti.dim[3] = nifti.nz = std::max(numberOfAcquisitions, acquisitionLength);
    }
    else
    {
      nifti.dim[0] = nifti.ndim = 4;
      nifti.dim[3] = nifti.nz = acquisitionLength;
      nifti.dim[4] = nifti.nt = numberOfAcquisitions;

      bool hasDt = false;
        
      if (firstInstance.GetManufacturer() == Manufacturer_Philips &&
          sortedSlices[0].HasAcquisitionTime())
      {
        // Check out "trDiff0" in "nii_dicom_batch.cpp"
        double a = NeuroToolbox::FixDicomTime(sortedSlices[0].GetAcquisitionTime());
        double maxTimeDifference = 0;
          
        for (size_t i = 1; i < sortedSlices.size(); i++)
        {
          if (sortedSlices[i].HasAcquisitionTime())
          {
            double b = NeuroToolbox::FixDicomTime(sortedSlices[i].GetAcquisitionTime());
            maxTimeDifference = std::max(maxTimeDifference, b - a);
          }
        }

        if (!NeuroToolbox::IsNear(maxTimeDifference, 0))
        {
          hasDt = true;
          nifti.pixdim[4] = nifti.dt = static_cast<float>(maxTimeDifference / (nifti.nt - 1.0));
        }
      }

      if (!hasDt)
      {
        double repetitionTime;
        if (firstInstance.LookupRepetitionTime(repetitionTime))
        {
          float r = static_cast<float>(repetitionTime / 1000.0);  // Conversion to seconds
          nifti.pixdim[4] = nifti.dt = r;
          hasDt = true;
        }
      }

      if (!hasDt)
      {
        nifti.pixdim[4] = nifti.dt = 1;
      }
    }
      
    nifti.nvox = 1;
    for (int i = 0; i < nifti.dim[0]; i++)
    {
      nifti.nvox *= nifti.dim[i + 1];
    }
    
    nifti.slice_code = firstInstance.DetectSiemensSliceCode();
      
    for (uint8_t i = 0; i < 3; i++)
    {
      nifti.sto_xyz.m[i][0] = firstInstance.GetAxisX(i) * nifti.dx;
      nifti.sto_xyz.m[i][1] = firstInstance.GetAxisY(i) * nifti.dy;
      nifti.sto_xyz.m[i][2] = sortedSlices[0].GetNormal(i) * nifti.dz;
      nifti.sto_xyz.m[i][3] = sortedSlices[0].GetOrigin(i);
    }

    ConvertDicomToNiftiOrientation(nifti);

    Compute3DOrientation(nifti, firstInstance.GetPhaseEncodingDirection());

    WriteDescription(nifti, sortedSlices);

    slices.reserve(sortedSlices.size());
    for (size_t j = 0; j < numberOfAcquisitions; j++)
    {
      for (size_t i = 0; i < acquisitionLength; i++)
      {
        slices.push_back(sortedSlices[i * numberOfAcquisitions + j]);
      }
    }

    assert(slices.size() == sortedSlices.size());
  }
}

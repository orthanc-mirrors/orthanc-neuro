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

#include <DicomFormat/DicomMap.h>


namespace Neuro
{
  static const Orthanc::DicomTag DICOM_TAG_SIEMENS_CSA_HEADER(0x0029, 0x1010);
  static const Orthanc::DicomTag DICOM_TAG_UIH_MR_VFRAME_SEQUENCE(0x0065, 0x1051); // https://github.com/rordenlab/dcm2niix/issues/225

  class NeuroToolbox
  {
  private:
    NeuroToolbox()  // This is a pure static class
    {
    }
    
  public:
    static double FixDicomTime(double t);

    static bool IsNear(double a,
                       double b,
                       double threshold);

    static bool IsNear(double a,
                       double b);
    
    static bool ParseVector(std::vector<double>& target,
                            const Orthanc::DicomMap& dicom,
                            const Orthanc::DicomTag& tag);
    
    static void CrossProduct(std::vector<double>& target,
                             const std::vector<double>& u,
                             const std::vector<double>& v);
  };
}

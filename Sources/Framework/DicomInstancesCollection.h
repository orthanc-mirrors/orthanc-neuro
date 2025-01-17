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

#include "InputDicomInstance.h"

#include <nifti1_io.h>


namespace Neuro
{
  class DicomInstancesCollection : public boost::noncopyable
  {
  private:
    std::vector<InputDicomInstance*>  instances_;
    std::vector<std::string>          orthancIds_;

    unsigned int GetMultiBandFactor() const;

    void WriteDescription(nifti_image& nifti,
                          const std::vector<Slice>& sortedSlices) const;

  public:
    ~DicomInstancesCollection();

    void AddInstance(InputDicomInstance* instance,  // Takes ownership
                     const std::string& orthancId);
    
    size_t GetSize() const
    {
      return instances_.size();
    }

    const InputDicomInstance& GetInstance(size_t index) const;

    const std::string& GetOrthancId(size_t index) const;

    void ExtractSlices(std::list<Slice>& slices) const;

    void CreateNiftiHeader(nifti_image& nifti /* out */,
                           std::vector<Slice>& slices /* out */) const;
  };
}

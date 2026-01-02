/**
 * Neuroimaging plugin for Orthanc
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include <ChunkedBuffer.h>
#include <Images/ImageAccessor.h>

#include <nifti1_io.h>


namespace Neuro
{
  class NiftiWriter : public boost::noncopyable
  {
  private:
    bool                    hasHeader_;
    Orthanc::ChunkedBuffer  buffer_;

  public:
    NiftiWriter() :
      hasHeader_(false)
    {
    }
  
    void WriteHeader(const nifti_image& header);

    void AddSlice(const Orthanc::ImageAccessor& slice);

    void Flatten(std::string& target,
                 bool compress);
  };
}

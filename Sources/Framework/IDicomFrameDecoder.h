/**
 * Neuroimaging plugin for Orthanc
 * Copyright (C) 2021-2024 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#include "NiftiWriter.h"
#include "Slice.h"

#include <vector>

namespace Neuro
{
  class IDicomFrameDecoder : public boost::noncopyable
  {
  public:
    class IDecodedFrame : public boost::noncopyable
    {
    public:
      virtual ~IDecodedFrame()
      {
      }

      virtual void GetRegion(Orthanc::ImageAccessor& region,
                             unsigned int x,
                             unsigned int y,
                             unsigned int width,
                             unsigned int height) = 0;
    };
    
    virtual ~IDicomFrameDecoder()
    {
    }

    virtual IDecodedFrame* DecodeFrame(const Slice& slice) = 0;

    static void Apply(NiftiWriter& writer /* output */,
                      IDicomFrameDecoder& decoder,
                      const std::vector<Slice>& slices);
  };
}

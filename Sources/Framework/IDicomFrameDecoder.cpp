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


#include "IDicomFrameDecoder.h"

#include <OrthancException.h>


namespace Neuro
{
  void IDicomFrameDecoder::Apply(NiftiWriter& writer,
                                 IDicomFrameDecoder& decoder,
                                 const std::vector<Slice>& slices)
  {
    for (size_t i = 1; i < slices.size(); i++)
    {
      if (slices[0].GetWidth() != slices[i].GetWidth() ||
          slices[0].GetHeight() != slices[i].GetHeight())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented,
                                        "The slices have varying dimensions");
      }
    }
      
    size_t currentInstanceIndex;
    unsigned int currentFrameNumber;
    std::unique_ptr<IDecodedFrame> currentFrame;

    bool first = true;
    Orthanc::PixelFormat format;

    for (size_t i = 0; i < slices.size(); i++)
    {
      if (currentFrame.get() == NULL ||
          currentInstanceIndex != slices[i].GetInstanceIndexInCollection() ||
          currentFrameNumber != slices[i].GetFrameNumber())
      {
        currentFrame.reset(decoder.DecodeFrame(slices[i]));
        if (currentFrame.get() == NULL)
        {
          throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
        }
          
        currentInstanceIndex = slices[i].GetInstanceIndexInCollection();
        currentFrameNumber = slices[i].GetFrameNumber();
      }

      Orthanc::ImageAccessor region;
      currentFrame->GetRegion(region, slices[i].GetX(), slices[i].GetY(), slices[i].GetWidth(), slices[i].GetHeight());

      if (region.GetWidth() != slices[i].GetWidth() ||
          region.GetHeight() != slices[i].GetHeight())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
        
      if (first)
      {
        first = false;
        format = region.GetFormat();
      }

      if (region.GetFormat() != format)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat,
                                        "The slices have varying pixel formats");
      }

      writer.AddSlice(region);
    }
  }
}

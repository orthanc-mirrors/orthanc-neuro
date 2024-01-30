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


#include "PluginFrameDecoder.h"


namespace Neuro
{
  static Orthanc::PixelFormat Convert(OrthancPluginPixelFormat format)
  {
    switch (format)
    {
      case OrthancPluginPixelFormat_Grayscale16:
        return Orthanc::PixelFormat_Grayscale16;

      case OrthancPluginPixelFormat_SignedGrayscale16:
        return Orthanc::PixelFormat_SignedGrayscale16;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  class PluginFrameDecoder::DecodedFrame : public IDecodedFrame
  {
  private:
    std::unique_ptr<OrthancPlugins::OrthancImage>  frame_;
      
  public:
    explicit DecodedFrame(OrthancPlugins::OrthancImage* frame) :
      frame_(frame)
    {
      if (frame == NULL)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
      }
    }
      
    virtual void GetRegion(Orthanc::ImageAccessor& region,
                           unsigned int x,
                           unsigned int y,
                           unsigned int width,
                           unsigned int height)
    {
      Orthanc::ImageAccessor f;
      f.AssignReadOnly(Convert(frame_->GetPixelFormat()), frame_->GetWidth(),
                       frame_->GetHeight(), frame_->GetPitch(), frame_->GetBuffer());

      f.GetRegion(region, x, y, width, height);
    }
  };

    
  PluginFrameDecoder::IDecodedFrame* PluginFrameDecoder::DecodeFrame(const Slice& slice)
  {
    const std::string id = collection_.GetOrthancId(slice.GetInstanceIndexInCollection());

    if (id != currentInstanceId_)
    {
      OrthancPlugins::MemoryBuffer dicom;
      dicom.GetDicomInstance(id);
        
      currentInstance_.reset(new OrthancPlugins::DicomInstance(dicom.GetData(), dicom.GetSize()));
      currentInstanceId_ = id;
    }

    assert(currentInstance_.get() != NULL);
    return new DecodedFrame(currentInstance_->GetDecodedFrame(slice.GetFrameNumber()));
  }
}

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


#include "NiftiWriter.h"

#include <Compression/GzipCompressor.h>
#include <Images/Image.h>
#include <OrthancException.h>

#include <cassert>


namespace Neuro
{
  void NiftiWriter::WriteHeader(const nifti_image& header)
  {
    if (hasHeader_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else
    {
      nifti_image fixed;
      memcpy(&fixed, &header, sizeof(nifti_image));

      std::string empty(1, '\0');
      fixed.fname = &empty[0];
      fixed.iname = NULL;
      fixed.num_ext = 0;  // no extension
    
      nifti_set_iname_offset(&fixed);

      if (fixed.nifti_type != NIFTI_FTYPE_NIFTI1_1)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }
    
      nifti_1_header serialized = nifti_convert_nim2nhdr(&fixed);
      serialized.vox_offset = (348 + 4);  // (*)

      static const uint8_t nope[4] = { 0, 0, 0, 0 };

      assert(sizeof(serialized) == 348);
      buffer_.AddChunk(&serialized, sizeof(serialized));

      assert(sizeof(nope) == 4);
      buffer_.AddChunk(&nope, sizeof(nope));  // because of (*)

      hasHeader_ = true;
    }
  }


  void NiftiWriter::AddSlice(const Orthanc::ImageAccessor& slice)
  {
    if (!hasHeader_)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
    else if (slice.GetWidth() != 0 &&
             slice.GetHeight() != 0)
    {
      Orthanc::Image image(slice.GetFormat(), slice.GetWidth(), slice.GetHeight(),
                           true /* force minimal pitch, as no pitch is allowed in NIfTI */);

      const size_t rowSize = GetBytesPerPixel(image.GetFormat()) * image.GetWidth();
      if (rowSize != image.GetPitch())
      {
        // Should never happen because of minimal pitch
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }

      assert(image.GetPitch() <= slice.GetPitch());

      for (unsigned int y = 0; y < slice.GetHeight(); y++)
      {
        const uint8_t *source = reinterpret_cast<const uint8_t*>(slice.GetConstRow(y));
        uint8_t *target = reinterpret_cast<uint8_t*>(image.GetRow(image.GetHeight() - 1 - y));
        memcpy(target, source, rowSize);
      }

      buffer_.AddChunk(image.GetConstBuffer(), rowSize * image.GetHeight());
    }
  }


  void NiftiWriter::Flatten(std::string& target,
                            bool compress)
  {
    if (compress)
    {
      std::string uncompressed;
      buffer_.Flatten(uncompressed);

      Orthanc::GzipCompressor compressor;
      Orthanc::IBufferCompressor::Compress(target, compressor, uncompressed);
    }
    else
    {
      buffer_.Flatten(target);
    }
  }
}

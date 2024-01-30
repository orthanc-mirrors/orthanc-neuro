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


#include "BufferReader.h"

#include <OrthancException.h>

#include <string.h>


namespace Neuro
{
  void BufferReader::ReadBlock(void* target,
                               size_t size)
  {
    if (size > 0)
    {
      if (pos_ + size > size_)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }
      else
      {
        memcpy(target, data_ + pos_, size);
        pos_ += size;
      }
    }
  }


  void BufferReader::Setup(const void* data,
                           size_t size)
  {
    if (size != 0 &&
        data == NULL)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NullPointer);
    }
    else
    {
      data_ = reinterpret_cast<const uint8_t*>(data);
      size_ = size;
      pos_ = 0;
    }
  }
  

  BufferReader::BufferReader(const std::string& buffer)
  {
    Setup(buffer.empty() ? NULL : buffer.c_str(), buffer.size());
  }
  

  std::string BufferReader::ReadNullTerminatedString()
  {
    for (size_t i = pos_; i < size_; i++)
    {
      if (data_[i] == '\0')
      {
        size_t length = i - pos_;
        size_t start = pos_;
        pos_ = i + 1;
        return std::string(reinterpret_cast<const char*>(data_ + start), length);
      }
    }

    throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
  }


  std::string BufferReader::ReadBlock(size_t size)
  {
    std::string s;
    s.resize(size);

    if (size > 0)
    {
      ReadBlock(&s[0], size);
    }

    return s;
  }


  void BufferReader::Skip(size_t bytes)
  {
    if (pos_ + bytes <= size_)
    {
      pos_ += bytes;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }


  uint32_t BufferReader::ReadUInt32()
  {
    if (pos_ + 4 <= size_)
    {
      uint32_t value = ((static_cast<uint32_t>(data_[pos_ + 3]) << 24) |
                        (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                        (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                        static_cast<uint32_t>(data_[pos_]));
      pos_ += 4;
      return value;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  }
}

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

#include <boost/noncopyable.hpp>
#include <stdint.h>
#include <string>

namespace Neuro
{
  class BufferReader : public boost::noncopyable
  {
  private:
    const uint8_t*  data_;
    size_t          size_;
    size_t          pos_;

    void ReadBlock(void* target,
                   size_t size);

    void Setup(const void* data,
               size_t size);

  public:
    explicit BufferReader(const std::string& buffer);
  
    BufferReader(void* data,
                 size_t size)
    {
      Setup(data, size);
    }

    std::string ReadNullTerminatedString();

    std::string ReadBlock(size_t size);

    void Skip(size_t bytes);

    uint32_t ReadUInt32();

    size_t GetPosition() const
    {
      return pos_;
    }
  };
}

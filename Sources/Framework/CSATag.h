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
#include <vector>


namespace Neuro
{
  class CSATag : public boost::noncopyable
  {
  private:
    std::string               vr_;
    std::vector<std::string>  values_;

  public:
    explicit CSATag(const std::string& vr) :
      vr_(vr)
    {
    }

    CSATag& AddValue(const std::string& value);

    const std::string& GetVR() const
    {
      return vr_;
    }

    const size_t GetSize() const
    {
      return values_.size();
    }

    const std::string& GetBinaryValue(size_t index) const;

    std::string GetStringValue(size_t index) const;

    bool ParseUnsignedInteger32(uint32_t& target,
                                size_t index) const;

    bool ParseDouble(double& target,
                     size_t index) const;

    bool ParseVector(std::vector<double>& target) const;
  };
}

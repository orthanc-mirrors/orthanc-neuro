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


#include "CSATag.h"

#include <OrthancException.h>
#include <SerializationToolbox.h>


namespace Neuro
{
  CSATag& CSATag::AddValue(const std::string& value)
  {
    values_.push_back(value);
    return *this;
  }


  const std::string& CSATag::GetBinaryValue(size_t index) const
  {
    if (index >= values_.size())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      return values_[index];
    }
  }


  std::string CSATag::GetStringValue(size_t index) const
  {
    const std::string& s = GetBinaryValue(index);

    // Crop the string at the first encountered '\0' value
    size_t pos = s.find('\0');
    if (pos == std::string::npos)
    {
      return s;
    }
    else
    {
      return s.substr(0, pos);
    }
  }


  bool CSATag::ParseUnsignedInteger32(uint32_t& target,
                                      size_t index) const
  {
    return Orthanc::SerializationToolbox::ParseUnsignedInteger32(target, GetStringValue(index));
  }


  bool CSATag::ParseDouble(double& target,
                           size_t index) const
  {
    return Orthanc::SerializationToolbox::ParseDouble(target, GetStringValue(index));
  }


  bool CSATag::ParseVector(std::vector<double>& target) const
  {
    target.resize(values_.size());

    for (size_t i = 0; i < values_.size(); i++)
    {
      if (!ParseDouble(target[i], i))
      {
        return false;
      }
    }

    return true;
  }
}

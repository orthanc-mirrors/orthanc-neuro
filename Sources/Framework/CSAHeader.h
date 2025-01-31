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

#include "CSATag.h"

#include <list>
#include <map>


namespace Neuro
{
  class CSAHeader : public boost::noncopyable
  {
  private:
    typedef std::map<std::string, CSATag*>  Content;

    Content  content_;

    void Clear();
  
  public:
    ~CSAHeader()
    {
      Clear();
    }

    void Load(const std::string& tag);

    bool HasTag(const std::string& name) const
    {
      return (content_.find(name) != content_.end());
    }

    const CSATag& GetTag(const std::string& name) const;
  
    void ListTags(std::list<std::string>& tags) const;

    bool ParseUnsignedInteger32(uint32_t& target,
                                const std::string& tagName) const;

    CSATag& AddTag(const std::string& name,
                   const std::string& vr);

    void AddValue(const std::string& tagName,
                  const std::string& value);
  };
}

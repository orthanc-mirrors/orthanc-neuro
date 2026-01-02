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


#include "CSAHeader.h"

#include "BufferReader.h"

#include <OrthancException.h>

#include <cassert>


namespace Neuro
{
  void CSAHeader::Clear()
  {
    for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      delete it->second;
    }
  }


  void CSAHeader::Load(const std::string& tag)
  {
    // https://nipy.org/nibabel/dicom/siemens_csa.html

    Clear();
  
    BufferReader reader(tag);
    
    if (reader.ReadUInt32() != 0x30315653)  // This is the "SV10" header
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    reader.ReadUInt32();  // Unused, often equals to 0x01020304

    const uint32_t n_tags = reader.ReadUInt32();
    if (n_tags == 0 ||
        n_tags > 128)
    {
      // This should in the range 1..128
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }

    if (reader.ReadUInt32() != 77)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
    }
  
    for (uint32_t i = 0; i < n_tags; i++)
    {
      const std::string name = reader.ReadNullTerminatedString();

      if (name.size() >= 63)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      reader.Skip(64 - name.size() - 1);

      const uint32_t vm = reader.ReadUInt32();

      const std::string vr = reader.ReadNullTerminatedString();
      if (vr.size() >= 4)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      reader.Skip(4 - vr.size() - 1);
    
      reader.ReadUInt32();  // "syngodt" = syngo.via data type
      const uint32_t nitems = reader.ReadUInt32();
      const uint32_t sync = reader.ReadUInt32();

      if (sync != 77 &&
          sync != 205)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat);
      }

      std::unique_ptr<CSATag> tag2(new CSATag(vr));

      for (uint32_t j = 0; j < nitems; j++)
      {
        reader.ReadUInt32();
        const uint32_t item_len = reader.ReadUInt32();
        reader.ReadUInt32();
        reader.ReadUInt32();

        if (vm == 0 ||
            j < vm)
        {
          tag2->AddValue(reader.ReadBlock(item_len));
        }
        else
        {
          reader.Skip(item_len);
        }

        // Set the stream position to the next 4 byte boundary
        if (reader.GetPosition() % 4 != 0)
        {
          reader.Skip(4 - reader.GetPosition() % 4);
        }
      }

      if (content_.find(name) != content_.end())
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                        "Tag is repeated in CSA header: " + name);
      }
      else
      {
        content_[name] = tag2.release();
      }
    }
  }


  const CSATag& CSAHeader::GetTag(const std::string& name) const
  {
    Content::const_iterator found = content_.find(name);

    if (found == content_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
    else
    {
      assert(found->second != NULL);
      return *found->second;
    }
  }


  void CSAHeader::ListTags(std::list<std::string>& tags) const
  {
    for (Content::const_iterator it = content_.begin(); it != content_.end(); ++it)
    {
      assert(it->second != NULL);
      tags.push_back(it->first);
    }
  }


  bool CSAHeader::ParseUnsignedInteger32(uint32_t& target,
                                         const std::string& tagName) const
  {
    Content::const_iterator found = content_.find(tagName);

    if (found == content_.end())
    {
      return false;
    }
    else if (found->second->GetSize() != 1)
    {
      return false;
    }
    else
    {
      return found->second->ParseUnsignedInteger32(target, 0);
    }
  }


  CSATag& CSAHeader::AddTag(const std::string& name,
                            const std::string& vr)
  {
    if (content_.find(name) != content_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange,
                                      "Tag already exists: " + name);
    }
    else
    {
      CSATag* tag = new CSATag(vr);
      content_[name] = tag;
      return *tag;
    }
  }


  void CSAHeader::AddValue(const std::string& tagName,
                           const std::string& value)
  {
    Content::iterator found = content_.find(tagName);

    if (found == content_.end())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem);
    }
    else
    {
      assert(found->second != NULL);
      found->second->AddValue(value);
    }
  }
}

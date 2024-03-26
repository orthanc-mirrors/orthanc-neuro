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


#include <gtest/gtest.h>

#include <Logging.h>


#if ORTHANC_ENABLE_DCMTK == 1
#  include "../Framework/InputDicomInstance.h"
#  include <DicomParsing/ParsedDicomFile.h>
#  include <SystemToolbox.h>

TEST(Dcmtk, DISABLED_DumpCSAHeader)
{
  std::string s;
  Orthanc::SystemToolbox::ReadFile(s, "49210406");
  
  Orthanc::ParsedDicomFile dicom(s);

  Neuro::InputDicomInstance instance(dicom);

  std::list<std::string> tags;
  instance.GetCSAHeader().ListTags(tags);

  for (std::list<std::string>::const_iterator it = tags.begin(); it != tags.end(); ++it)
  {
    const Neuro::CSATag& tag = instance.GetCSAHeader().GetTag(*it);
    
    printf("[%s] (%d) = ", it->c_str(), tag.GetSize());
    for (size_t i = 0; i < tag.GetSize(); i++)
    {
      printf("[%s] ", tag.GetStringValue(i).c_str());
    }
    
    printf("\n");
  }
}
#endif


int main(int argc, char **argv)
{
  Orthanc::Logging::Initialize();
  Orthanc::Logging::EnableInfoLevel(true);

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  Orthanc::Logging::Finalize();

  return result;
}

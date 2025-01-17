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


#include "NeuroToolbox.h"

#include <OrthancException.h>
#include <SerializationToolbox.h>
#include <Toolbox.h>

#include <boost/lexical_cast.hpp>


namespace Neuro
{
  double NeuroToolbox::FixDicomTime(double t)
  {
    // Switch from the "HHMMSS.frac" format of DICOM to the number of
    // seconds since midnight
    const double frac = t - std::floor(t);
    const unsigned int integral = static_cast<unsigned int>(std::floor(t));
    const unsigned int seconds = integral % 100;
    const unsigned int minutes = (integral / 100) % 100;
    const unsigned int hours = (integral / 10000);

    if (seconds >= 60 ||
        minutes >= 60 ||
        hours >= 24)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat, "Badly formatted DICOM time: " +
                                      boost::lexical_cast<std::string>(t));
    }
    else
    {
      return static_cast<double>(hours * 3600 + minutes * 60 + seconds) + frac;
    }
  }


  bool NeuroToolbox::IsNear(double a,
                            double b,
                            double threshold)
  {
    return fabs(a - b) <= threshold;
  }


  bool NeuroToolbox::IsNear(double a,
                            double b)
  {
    return IsNear(a, b, std::numeric_limits<float>::epsilon());
  }


  bool NeuroToolbox::ParseVector(std::vector<double>& target,
                                 const Orthanc::DicomMap& dicom,
                                 const Orthanc::DicomTag& tag)
  {
    std::string value;
    if (dicom.LookupStringValue(value, tag, false))
    {
      std::vector<std::string> tokens;
      Orthanc::Toolbox::TokenizeString(tokens, value, '\\');

      target.resize(tokens.size());
      for (size_t i = 0; i < tokens.size(); i++)
      {
        if (!Orthanc::SerializationToolbox::ParseDouble(target[i], tokens[i]))
        {
          return false;
        }
      }

      return true;
    }
    else
    {
      return false;
    }
  }


  void NeuroToolbox::CrossProduct(std::vector<double>& target,
                                  const std::vector<double>& u,
                                  const std::vector<double>& v)
  {
    if (u.size() != 3 ||
        v.size() != 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
    else
    {
      target.resize(3);
      target[0] = u[1] * v[2] - u[2] * v[1];
      target[1] = u[2] * v[0] - u[0] * v[2];
      target[2] = u[0] * v[1] - u[1] * v[0];
    }
  }
}

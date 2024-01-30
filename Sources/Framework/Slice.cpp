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


#include "Slice.h"

#include <OrthancException.h>


namespace Neuro
{
  Slice::Slice(size_t instanceIndexInCollection,
               unsigned int frameNumber,
               int32_t instanceNumber,
               unsigned int x,
               unsigned int y,
               unsigned int width,
               unsigned int height,
               double originX,
               double originY,
               double originZ,
               double normalX,
               double normalY,
               double normalZ) :
    instanceIndexInCollection_(instanceIndexInCollection),
    frameNumber_(frameNumber),
    instanceNumber_(instanceNumber),
    x_(x),
    y_(y),
    width_(width),
    height_(height),
    originX_(originX),
    originY_(originY),
    originZ_(originZ),
    normalX_(normalX),
    normalY_(normalY),
    normalZ_(normalZ),
    hasAcquisitionTime_(false),
    acquisitionTime_(0)  // dummy value
  {
    projectionAlongNormal_ = (originX * normalX + originY * normalY + originZ * normalZ);
  }


  double Slice::GetNormal(unsigned int i) const
  {
    switch (i)
    {
      case 0:
        return normalX_;

      case 1:
        return normalY_;

      case 2:
        return normalZ_;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }
  

  double Slice::GetOrigin(unsigned int i) const
  {
    switch (i)
    {
      case 0:
        return originX_;

      case 1:
        return originY_;

      case 2:
        return originZ_;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }


  void Slice::SetAcquisitionTime(double t)
  {
    hasAcquisitionTime_ = true;
    acquisitionTime_ = t;
  }


  double Slice::GetAcquisitionTime() const
  {
    if (hasAcquisitionTime_)
    {
      return acquisitionTime_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadSequenceOfCalls);
    }
  }
}

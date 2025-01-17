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

#include <stddef.h>
#include <stdint.h>


namespace Neuro
{
  class Slice
  {
  private:
    size_t        instanceIndexInCollection_;
    unsigned int  frameNumber_;
    int32_t       instanceNumber_;
    unsigned int  x_;
    unsigned int  y_;
    unsigned int  width_;
    unsigned int  height_;
    double        originX_;
    double        originY_;
    double        originZ_;
    double        normalX_;
    double        normalY_;
    double        normalZ_;
    bool          hasAcquisitionTime_;
    double        acquisitionTime_;
    double        projectionAlongNormal_;

  public:
    Slice(size_t instanceIndexInCollection,
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
          double normalZ);

    size_t GetInstanceIndexInCollection() const
    {
      return instanceIndexInCollection_;
    }

    unsigned int GetFrameNumber() const
    {
      return frameNumber_;
    }

    int32_t GetInstanceNumber() const
    {
      return instanceNumber_;
    }

    unsigned int GetX() const
    {
      return x_;
    }

    unsigned int GetY() const
    {
      return y_;
    }

    unsigned int GetWidth() const
    {
      return width_;
    }

    unsigned int GetHeight() const
    {
      return height_;
    }

    double GetNormal(unsigned int i) const;

    double GetOrigin(unsigned int i) const;

    double GetProjectionAlongNormal() const
    {
      return projectionAlongNormal_;
    }

    void SetAcquisitionTime(double t);

    bool HasAcquisitionTime() const
    {
      return hasAcquisitionTime_;
    }

    double GetAcquisitionTime() const;
  };
}

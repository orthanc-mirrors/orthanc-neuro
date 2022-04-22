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


#pragma once

#include "../Framework/IDicomFrameDecoder.h"
#include "../Framework/DicomInstancesCollection.h"

#include "../../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"


namespace Neuro
{
  class PluginFrameDecoder : public IDicomFrameDecoder
  {
  private:
    class DecodedFrame;

    const DicomInstancesCollection&                collection_;
    std::string                                    currentInstanceId_;
    std::unique_ptr<OrthancPlugins::DicomInstance> currentInstance_;
    
  public:
    explicit PluginFrameDecoder(const DicomInstancesCollection& collection) :
      collection_(collection)
    {
    }
    
    virtual IDecodedFrame* DecodeFrame(const Slice& slice) ORTHANC_OVERRIDE;
  };
}

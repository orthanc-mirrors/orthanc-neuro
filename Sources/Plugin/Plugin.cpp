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


#include "PluginFrameDecoder.h"

#include "../Framework/NeuroToolbox.h"
#include "../Framework/NiftiWriter.h"

#include <EmbeddedResources.h>

#include <Logging.h>
#include <SystemToolbox.h>

#define ORTHANC_PLUGIN_NAME  "neuro"


static void CreateNifti(std::string& target,
                        const Neuro::DicomInstancesCollection& collection,
                        bool compress)
{
  nifti_image nifti;
  std::vector<Neuro::Slice> slices;
  collection.CreateNiftiHeader(nifti, slices);

  Neuro::NiftiWriter writer;
  writer.WriteHeader(nifti);

  Neuro::PluginFrameDecoder decoder(collection);
  Neuro::IDicomFrameDecoder::Apply(writer, decoder, slices);
  
  writer.Flatten(target, compress);
}


static Neuro::InputDicomInstance* AcquireInstance(const std::string& instanceId)
{
#if 0
  /**
   * This version uses DCMTK. It should be avoided for performance, as
   * it requires reading the entire DICOM files from the disk, whereas
   * "OrthancPluginDicomInstanceToJson()" will only read the DICOM
   * files up to pixel data, and will take advantage of the caching
   * mechanisms implemented inside the Orthanc core.
   **/
  OrthancPlugins::MemoryBuffer dicom;
  dicom.GetDicomInstance(instanceId);

  Orthanc::ParsedDicomFile parsed(dicom.GetData(), dicom.GetSize());
  return new Neuro::InputDicomInstance(parsed);
  
#else
  Orthanc::DicomMap tags;

  {
    OrthancPlugins::OrthancString s;
    s.Assign(OrthancPluginDicomInstanceToJson(
               OrthancPlugins::GetGlobalContext(), instanceId.c_str(), OrthancPluginDicomToJsonFormat_Full,
               static_cast<OrthancPluginDicomToJsonFlags>(OrthancPluginDicomToJsonFlags_IncludePrivateTags |
                                                          OrthancPluginDicomToJsonFlags_IncludeUnknownTags |
                                                          OrthancPluginDicomToJsonFlags_StopAfterPixelData |
                                                          OrthancPluginDicomToJsonFlags_SkipGroupLengths), 0));

    Json::Value json;
    if (s.GetContent() == NULL ||
        !OrthancPlugins::ReadJson(json, s.GetContent()))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem, "Missing instance: " + instanceId);
    }
    
    tags.FromDicomAsJson(json);
  }

  std::unique_ptr<Neuro::InputDicomInstance> instance(new Neuro::InputDicomInstance(tags));
  
  switch (instance->GetManufacturer())
  {
    case Neuro::Manufacturer_Siemens:
    {
      std::string csa;
      if (OrthancPlugins::RestApiGetString(csa, "/instances/" + instanceId + "/content/" +
                                           Neuro::DICOM_TAG_SIEMENS_CSA_HEADER.Format(), false))
      {
        instance->GetCSAHeader().Load(csa);
      }
      break;
    }

    case Neuro::Manufacturer_UIH:
    {
      const std::string uri = "/instances/" + instanceId + "/content/" + Neuro::DICOM_TAG_UIH_MR_VFRAME_SEQUENCE.Format();
      
      Json::Value uih;
      if (OrthancPlugins::RestApiGet(uih, uri, false) &&
          uih.type() == Json::arrayValue)
      {
        for (Json::Value::ArrayIndex i = 0; i < uih.size(); i++)
        {
          Json::Value tags2;

          if (uih[i].type() != Json::stringValue ||
              !OrthancPlugins::RestApiGet(tags2, uri + "/" + uih[i].asString(), false) ||
              tags2.type() != Json::arrayValue)
          {
            throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
          }

          Orthanc::DicomMap m;

          for (Json::Value::ArrayIndex j = 0; j < tags2.size(); j++)
          {
            Orthanc::DicomTag tag(0, 0);
            std::string value;
            
            if (tags2[j].type() != Json::stringValue ||
                !Orthanc::DicomTag::ParseHexadecimal(tag, tags2[j].asCString()) ||
                !OrthancPlugins::RestApiGetString(value, uri + "/" + uih[i].asString() + "/" + tags2[j].asString(), false))
            {
              throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
            }
            else
            {
              m.SetValue(tag, value, false);
            }
          }

          instance->AddUIHFrameSequenceItem(m);
        }
      }
      break;
    }

    default:
      break;
  }

  return instance.release();
#endif
}


static bool HasBooleanFlag(const OrthancPluginHttpRequest* request,
                           const std::string& flag)
{
  for (uint32_t i = 0; i < request->getCount; i++)
  {
    if (std::string(request->getKeys[i]) == flag)
    {
      return true;
    }
  }

  return false;
}


void SeriesToNifti(OrthancPluginRestOutput* output,
                   const char* url,
                   const OrthancPluginHttpRequest* request)
{
  static const char* const KEY_INSTANCES = "Instances";

  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();
  
  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    const std::string seriesId(request->groups[0]);

    Json::Value series;
    if (!OrthancPlugins::RestApiGet(series, "/series/" + seriesId, false))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentItem, "Missing series: " + seriesId);
    }

    if (series.type() != Json::objectValue ||
        !series.isMember(KEY_INSTANCES) ||
        series[KEY_INSTANCES].type() != Json::arrayValue)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
    }

    Neuro::DicomInstancesCollection collection;

    for (Json::Value::ArrayIndex i = 0; i < series[KEY_INSTANCES].size(); i++)
    {
      if (series[KEY_INSTANCES][i].type() != Json::stringValue)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
      }
      else
      {
        const std::string id = series[KEY_INSTANCES][i].asString();
        collection.AddInstance(AcquireInstance(id), id);
      }
    }

    const bool compress = HasBooleanFlag(request, "compress");

    std::string nifti;
    CreateNifti(nifti, collection, compress);

    std::string filename = seriesId + ".nii";
    if (compress)
    {
      filename += ".gz";
    }
    
    const std::string contentDisposition = "filename=\"" + filename + "\"";
    OrthancPluginSetHttpHeader(context, output, "Content-Disposition", contentDisposition.c_str());
  
    OrthancPluginAnswerBuffer(context, output, nifti.c_str(), nifti.size(), "application/octet-stream");
  }
}


void InstanceToNifti(OrthancPluginRestOutput* output,
                     const char* url,
                     const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();
  
  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    const std::string instanceId(request->groups[0]);

    Neuro::DicomInstancesCollection collection;
    collection.AddInstance(AcquireInstance(instanceId), instanceId);

    const bool compress = HasBooleanFlag(request, "compress");

    std::string nifti;
    CreateNifti(nifti, collection, compress);

    std::string filename = instanceId + ".nii";
    if (compress)
    {
      filename += ".gz";
    }

    const std::string contentDisposition = "filename=\"" + filename + "\"";
    OrthancPluginSetHttpHeader(context, output, "Content-Disposition", contentDisposition.c_str());
  
    OrthancPluginAnswerBuffer(context, output, nifti.c_str(), nifti.size(), "application/octet-stream");
  }
}


extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    OrthancPlugins::SetGlobalContext(context);
    Orthanc::Logging::InitializePluginContext(context);
    Orthanc::Logging::EnableInfoLevel(true);

    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context) == 0)
    {
      OrthancPlugins::ReportMinimalOrthancVersion(ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
                                                  ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
                                                  ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      return -1;
    }

    OrthancPlugins::SetDescription(ORTHANC_PLUGIN_NAME, "Add support for NIfTI in Orthanc.");

    OrthancPlugins::RegisterRestCallback<SeriesToNifti>("/series/(.*)/nifti", true /* thread safe */);
    OrthancPlugins::RegisterRestCallback<InstanceToNifti>("/instances/(.*)/nifti", true /* thread safe */);

    {
      std::string explorer;
      Orthanc::EmbeddedResources::GetFileResource(
        explorer, Orthanc::EmbeddedResources::ORTHANC_EXPLORER);
      OrthancPlugins::ExtendOrthancExplorer(ORTHANC_PLUGIN_NAME, explorer.c_str());
    }
 
    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return ORTHANC_PLUGIN_NAME;
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_PLUGIN_VERSION;
  }
}

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


namespace Neuro
{
  enum PhaseEncodingDirection
  {
    PhaseEncodingDirection_None,
    PhaseEncodingDirection_Column,
    PhaseEncodingDirection_Row
  };

  enum Modality
  {
    Modality_Unknown,
    Modality_MR,
    Modality_PET,
    Modality_CT
  };

  enum Manufacturer
  {
    Manufacturer_Unknown,
    Manufacturer_Siemens,
    Manufacturer_GE,
    Manufacturer_Hitachi,
    Manufacturer_Mediso,
    Manufacturer_Philips,
    Manufacturer_Toshiba,
    Manufacturer_Canon,
    Manufacturer_UIH,
    Manufacturer_Bruker
  };
}

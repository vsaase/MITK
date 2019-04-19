/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "mitkDICOMQIIOMimeTypes.h"
#include "mitkIOMimeTypes.h"

#include <mitkLogMacros.h>

#include <itkGDCMImageIO.h>
#include <itksys/SystemTools.hxx>

#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>

namespace mitk
{
  std::vector<CustomMimeType *> MitkDICOMQIIOMimeTypes::Get()
  {
    std::vector<CustomMimeType *> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(DICOMSEG_MIMETYPE().Clone());
    //mimeTypes.push_back(DICOMPM_MIMETYPE().Clone());

    return mimeTypes;
  }

  // Mime Types

  //======= Mime Type DICOM SEG =======
  MitkDICOMQIIOMimeTypes::MitkDICOMSEGMimeType::MitkDICOMSEGMimeType() : CustomMimeType(DICOMSEG_MIMETYPE_NAME())
  {
    this->AddExtension("dcm");
    this->SetCategory(IOMimeTypes::CATEGORY_IMAGES());
    this->SetComment("DICOM SEG");
  }

  bool MitkDICOMQIIOMimeTypes::MitkDICOMSEGMimeType::AppliesTo(const std::string &path) const
  {
    std::ifstream myfile;
    myfile.open(path, std::ios::binary);
    //    myfile.seekg (128);
    char *buffer = new char[128];
    myfile.read(buffer, 128);
    myfile.read(buffer, 4);
    if (std::string(buffer).compare("DICM") != 0)
    {
      delete[] buffer;
      return false;
    }
    delete[] buffer;

    bool canRead(CustomMimeType::AppliesTo(path));

    // fix for bug 18572
    // Currently this function is called for writing as well as reading, in that case
    // the image information can of course not be read
    // This is a bug, this function should only be called for reading.
    if (!itksys::SystemTools::FileExists(path.c_str()))
    {
      return canRead;
    }
    // end fix for bug 18572


    DcmFileFormat dcmFileFormat;
    OFCondition status = dcmFileFormat.loadFile(path.c_str());

    if (status.bad())
    {
      canRead = false;
    }

    if (!canRead)
    {
      return canRead;
    }

    OFString modality;
    OFString sopClassUID;
    if (dcmFileFormat.getDataset()->findAndGetOFString(DCM_Modality, modality).good() && dcmFileFormat.getDataset()->findAndGetOFString(DCM_SOPClassUID, sopClassUID).good())
    {
      if (modality.compare("SEG") == 0)
      {//atm we could read SegmentationStorage files. Other storage classes with "SEG" modality, e.g. SurfaceSegmentationStorage (1.2.840.10008.5.1.4.1.1.66.5), are not supported yet.
        if (sopClassUID.compare("1.2.840.10008.5.1.4.1.1.66.4") == 0)
        {
          canRead = true;
        }
        else
        {
          canRead = false;
        }
      }
      else
      {
        canRead = false;
      }
    }

    return canRead;
  }

  MitkDICOMQIIOMimeTypes::MitkDICOMSEGMimeType *MitkDICOMQIIOMimeTypes::MitkDICOMSEGMimeType::Clone() const
  {
    return new MitkDICOMSEGMimeType(*this);
  }

  MitkDICOMQIIOMimeTypes::MitkDICOMSEGMimeType MitkDICOMQIIOMimeTypes::DICOMSEG_MIMETYPE()
  {
    return MitkDICOMSEGMimeType();
  }

  std::string MitkDICOMQIIOMimeTypes::DICOMSEG_MIMETYPE_NAME()
  {
    // create a unique and sensible name for this mime type
    return IOMimeTypes::DEFAULT_BASE_NAME() + ".image.dicom.seg";
  }

  //======= Mime Type DICOM PM =======
  //...
}

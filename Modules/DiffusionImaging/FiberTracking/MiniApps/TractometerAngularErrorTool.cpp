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

#include "MiniAppManager.h"
#include <mitkBaseDataIOFactory.h>
#include <mitkBaseData.h>
#include <mitkImageCast.h>
#include <mitkImageToItk.h>
#include <mitkDiffusionCoreObjectFactory.h>
#include <mitkFiberTrackingObjectFactory.h>
#include <itkEvaluateDirectionImagesFilter.h>
#include <metaCommand.h>
#include "ctkCommandLineParser.h"
#include "ctkCommandLineParser.cpp"
#include <mitkAny.h>
#include <itkImageFileWriter.h>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <itkTractsToVectorImageFilter.h>
#include <mitkIOUtil.h>

#define _USE_MATH_DEFINES
#include <math.h>

int TractometerAngularErrorTool(int argc, char* argv[])
{
    ctkCommandLineParser parser;
    parser.setArgumentPrefix("--", "-");
    parser.addArgument("input", "i", ctkCommandLineParser::String, "input tractogram (.fib, vtk ascii file format)", mitk::Any(), false);
    parser.addArgument("reference", "r", ctkCommandLineParser::StringList, "reference direction images", mitk::Any(), false);
    parser.addArgument("out", "o", ctkCommandLineParser::String, "output root", mitk::Any(), false);
    parser.addArgument("mask", "m", ctkCommandLineParser::String, "mask image");
    parser.addArgument("athresh", "a", ctkCommandLineParser::Float, "angular threshold in degrees. closer fiber directions are regarded as one direction and clustered together.", 25, true);
    parser.addArgument("verbose", "v", ctkCommandLineParser::Bool, "output optional and intermediate calculation results");

    map<string, mitk::Any> parsedArgs = parser.parseArguments(argc, argv);
    if (parsedArgs.size()==0)
        return EXIT_FAILURE;

    ctkCommandLineParser::StringContainerType referenceImages = mitk::any_cast<ctkCommandLineParser::StringContainerType>(parsedArgs["reference"]);

    string fibFile = mitk::any_cast<string>(parsedArgs["input"]);

    string maskImage("");
    if (parsedArgs.count("mask"))
        maskImage = mitk::any_cast<string>(parsedArgs["mask"]);

    float angularThreshold = 25;
    if (parsedArgs.count("athresh"))
        angularThreshold = mitk::any_cast<float>(parsedArgs["athresh"]);

    string outRoot = mitk::any_cast<string>(parsedArgs["out"]);

    bool verbose = false;
    if (parsedArgs.count("verbose"))
        verbose = mitk::any_cast<bool>(parsedArgs["verbose"]);

    try
    {
        RegisterDiffusionCoreObjectFactory();
        RegisterFiberTrackingObjectFactory();

        typedef itk::Image<unsigned char, 3>                                    ItkUcharImgType;
        typedef itk::Image< itk::Vector< float, 3>, 3 >                         ItkDirectionImage3DType;
        typedef itk::VectorContainer< int, ItkDirectionImage3DType::Pointer >   ItkDirectionImageContainerType;
        typedef itk::EvaluateDirectionImagesFilter< float >                     EvaluationFilterType;

        // load fiber bundle
        mitk::FiberBundleX::Pointer inputTractogram = dynamic_cast<mitk::FiberBundleX*>(mitk::IOUtil::LoadDataNode(fibFile)->GetData());

        // load reference directions
        ItkDirectionImageContainerType::Pointer referenceImageContainer = ItkDirectionImageContainerType::New();
        for (int i=0; i<referenceImages.size(); i++)
        {
            try
            {
                mitk::Image::Pointer img = dynamic_cast<mitk::Image*>(mitk::IOUtil::LoadDataNode(referenceImages.at(i))->GetData());
                typedef mitk::ImageToItk< ItkDirectionImage3DType > CasterType;
                CasterType::Pointer caster = CasterType::New();
                caster->SetInput(img);
                caster->Update();
                ItkDirectionImage3DType::Pointer itkImg = caster->GetOutput();
                referenceImageContainer->InsertElement(referenceImageContainer->Size(),itkImg);
            }
            catch(...){ MITK_INFO << "could not load: " << referenceImages.at(i); }
        }

        // load/create mask image
        ItkUcharImgType::Pointer itkMaskImage = ItkUcharImgType::New();
        if (maskImage.compare("")==0)
        {
            ItkDirectionImage3DType::Pointer dirImg = referenceImageContainer->GetElement(0);
            itkMaskImage->SetSpacing( dirImg->GetSpacing() );
            itkMaskImage->SetOrigin( dirImg->GetOrigin() );
            itkMaskImage->SetDirection( dirImg->GetDirection() );
            itkMaskImage->SetLargestPossibleRegion( dirImg->GetLargestPossibleRegion() );
            itkMaskImage->SetBufferedRegion( dirImg->GetLargestPossibleRegion() );
            itkMaskImage->SetRequestedRegion( dirImg->GetLargestPossibleRegion() );
            itkMaskImage->Allocate();
            itkMaskImage->FillBuffer(1);
        }
        else
        {
            mitk::Image::Pointer mitkMaskImage = dynamic_cast<mitk::Image*>(mitk::IOUtil::LoadDataNode(maskImage)->GetData());
            CastToItkImage<ItkUcharImgType>(mitkMaskImage, itkMaskImage);
        }


        // extract directions from fiber bundle
        itk::TractsToVectorImageFilter::Pointer fOdfFilter = itk::TractsToVectorImageFilter::New();
        fOdfFilter->SetFiberBundle(inputTractogram);
        fOdfFilter->SetMaskImage(itkMaskImage);
        fOdfFilter->SetAngularThreshold(cos(angularThreshold*M_PI/180));
        fOdfFilter->SetNormalizeVectors(true);
        fOdfFilter->SetUseWorkingCopy(false);
        fOdfFilter->Update();
        ItkDirectionImageContainerType::Pointer directionImageContainer = fOdfFilter->GetDirectionImageContainer();

        if (verbose)
        {
            // write vector field
            mitk::FiberBundleX::Pointer directions = fOdfFilter->GetOutputFiberBundle();
            mitk::CoreObjectFactory::FileWriterList fileWriters = mitk::CoreObjectFactory::GetInstance()->GetFileWriters();
            for (mitk::CoreObjectFactory::FileWriterList::iterator it = fileWriters.begin() ; it != fileWriters.end() ; ++it)
            {
                if ( (*it)->CanWriteBaseDataType(directions.GetPointer()) ) {
                    QString outfilename(outRoot.c_str());
                    outfilename += "_VECTOR_FIELD.fib";
                    (*it)->SetFileName( outfilename.toStdString().c_str() );
                    (*it)->DoWrite( directions.GetPointer() );
                }
            }

            // write direction images
            for (int i=0; i<directionImageContainer->Size(); i++)
            {
                itk::TractsToVectorImageFilter::ItkDirectionImageType::Pointer itkImg = directionImageContainer->GetElement(i);
                typedef itk::ImageFileWriter< itk::TractsToVectorImageFilter::ItkDirectionImageType > WriterType;
                WriterType::Pointer writer = WriterType::New();
                QString outfilename(outRoot.c_str());
                outfilename += "_DIRECTION_";
                outfilename += QString::number(i);
                outfilename += ".nrrd";
                MITK_INFO << "writing " << outfilename.toStdString();
                writer->SetFileName(outfilename.toStdString().c_str());
                writer->SetInput(itkImg);
                writer->Update();
            }

            // write num direction image
            {
                ItkUcharImgType::Pointer numDirImage = fOdfFilter->GetNumDirectionsImage();
                typedef itk::ImageFileWriter< ItkUcharImgType > WriterType;
                WriterType::Pointer writer = WriterType::New();
                QString outfilename(outRoot.c_str());
                outfilename += "_NUM_DIRECTIONS.nrrd";
                MITK_INFO << "writing " << outfilename.toStdString();
                writer->SetFileName(outfilename.toStdString().c_str());
                writer->SetInput(numDirImage);
                writer->Update();
            }
        }

        // evaluate directions
        EvaluationFilterType::Pointer evaluationFilter = EvaluationFilterType::New();
        evaluationFilter->SetImageSet(directionImageContainer);
        evaluationFilter->SetReferenceImageSet(referenceImageContainer);
        evaluationFilter->SetMaskImage(itkMaskImage);
        evaluationFilter->Update();

        if (verbose)
        {
            EvaluationFilterType::OutputImageType::Pointer angularErrorImage = evaluationFilter->GetOutput(0);
            typedef itk::ImageFileWriter< EvaluationFilterType::OutputImageType > WriterType;
            WriterType::Pointer writer = WriterType::New();
            QString outfilename(outRoot.c_str());
            outfilename += "_ERROR_IMAGE.nrrd";
            writer->SetFileName(outfilename.toStdString().c_str());
            writer->SetInput(angularErrorImage);
            writer->Update();
        }

        QString logFile(outRoot.c_str()); logFile += "_ANGULAR_ERROR.csv";
        QFile file(logFile);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        QString sens = QString("Mean:");
        sens += ",";
        sens += QString::number(evaluationFilter->GetMeanAngularError());
        sens += ";";

        sens += QString("Median:");
        sens += ",";
        sens += QString::number(evaluationFilter->GetMedianAngularError());
        sens += ";";

        sens += QString("Maximum:");
        sens += ",";
        sens += QString::number(evaluationFilter->GetMaxAngularError());
        sens += ";";

        sens += QString("Minimum:");
        sens += ",";
        sens += QString::number(evaluationFilter->GetMinAngularError());
        sens += ";";

        sens += QString("STDEV:");
        sens += ",";
        sens += QString::number(std::sqrt(evaluationFilter->GetVarAngularError()));
        sens += ";";

        out << sens;

        MITK_INFO << "DONE";
    }
    catch (itk::ExceptionObject e)
    {
        MITK_INFO << e;
        return EXIT_FAILURE;
    }
    catch (std::exception e)
    {
        MITK_INFO << e.what();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        MITK_INFO << "ERROR!?!";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
RegisterFiberTrackingMiniApp(TractometerAngularErrorTool);

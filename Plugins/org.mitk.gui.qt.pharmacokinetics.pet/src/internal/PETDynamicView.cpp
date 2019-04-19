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

#include "PETDynamicView.h"

#include "mitkWorkbenchUtil.h"


#include "mitkAterialInputFunctionGenerator.h"

#include "mitkOneTissueCompartmentModelFactory.h"
#include "mitkOneTissueCompartmentModelParameterizer.h"
#include "mitkExtendedOneTissueCompartmentModelFactory.h"
#include "mitkExtendedOneTissueCompartmentModelParameterizer.h"
#include "mitkTwoTissueCompartmentFDGModelFactory.h"
#include "mitkTwoTissueCompartmentFDGModelParameterizer.h"
#include "mitkTwoTissueCompartmentModelFactory.h"
#include "mitkTwoTissueCompartmentModelParameterizer.h"
#include "mitkNumericTwoTissueCompartmentModelFactory.h"
#include "mitkNumericTwoTissueCompartmentModelParameterizer.h"


#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateOr.h>
#include <mitkPixelBasedParameterFitImageGenerator.h>
#include <mitkROIBasedParameterFitImageGenerator.h>
#include <mitkLevenbergMarquardtModelFitFunctor.h>
#include <mitkSumOfSquaredDifferencesFitCostFunction.h>
#include <mitkSquaredDifferencesFitCostFunction.h>
#include <mitkChiSquareFitCostFunction.h>
#include <mitkReducedChiSquareFitCostFunction.h>
#include <mitkSimpleBarrierConstraintChecker.h>
#include <mitkModelFitResultHelper.h>
#include <mitkImageTimeSelector.h>
#include <mitkMaskedDynamicImageStatisticsGenerator.h>
#include <mitkExtractTimeGrid.h>
#include <mitkInitialParameterizationDelegateBase.h>

#include <QMessageBox>
#include <QThreadPool>
#include <QmitkDataStorageComboBox.h>
#include <QFileDialog>
#include <boost/tokenizer.hpp>


// Includes for image casting between ITK and MITK
#include <mitkImage.h>
#include "mitkImageCast.h"
#include "mitkITKImageImport.h"
#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <iostream>


const std::string PETDynamicView::VIEW_ID = "org.mitk.gui.qt.pharmacokinetics.pet";

inline double convertToDouble(const std::string& data)
{
  std::istringstream stepStream(data);
  double value = 0.0;
  stepStream >> value;
  return value;
}

void PETDynamicView::SetFocus()
{
  m_Controls.btnModelling->setFocus();
}

void PETDynamicView::CreateQtPartControl(QWidget* parent)
{
  m_Controls.setupUi(parent);

  m_Controls.btnModelling->setEnabled(false);
  m_Controls.errorMessageLabel->hide();

  this->InitModelComboBox();

  connect(m_Controls.btnModelling, SIGNAL(clicked()), this, SLOT(OnModellingButtonClicked()));

  connect(m_Controls.comboModel, SIGNAL(currentIndexChanged(int)), this, SLOT(OnModellSet(int)));
  connect(m_Controls.radioPixelBased, SIGNAL(toggled(bool)), this, SLOT(UpdateGUIControls()));

  //AIF setting
  m_Controls.groupAIF->hide();
  m_Controls.btnAIFFile->setEnabled(false);
  m_Controls.btnAIFFile->setEnabled(false);
  m_Controls.radioAIFImage->setChecked(true);
  m_Controls.comboAIFMask->SetDataStorage(this->GetDataStorage());
  m_Controls.comboAIFMask->SetPredicate(m_IsMaskPredicate);
  m_Controls.comboAIFMask->setVisible(true);
  m_Controls.comboAIFMask->setEnabled(true);
  m_Controls.comboAIFImage->SetDataStorage(this->GetDataStorage());
  m_Controls.comboAIFImage->SetPredicate(m_IsNoMaskImagePredicate);
  m_Controls.comboAIFImage->setEnabled(false);
  m_Controls.checkDedicatedAIFImage->setEnabled(true);
  m_Controls.HCLSpinBox->setValue(0.0);

  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.comboAIFMask,
          SLOT(setVisible(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.labelAIFMask,
          SLOT(setVisible(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.checkDedicatedAIFImage,
          SLOT(setVisible(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.comboAIFMask,
          SLOT(setEnabled(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.checkDedicatedAIFImage,
          SLOT(setEnabled(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.checkDedicatedAIFImage,
          SLOT(setVisible(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), m_Controls.comboAIFImage,
          SLOT(setVisible(bool)));
  connect(m_Controls.checkDedicatedAIFImage, SIGNAL(toggled(bool)), m_Controls.comboAIFImage,
          SLOT(setEnabled(bool)));
  connect(m_Controls.radioAIFImage, SIGNAL(toggled(bool)), this, SLOT(UpdateGUIControls()));
  connect(m_Controls.radioAIFFile, SIGNAL(toggled(bool)), m_Controls.btnAIFFile,
          SLOT(setEnabled(bool)));
  connect(m_Controls.radioAIFFile, SIGNAL(toggled(bool)), m_Controls.aifFilePath,
          SLOT(setEnabled(bool)));
  connect(m_Controls.radioAIFFile, SIGNAL(toggled(bool)), this, SLOT(UpdateGUIControls()));

  connect(m_Controls.btnAIFFile, SIGNAL(clicked()), this, SLOT(LoadAIFfromFile()));

  //Model fit configuration
  m_Controls.groupBox_FitConfiguration->hide();

  m_Controls.checkBox_Constraints->setEnabled(false);
  m_Controls.constraintManager->setEnabled(false);
  m_Controls.initialValuesManager->setEnabled(false);
  m_Controls.initialValuesManager->setDataStorage(this->GetDataStorage());

  connect(m_Controls.radioButton_StartParameters, SIGNAL(toggled(bool)), this,
          SLOT(UpdateGUIControls()));
  connect(m_Controls.checkBox_Constraints, SIGNAL(toggled(bool)), this,
          SLOT(UpdateGUIControls()));
  connect(m_Controls.initialValuesManager, SIGNAL(initialValuesChanged(void)), this, SLOT(UpdateGUIControls()));


  connect(m_Controls.radioButton_StartParameters, SIGNAL(toggled(bool)),
          m_Controls.initialValuesManager,
          SLOT(setEnabled(bool)));
  connect(m_Controls.checkBox_Constraints, SIGNAL(toggled(bool)), m_Controls.constraintManager,
          SLOT(setEnabled(bool)));
  connect(m_Controls.checkBox_Constraints, SIGNAL(toggled(bool)), m_Controls.constraintManager,
          SLOT(setVisible(bool)));


  UpdateGUIControls();

}

void PETDynamicView::UpdateGUIControls()
{
  m_Controls.lineFitName->setPlaceholderText(QString::fromStdString(this->GetDefaultFitName()));
  m_Controls.lineFitName->setEnabled(!m_FittingInProgress);

  m_Controls.checkBox_Constraints->setEnabled(m_modelConstraints.IsNotNull());

   bool is1TCMFactory = dynamic_cast<mitk::OneTissueCompartmentModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;
   bool isExt1TCMFactory = dynamic_cast<mitk::ExtendedOneTissueCompartmentModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;
   bool isFDGCMFactory = dynamic_cast<mitk::TwoTissueCompartmentFDGModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;

  bool is2TCMFactory = dynamic_cast<mitk::TwoTissueCompartmentModelFactory*>
                       (m_selectedModelFactory.GetPointer()) != NULL ||
                       dynamic_cast<mitk::NumericTwoTissueCompartmentModelFactory*>
                       (m_selectedModelFactory.GetPointer()) != NULL;


  m_Controls.groupAIF->setVisible(is1TCMFactory || isExt1TCMFactory || isFDGCMFactory || is2TCMFactory);

  m_Controls.groupBox_FitConfiguration->setVisible(m_selectedModelFactory);

  m_Controls.groupBox->setEnabled(!m_FittingInProgress);
  m_Controls.comboModel->setEnabled(!m_FittingInProgress);
  m_Controls.groupAIF->setEnabled(!m_FittingInProgress);
  m_Controls.groupBox_FitConfiguration->setEnabled(!m_FittingInProgress);

  m_Controls.radioROIbased->setEnabled(m_selectedMask.IsNotNull());

  m_Controls.btnModelling->setEnabled(m_selectedImage.IsNotNull()
                                      && m_selectedModelFactory.IsNotNull() && !m_FittingInProgress && CheckModelSettings());
}

//void PETDynamicView::OnModelSettingChanged()
//{
//  bool ok = m_selectedImage.IsNotNull() && m_selectedModelFactory.IsNotNull() && !m_FittingInProgress && CheckModelSettings();

//  m_Controls.btnModelling->setEnabled(ok);
//}


void PETDynamicView::OnModellSet(int index)
{
  m_selectedModelFactory = NULL;

  if (index > 0)
  {
    if (static_cast<ModelFactoryStackType::size_type>(index) <= m_FactoryStack.size() )
    {
        m_selectedModelFactory = m_FactoryStack[index - 1];
    }
    else
    {
        MITK_WARN << "Invalid model index. Index outside of the factory stack. Factory stack size: "<< m_FactoryStack.size() << "; invalid index: "<< index;
    }
  }

  if (m_selectedModelFactory)
  {
    this->m_modelConstraints = dynamic_cast<mitk::SimpleBarrierConstraintChecker*>
                               (m_selectedModelFactory->CreateDefaultConstraints().GetPointer());

    m_Controls.initialValuesManager->setInitialValues(m_selectedModelFactory->GetParameterNames(),
        m_selectedModelFactory->GetDefaultInitialParameterization());

    if (this->m_modelConstraints.IsNull())
    {
      this->m_modelConstraints = mitk::SimpleBarrierConstraintChecker::New();
    }

    m_Controls.constraintManager->setChecker(this->m_modelConstraints,
        this->m_selectedModelFactory->GetParameterNames());
  }

  m_Controls.checkBox_Constraints->setEnabled(m_modelConstraints.IsNotNull());


  UpdateGUIControls();
}

std::string PETDynamicView::GetFitName() const
{
  std::string fitName = m_Controls.lineFitName->text().toStdString();
  if (fitName.empty())
  {
    fitName = m_Controls.lineFitName->placeholderText().toStdString();
  }
  return fitName;
}

std::string PETDynamicView::GetDefaultFitName() const
{
    std::string defaultName = "undefined model";

    if (this->m_selectedModelFactory.IsNotNull())
    {
        defaultName = this->m_selectedModelFactory->GetClassID();
    }

    if (this->m_Controls.radioPixelBased->isChecked())
    {
        defaultName += "_pixel";
    }
    else
    {
        defaultName += "_roi";
    }

    return defaultName;
}


void PETDynamicView::OnModellingButtonClicked()
{
  //check if all static parameters set
  if (m_selectedModelFactory.IsNotNull() && CheckModelSettings())
  {
    mitk::ParameterFitImageGeneratorBase::Pointer generator = NULL;
    mitk::modelFit::ModelFitInfo::Pointer fitSession = NULL;


    bool isOTCFactory = dynamic_cast<mitk::OneTissueCompartmentModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;
    bool isextOTCFactory = dynamic_cast<mitk::ExtendedOneTissueCompartmentModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;

    bool isFDGFactory = dynamic_cast<mitk::TwoTissueCompartmentFDGModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;

    bool isTTCFactory = dynamic_cast<mitk::TwoTissueCompartmentModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;
    bool isNumTTCFactory = dynamic_cast<mitk::NumericTwoTissueCompartmentModelFactory*>
                           (m_selectedModelFactory.GetPointer()) != NULL;


    if (isOTCFactory)
    {
      if (this->m_Controls.radioPixelBased->isChecked())
      {
        GenerateModelFit_PixelBased<mitk::OneTissueCompartmentModelParameterizer>(fitSession, generator);
      }
      else
      {
        GenerateModelFit_ROIBased<mitk::OneTissueCompartmentModelParameterizer>(fitSession, generator);
      }
    }

    else if (isextOTCFactory)
    {
      if (this->m_Controls.radioPixelBased->isChecked())
      {
        GenerateModelFit_PixelBased<mitk::ExtendedOneTissueCompartmentModelParameterizer>(fitSession, generator);
      }
      else
      {
        GenerateModelFit_ROIBased<mitk::ExtendedOneTissueCompartmentModelParameterizer>(fitSession, generator);
      }
    }
    else if (isFDGFactory)
    {
      if (this->m_Controls.radioPixelBased->isChecked())
      {
        GenerateModelFit_PixelBased<mitk::TwoTissueCompartmentFDGModelParameterizer>(fitSession, generator);
      }
      else
      {
        GenerateModelFit_ROIBased<mitk::TwoTissueCompartmentFDGModelParameterizer>(fitSession, generator);
      }
    }


    else if (isTTCFactory)
    {
      if (this->m_Controls.radioPixelBased->isChecked())
      {
        GenerateModelFit_PixelBased<mitk::TwoTissueCompartmentModelParameterizer>(fitSession, generator);
      }
      else
      {
        GenerateModelFit_ROIBased<mitk::TwoTissueCompartmentModelParameterizer>(fitSession, generator);
      }
    }

    else if (isNumTTCFactory)
    {
      if (this->m_Controls.radioPixelBased->isChecked())
      {
        GenerateModelFit_PixelBased<mitk::NumericTwoTissueCompartmentModelParameterizer>(fitSession,
            generator);
      }
      else
      {
        GenerateModelFit_ROIBased<mitk::NumericTwoTissueCompartmentModelParameterizer>(fitSession,
            generator);
      }
    }

    //add other models with else if

    if (generator.IsNotNull() && fitSession.IsNotNull())
    {
      m_FittingInProgress = true;
      DoFit(fitSession, generator);
    }
    else
    {
      QMessageBox box;
      box.setText("Fitting error!");
      box.setInformativeText("Could not establish fitting job. Error when setting ab generator, model parameterizer or session info.");
      box.setStandardButtons(QMessageBox::Ok);
      box.setDefaultButton(QMessageBox::Ok);
      box.setIcon(QMessageBox::Warning);
      box.exec();
    }

  }
  else
  {
    QMessageBox box;
    box.setText("Static parameters for model are not set!");
    box.setInformativeText("Some static parameters, that are needed for calculation are not set and equal to zero. Modeling not possible");
    box.setStandardButtons(QMessageBox::Ok);
    box.setDefaultButton(QMessageBox::Ok);
    box.setIcon(QMessageBox::Warning);
    box.exec();
  }
}


void PETDynamicView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                        const QList<mitk::DataNode::Pointer>& selectedNodes)
{
    m_selectedMaskNode = NULL;
    m_selectedMask = NULL;

    m_Controls.errorMessageLabel->setText("");
    m_Controls.masklabel->setText("No (valid) mask selected.");
    m_Controls.timeserieslabel->setText("No (valid) series selected.");

    QList<mitk::DataNode::Pointer> nodes = selectedNodes;

    if (nodes.size() > 0 && this->m_IsNoMaskImagePredicate->CheckNode(nodes.front()))
    {
      this->m_selectedNode = nodes.front();
      auto selectedImage = dynamic_cast<mitk::Image*>(this->m_selectedNode->GetData());
      m_Controls.timeserieslabel->setText((this->m_selectedNode->GetName()).c_str());

      if (selectedImage != this->m_selectedImage)
      {
        if (selectedImage)
        {
          this->m_Controls.initialValuesManager->setReferenceImageGeometry(selectedImage->GetGeometry());
        }
        else
        {
          this->m_Controls.initialValuesManager->setReferenceImageGeometry(nullptr);
        }
      }
      this->m_selectedImage = selectedImage;
      nodes.pop_front();
    }
    else
    {
      this->m_selectedNode = NULL;
      this->m_selectedImage = NULL;
      this->m_Controls.initialValuesManager->setReferenceImageGeometry(nullptr);
    }

    if (nodes.size() > 0 && this->m_IsMaskPredicate->CheckNode(nodes.front()))
    {
        this->m_selectedMaskNode = nodes.front();
        this->m_selectedMask = dynamic_cast<mitk::Image*>(this->m_selectedMaskNode->GetData());

        if (this->m_selectedMask->GetTimeSteps() > 1)
        {
          MITK_INFO <<
                    "Selected mask has multiple timesteps. Only use first timestep to mask model fit. Mask name: " <<
                    m_selectedMaskNode->GetName();
          mitk::ImageTimeSelector::Pointer maskedImageTimeSelector = mitk::ImageTimeSelector::New();
          maskedImageTimeSelector->SetInput(this->m_selectedMask);
          maskedImageTimeSelector->SetTimeNr(0);
          maskedImageTimeSelector->UpdateLargestPossibleRegion();
          this->m_selectedMask = maskedImageTimeSelector->GetOutput();
        }

        m_Controls.masklabel->setText((this->m_selectedMaskNode->GetName()).c_str());
    }

    if (m_selectedMask.IsNull())
    {
      this->m_Controls.radioPixelBased->setChecked(true);
    }

    m_Controls.errorMessageLabel->show();

    UpdateGUIControls();
}

bool PETDynamicView::CheckModelSettings() const
{
  bool ok = true;

  //check wether any model is set at all. Otherwise exit with false
  if (m_selectedModelFactory.IsNotNull())
  {

    bool isOTCFactory = dynamic_cast<mitk::OneTissueCompartmentModelFactory*>
                          (m_selectedModelFactory.GetPointer()) != NULL;
    bool isextOTCFactory = dynamic_cast<mitk::ExtendedOneTissueCompartmentModelFactory*>
                          (m_selectedModelFactory.GetPointer()) != NULL;
    bool isFDGFactory = dynamic_cast<mitk::TwoTissueCompartmentFDGModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;
    bool isTTCFactory = dynamic_cast<mitk::TwoTissueCompartmentModelFactory*>
                        (m_selectedModelFactory.GetPointer()) != NULL;
    bool isNumTTCFactory = dynamic_cast<mitk::NumericTwoTissueCompartmentModelFactory*>
                           (m_selectedModelFactory.GetPointer()) != NULL;

    if (isOTCFactory || isextOTCFactory || isFDGFactory || isTTCFactory || isNumTTCFactory)
    {
        if (this->m_Controls.radioAIFImage->isChecked())
        {
          ok = ok && m_Controls.comboAIFMask->GetSelectedNode().IsNotNull();

          if (this->m_Controls.checkDedicatedAIFImage->isChecked())
          {
            ok = ok && m_Controls.comboAIFImage->GetSelectedNode().IsNotNull();
          }
        }
        else if (this->m_Controls.radioAIFFile->isChecked())
        {
          ok = ok && (this->AIFinputGrid.size() != 0) && (this->AIFinputFunction.size() != 0);
        }
        else
        {
          ok = false;
        }

     }
    //add other models as else if and check wether all needed static parameters are set
    else
    {
      ok = false;
    }

    if (this->m_Controls.radioButton_StartParameters->isChecked() && !this->m_Controls.initialValuesManager->hasValidInitialValues())
    {
      std::string warning = "Warning. Invalid start parameters. At least one parameter as an invalid image setting as source.";
      MITK_ERROR << warning;
      m_Controls.infoBox->append(QString("<font color='red'><b>") + QString::fromStdString(warning) + QString("</b></font>"));

      ok = false;
    };
  }
  else
  {
    ok = false;
  }

  return ok;
}

void PETDynamicView::ConfigureInitialParametersOfParameterizer(mitk::ModelParameterizerBase*
    parameterizer) const
{
  if (m_Controls.radioButton_StartParameters->isChecked())
  {
    //use user defined initial parameters
    mitk::InitialParameterizationDelegateBase::Pointer paramDelegate = m_Controls.initialValuesManager->getInitialParametrizationDelegate();
    parameterizer->SetInitialParameterizationDelegate(paramDelegate);
  }
}

template <typename TParameterizer>
void PETDynamicView::GenerateModelFit_PixelBased(
  mitk::modelFit::ModelFitInfo::Pointer& modelFitInfo,
  mitk::ParameterFitImageGeneratorBase::Pointer& generator)
{
  mitk::PixelBasedParameterFitImageGenerator::Pointer fitGenerator =
    mitk::PixelBasedParameterFitImageGenerator::New();

  typename TParameterizer::Pointer modelParameterizer =
    TParameterizer::New();


  mitk::AIFBasedModelBase::AterialInputFunctionType aif;
  mitk::AIFBasedModelBase::TimeGridType aifTimeGrid;

  GetAIF(aif, aifTimeGrid);

  //Model configuration (static parameters) can be done now
  modelParameterizer->SetAIF(aif);
  modelParameterizer->SetAIFTimeGrid(aifTimeGrid);
  this->ConfigureInitialParametersOfParameterizer(modelParameterizer);

  //Specify fitting strategy and criterion parameters
  mitk::ModelFitFunctorBase::Pointer fitFunctor = CreateDefaultFitFunctor(modelParameterizer);

  //Parametrize fit generator
  fitGenerator->SetModelParameterizer(modelParameterizer);
  std::string roiUID = "";

  if (m_selectedMask.IsNotNull())
  {
    fitGenerator->SetMask(m_selectedMask);
    roiUID = mitk::EnsureModelFitUID(this->m_selectedMaskNode);
  }

  fitGenerator->SetDynamicImage(this->m_selectedImage);
  fitGenerator->SetFitFunctor(fitFunctor);

  generator = fitGenerator.GetPointer();

  //Create model info
  modelFitInfo = mitk::modelFit::CreateFitInfoFromModelParameterizer(modelParameterizer,
    m_selectedNode->GetData(), mitk::ModelFitConstants::FIT_TYPE_VALUE_PIXELBASED(), this->GetFitName(), roiUID);

  mitk::ScalarListLookupTable::ValueType infoSignal;

  for (mitk::AIFBasedModelBase::AterialInputFunctionType::const_iterator pos = aif.begin();
       pos != aif.end(); ++pos)
  {
    infoSignal.push_back(*pos);
  }

  modelFitInfo->inputData.SetTableValue("AIF", infoSignal);
}


template <typename TParameterizer>
void PETDynamicView::GenerateModelFit_ROIBased(
  mitk::modelFit::ModelFitInfo::Pointer& modelFitInfo,
  mitk::ParameterFitImageGeneratorBase::Pointer& generator)
{
  mitk::ROIBasedParameterFitImageGenerator::Pointer fitGenerator =
    mitk::ROIBasedParameterFitImageGenerator::New();

  typename TParameterizer::Pointer modelParameterizer =
    TParameterizer::New();


  //Compute AIF
  mitk::AterialInputFunctionGenerator::Pointer aifGenerator =
    mitk::AterialInputFunctionGenerator::New();
  aifGenerator->SetDynamicImage(this->m_selectedImage);
  aifGenerator->SetMask(this->m_selectedAIFMask);

  mitk::AIFBasedModelBase::AterialInputFunctionType aif = aifGenerator->GetAterialInputFunction();
  mitk::AIFBasedModelBase::TimeGridType aifTimeGrid = aifGenerator->GetAterialInputFunctionTimeGrid();

  //Model configuration (static parameters) can be done now
  modelParameterizer->SetAIF(aif);
  modelParameterizer->SetAIFTimeGrid(aifTimeGrid);

  this->ConfigureInitialParametersOfParameterizer(modelParameterizer);

  //Compute ROI signal
  mitk::MaskedDynamicImageStatisticsGenerator::Pointer signalGenerator =
    mitk::MaskedDynamicImageStatisticsGenerator::New();
  signalGenerator->SetMask(m_selectedMask);
  signalGenerator->SetDynamicImage(m_selectedImage);
  signalGenerator->Generate();

  mitk::MaskedDynamicImageStatisticsGenerator::ResultType roiSignal = signalGenerator->GetMean();

  //Specify fitting strategy and criterion parameters
  mitk::ModelFitFunctorBase::Pointer fitFunctor = CreateDefaultFitFunctor(modelParameterizer);

  //Parametrize fit generator
  fitGenerator->SetModelParameterizer(modelParameterizer);
  fitGenerator->SetMask(m_selectedMask);
  fitGenerator->SetFitFunctor(fitFunctor);
  fitGenerator->SetSignal(roiSignal);
  fitGenerator->SetTimeGrid(mitk::ExtractTimeGrid(m_selectedImage));

  generator = fitGenerator.GetPointer();

  std::string roiUID = mitk::EnsureModelFitUID(this->m_selectedMaskNode);

  //Create model info
  modelFitInfo = mitk::modelFit::CreateFitInfoFromModelParameterizer(modelParameterizer,
    m_selectedNode->GetData(), mitk::ModelFitConstants::FIT_TYPE_VALUE_ROIBASED(), this->GetFitName(), roiUID);

  mitk::ScalarListLookupTable::ValueType infoSignal;

  for (mitk::MaskedDynamicImageStatisticsGenerator::ResultType::const_iterator pos =
         roiSignal.begin(); pos != roiSignal.end(); ++pos)
  {
    infoSignal.push_back(*pos);
  }

  modelFitInfo->inputData.SetTableValue("ROI", infoSignal);

  infoSignal.clear();

  for (mitk::AIFBasedModelBase::AterialInputFunctionType::const_iterator pos = aif.begin();
       pos != aif.end(); ++pos)
  {
    infoSignal.push_back(*pos);
  }

  modelFitInfo->inputData.SetTableValue("AIF", infoSignal);
}

void PETDynamicView::DoFit(const mitk::modelFit::ModelFitInfo* fitSession,
                           mitk::ParameterFitImageGeneratorBase* generator)
{
  std::stringstream message;
  message << "<font color='green'>" << "Fitting Data Set . . ." << "</font>";
  m_Controls.errorMessageLabel->setText(message.str().c_str());
  m_Controls.errorMessageLabel->show();

  /////////////////////////
  //create job and put it into the thread pool
  ParameterFitBackgroundJob* pJob = new ParameterFitBackgroundJob(generator, fitSession,
      this->m_selectedNode);

  pJob->setAutoDelete(true);

  connect(pJob, SIGNAL(Error(QString)), this, SLOT(OnJobError(QString)));
  connect(pJob, SIGNAL(Finished()), this, SLOT(OnJobFinished()));
  connect(pJob, SIGNAL(ResultsAreAvailable(mitk::modelFit::ModelFitResultNodeVectorType,
                       const ParameterFitBackgroundJob*)), this,
          SLOT(OnJobResultsAreAvailable(mitk::modelFit::ModelFitResultNodeVectorType,
                                        const ParameterFitBackgroundJob*)), Qt::BlockingQueuedConnection);

  connect(pJob, SIGNAL(JobProgress(double)), this, SLOT(OnJobProgress(double)));
  connect(pJob, SIGNAL(JobStatusChanged(QString)), this, SLOT(OnJobStatusChanged(QString)));

  QThreadPool* threadPool = QThreadPool::globalInstance();
  threadPool->start(pJob);
}

PETDynamicView::PETDynamicView() : m_FittingInProgress(false)
{
  m_selectedImage = NULL;
  m_selectedMask = NULL;

  mitk::ModelFactoryBase::Pointer factory =
    mitk::OneTissueCompartmentModelFactory::New().GetPointer();
    m_FactoryStack.push_back(factory);
    factory = mitk::ExtendedOneTissueCompartmentModelFactory::New().GetPointer();
    m_FactoryStack.push_back(factory);
    factory = mitk::TwoTissueCompartmentModelFactory::New().GetPointer();
    m_FactoryStack.push_back(factory);
    factory = mitk::TwoTissueCompartmentFDGModelFactory::New().GetPointer();
    m_FactoryStack.push_back(factory);
    factory = mitk::NumericTwoTissueCompartmentModelFactory::New().GetPointer();
    m_FactoryStack.push_back(factory);

  mitk::NodePredicateDataType::Pointer isLabelSet = mitk::NodePredicateDataType::New("LabelSetImage");
  mitk::NodePredicateDataType::Pointer isImage = mitk::NodePredicateDataType::New("Image");
  mitk::NodePredicateProperty::Pointer isBinary = mitk::NodePredicateProperty::New("binary", mitk::BoolProperty::New(true));
  mitk::NodePredicateAnd::Pointer isLegacyMask = mitk::NodePredicateAnd::New(isImage, isBinary);

  mitk::NodePredicateOr::Pointer isMask = mitk::NodePredicateOr::New(isLegacyMask, isLabelSet);
  mitk::NodePredicateAnd::Pointer isNoMask = mitk::NodePredicateAnd::New(isImage, mitk::NodePredicateNot::New(isMask));

  this->m_IsMaskPredicate = mitk::NodePredicateAnd::New(isMask, mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))).GetPointer();

  this->m_IsNoMaskImagePredicate = mitk::NodePredicateAnd::New(isNoMask, mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object"))).GetPointer();

}

void PETDynamicView::OnJobFinished()
{
  this->m_Controls.infoBox->append(QString("Fitting finished"));
  this->m_FittingInProgress = false;
};

void PETDynamicView::OnJobError(QString err)
{
  MITK_ERROR << err.toStdString().c_str();

  m_Controls.infoBox->append(QString("<font color='red'><b>") + err + QString("</b></font>"));

};

void PETDynamicView::OnJobResultsAreAvailable(mitk::modelFit::ModelFitResultNodeVectorType results,
    const ParameterFitBackgroundJob* pJob)
{
  //Store the resulting parameter fit image via convenience helper function in data storage
  //(handles the correct generation of the nodes and their properties)
  mitk::modelFit::StoreResultsInDataStorage(this->GetDataStorage(), results, pJob->GetParentNode());

  m_Controls.errorMessageLabel->setText("");
  m_Controls.errorMessageLabel->hide();
};

void PETDynamicView::OnJobProgress(double progress)
{
  QString report = QString("Progress. ") + QString::number(progress);
  this->m_Controls.infoBox->append(report);
};

void PETDynamicView::OnJobStatusChanged(QString info)
{
  this->m_Controls.infoBox->append(info);
}


void PETDynamicView::InitModelComboBox() const
{
  this->m_Controls.comboModel->clear();
  this->m_Controls.comboModel->addItem(tr("No model selected"));

  for (ModelFactoryStackType::const_iterator pos = m_FactoryStack.begin();
       pos != m_FactoryStack.end(); ++pos)
  {
    this->m_Controls.comboModel->addItem(QString::fromStdString((*pos)->GetClassID()));
  }

  this->m_Controls.comboModel->setCurrentIndex(0);
};

mitk::ModelFitFunctorBase::Pointer PETDynamicView::CreateDefaultFitFunctor(
  const mitk::ModelParameterizerBase* parameterizer) const
{
  mitk::LevenbergMarquardtModelFitFunctor::Pointer fitFunctor =
    mitk::LevenbergMarquardtModelFitFunctor::New();
  mitk::SumOfSquaredDifferencesFitCostFunction::Pointer evaluation =
    mitk::SumOfSquaredDifferencesFitCostFunction::New();

  fitFunctor->RegisterEvaluationParameter("sum_diff^2", evaluation);

  mitk::ChiSquareFitCostFunction::Pointer chi2 =
          mitk::ChiSquareFitCostFunction::New();
  fitFunctor->RegisterEvaluationParameter("Chi^2", chi2);

  mitk::ReducedChiSquareFitCostFunction::Pointer redchi2 =
          mitk::ReducedChiSquareFitCostFunction::New();
  fitFunctor->RegisterEvaluationParameter("redChi^2", redchi2);



  if (m_Controls.checkBox_Constraints->isChecked())
  {
    fitFunctor->SetConstraintChecker(m_modelConstraints);
  }

  mitk::ModelBase::Pointer refModel = parameterizer->GenerateParameterizedModel();

  ::itk::LevenbergMarquardtOptimizer::ScalesType scales;
  scales.SetSize(refModel->GetNumberOfParameters());
  scales.Fill(1.0);
  fitFunctor->SetScales(scales);

  return fitFunctor.GetPointer();
}

void PETDynamicView::GetAIF(mitk::AIFBasedModelBase::AterialInputFunctionType& aif,
                             mitk::AIFBasedModelBase::AterialInputFunctionType& aifTimeGrid)
{
  if (this->m_Controls.radioAIFFile->isChecked())
  {
    aif.clear();
    aifTimeGrid.clear();

    aif.SetSize(AIFinputFunction.size());
    aifTimeGrid.SetSize(AIFinputGrid.size());

    aif.fill(0.0);
    aifTimeGrid.fill(0.0);

    itk::Array<double>::iterator aifPos = aif.begin();

    for (std::vector<double>::const_iterator pos = AIFinputFunction.begin();
         pos != AIFinputFunction.end(); ++pos, ++aifPos)
    {
      *aifPos = *pos;
    }

    itk::Array<double>::iterator gridPos = aifTimeGrid.begin();

    for (std::vector<double>::const_iterator pos = AIFinputGrid.begin(); pos != AIFinputGrid.end();
         ++pos, ++gridPos)
    {
      *gridPos = *pos;
    }
  }
  else if (this->m_Controls.radioAIFImage->isChecked())
  {
    aif.clear();
    aifTimeGrid.clear();

    mitk::AterialInputFunctionGenerator::Pointer aifGenerator =
      mitk::AterialInputFunctionGenerator::New();

    //Hematocrit level
    aifGenerator->SetHCL(this->m_Controls.HCLSpinBox->value());

    //mask settings
    this->m_selectedAIFMaskNode = m_Controls.comboAIFMask->GetSelectedNode();
    this->m_selectedAIFMask = dynamic_cast<mitk::Image*>(this->m_selectedAIFMaskNode->GetData());

    if (this->m_selectedAIFMask->GetTimeSteps() > 1)
    {
      MITK_INFO <<
                "Selected AIF mask has multiple timesteps. Only use first timestep to mask model fit. AIF Mask name: "
                <<
                m_selectedAIFMaskNode->GetName() ;
      mitk::ImageTimeSelector::Pointer maskedImageTimeSelector = mitk::ImageTimeSelector::New();
      maskedImageTimeSelector->SetInput(this->m_selectedAIFMask);
      maskedImageTimeSelector->SetTimeNr(0);
      maskedImageTimeSelector->UpdateLargestPossibleRegion();
      this->m_selectedAIFMask = maskedImageTimeSelector->GetOutput();
    }

    if (this->m_selectedAIFMask.IsNotNull())
    {
      aifGenerator->SetMask(this->m_selectedAIFMask);
    }

    //image settings
    if (this->m_Controls.checkDedicatedAIFImage->isChecked())
    {
      this->m_selectedAIFImageNode = m_Controls.comboAIFImage->GetSelectedNode();
      this->m_selectedAIFImage = dynamic_cast<mitk::Image*>(this->m_selectedAIFImageNode->GetData());
    }
    else
    {
      this->m_selectedAIFImageNode = m_selectedNode;
      this->m_selectedAIFImage = m_selectedImage;
    }


    aifGenerator->SetDynamicImage(this->m_selectedAIFImage);

    aif = aifGenerator->GetAterialInputFunction();
    aifTimeGrid = aifGenerator->GetAterialInputFunctionTimeGrid();
  }
  else
  {
    mitkThrow() << "Cannot generate AIF. View is in a invalide state. No AIF mode selected.";
  }

}


void PETDynamicView::LoadAIFfromFile()
{
  QFileDialog dialog;
  dialog.setNameFilter(tr("Images (*.csv"));

  QString fileName = dialog.getOpenFileName();

  m_Controls.aifFilePath->setText(fileName);

  std::string m_aifFilePath = fileName.toStdString();
  //Read Input
  typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;
  /////////////////////////////////////////////////////////////////////////////////////////////////
  //AIF Data

  std::ifstream in1(m_aifFilePath.c_str());

  if (!in1.is_open())
  {
    m_Controls.errorMessageLabel->setText("Could not open AIF File!");
  }


  std::vector< std::string > vec1;
  std::string line1;

  while (getline(in1, line1))
  {
    Tokenizer tok(line1);
    vec1.assign(tok.begin(), tok.end());

    //        if (vec1.size() < 3) continue;

    this->AIFinputGrid.push_back(convertToDouble(vec1[0]));
    this->AIFinputFunction.push_back(convertToDouble(vec1[1]));

  }

}

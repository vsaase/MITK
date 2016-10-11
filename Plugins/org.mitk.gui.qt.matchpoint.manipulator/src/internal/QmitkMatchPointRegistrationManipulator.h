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


#ifndef __Q_MITK_MATCHPOINT_REGISTRATION_MANIPULATOR_H
#define __Q_MITK_MATCHPOINT_REGISTRATION_MANIPULATOR_H

#include <QmitkAbstractView.h>
#include <mitkIRenderWindowPartListener.h>
#include <QmitkSliceNavigationListener.h>

#include "ui_QmitkMatchPointRegistrationManipulator.h"

/*!
\brief QmitkMatchPointRegistrationManipulator

\warning  This class is not yet documented. Use "git blame" and ask the author to provide basic documentation.

\sa QmitkFunctionality
\ingroup ${plugin_target}_internal
*/
class QmitkMatchPointRegistrationManipulator : public QmitkAbstractView, public mitk::IRenderWindowPartListener
{  
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

public:  

  static const std::string VIEW_ID;

  /**
  * Creates smartpointer typedefs
  */
  berryObjectMacro(QmitkMatchPointRegistrationManipulator)

  QmitkMatchPointRegistrationManipulator();
  ~QmitkMatchPointRegistrationManipulator();

  virtual void CreateQtPartControl(QWidget *parent);

  protected slots:

    /// \brief Called when the user clicks the GUI button

  void OnEvalBtnPushed();
  void OnStopBtnPushed();
  void OnSettingsChanged(mitk::DataNode*);

    void OnSliceChanged();

protected:
  /// \brief called by QmitkFunctionality when DataManager's selection has changed
  virtual void OnSelectionChanged( berry::IWorkbenchPart::Pointer source,
    const QList<mitk::DataNode::Pointer>& nodes);

  virtual void SetFocus();

  virtual void RenderWindowPartActivated(mitk::IRenderWindowPart* renderWindowPart);
  virtual void RenderWindowPartDeactivated(mitk::IRenderWindowPart* renderWindowPart);

  Ui::MatchPointRegistrationManipulatorControls m_Controls;

private:
  QWidget *m_Parent;

  void Error(QString msg);

  /** Methods returns a list of all eval nodes in the data manager.*/
  QList<mitk::DataNode::Pointer> GetEvalNodes();

  /**
  * Checks if appropriated nodes are selected in the data manager. If nodes are selected,
  * they are stored m_spSelectedRegNode, m_spSelectedInputNode and m_spSelectedRefNode.
  * They are also checked for vadility and stored in m_ValidInput,... .
  * It also sets the info lables accordingly.*/
  void CheckInputs();

  /**
  * Updates the state of controls regarding to selected eval object.*/
  void ConfigureControls();

  mitk::DataNode::Pointer m_selectedEvalNode;
  mitk::DataStorage::SetOfObjects::ConstPointer m_evalNodes;

  QmitkSliceNavigationListener m_SliceChangeListener;

  itk::TimeStamp m_selectedNodeTime;
  itk::TimeStamp m_currentPositionTime;

  /** @brief currently valid selected position in the inspector*/
  mitk::Point3D m_currentSelectedPosition;
  /** @brief indicates if the currently selected position is valid for the currently selected fit.
  * This it is within the input image */
  unsigned int m_currentSelectedTimeStep;

  mitk::DataNode::Pointer m_spSelectedRegNode;
  mitk::DataNode::Pointer m_spSelectedMovingNode;
  mitk::DataNode::Pointer m_spSelectedTargetNode;

  bool m_autoTarget;
  bool m_autoMoving;
  bool m_activeEvaluation;

  const std::string HelperNodeName;
};

#endif // MatchPoint_h


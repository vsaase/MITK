/*=========================================================================

 Program:   openCherry Platform
 Language:  C++
 Date:      $Date$
 Version:   $Revision$

 Copyright (c) German Cancer Research Center, Division of Medical and
 Biological Informatics. All rights reserved.
 See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notices for more information.

 =========================================================================*/

#include "cherryQtControlWidget.h"

#include <QMoveEvent>
#include <QResizeEvent>

#include <algorithm>

namespace cherry {

QtControlWidget::QtControlWidget(QWidget* parent, Qt::WindowFlags f)
 : QFrame(parent, f)
{
  this->setFrameStyle(QFrame::NoFrame);
}

void QtControlWidget::AddControlListener(GuiTk::IControlListener::Pointer listener)
{
  controlEvents.AddListener(listener);
}

void QtControlWidget::RemoveControlListener(GuiTk::IControlListener::Pointer listener)
{
  controlEvents.RemoveListener(listener);
}

void QtControlWidget::moveEvent(QMoveEvent* event)
{
  GuiTk::ControlEvent::Pointer controlEvent = new GuiTk::ControlEvent(this, event->pos().x(), event->pos().y(),
      0, 0);
  controlEvents.movedEvent(controlEvent);
}

void QtControlWidget::resizeEvent(QResizeEvent* event)
{
  GuiTk::ControlEvent::Pointer controlEvent = new GuiTk::ControlEvent(this, 0, 0,
      event->size().width(), event->size().height());
  controlEvents.resizedEvent(controlEvent);
}

void QtControlWidget::focusInEvent(QFocusEvent* /*event*/)
{
  GuiTk::ControlEvent::Pointer controlEvent = new GuiTk::ControlEvent(this, 0, 0, 0, 0);
  controlEvents.activatedEvent(controlEvent);
}

}

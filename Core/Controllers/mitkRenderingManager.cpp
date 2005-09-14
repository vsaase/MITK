/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
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

#include "mitkRenderingManager.h"
#include "mitkRenderingManagerFactory.h"
#include "mitkRenderWindow.h"

mitk::RenderingManager::Pointer mitk::RenderingManager::m_Instance;
mitk::RenderingManagerFactory *mitk::RenderingManager::m_RenderingManagerFactory;

void
mitk::RenderingManager
::SetFactory( RenderingManagerFactory *factory )
{
  m_RenderingManagerFactory = factory;
}

mitk::RenderingManager::Pointer
mitk::RenderingManager
::GetInstance()
{
  if ( !RenderingManager::m_Instance )
  {
    if ( m_RenderingManagerFactory )
    {
      RenderingManager *rawPointer =
        m_RenderingManagerFactory->CreateRenderingManager();
      m_Instance = rawPointer;
      rawPointer->UnRegister();
    }
  }

  return m_Instance;
}

mitk::RenderingManager::Pointer 
mitk::RenderingManager
::New()
{ 
  return RenderingManager::GetInstance();
}

mitk::RenderingManager::RenderingManager()
: m_UpdatePending( false ), m_Interval( 10 )
{
  // The default (minimum) interval is 10 msec, theoretically enabling a
  // maximum frame rate of 100 Hz.
}

mitk::RenderingManager::~RenderingManager()
{
}

void
mitk::RenderingManager
::AddRenderWindow( RenderWindow *renderWindow )
{
  m_RenderWindowList[renderWindow] = false;
}

void
mitk::RenderingManager
::RemoveRenderWindow( RenderWindow *renderWindow )
{
  m_RenderWindowList.erase( renderWindow );
}

void
mitk::RenderingManager
::RequestUpdate( RenderWindow *renderWindow )
{
  m_RenderWindowList[renderWindow] = true;
  
  if ( !m_UpdatePending )
  {
    m_UpdatePending = true;
    this->RestartTimer();
  }
}

void
mitk::RenderingManager
::ForceImmediateUpdate( RenderWindow *renderWindow )
{
  // Immediately repaint this window (implementation platform specific)
  renderWindow->Repaint();

  // Erase potentially pending requests for this window
  m_RenderWindowList[renderWindow] = false;

  // Check if there are pending requests for any other windows
  m_UpdatePending = false;
  RenderWindowList::iterator it;
  for ( it = m_RenderWindowList.begin(); it != m_RenderWindowList.end(); ++it )
  {
    if ( it->second )
    {
      m_UpdatePending = true;
    }
  }

  // Stop the timer if no more requests are pending
  if ( !m_UpdatePending )
  {
    this->StopTimer();
  }
}

void
mitk::RenderingManager
::RequestUpdateAll()
{
  RenderWindowList::iterator it;
  for ( it = m_RenderWindowList.begin(); it != m_RenderWindowList.end(); ++it )
  {
    it->second = true;
  }

  // Restart the timer if there are no requests already
  if ( !m_UpdatePending )
  {
    m_UpdatePending = true;
    this->RestartTimer();
  }
}

void
mitk::RenderingManager
::ForceImmediateUpdateAll()
{
  RenderWindowList::iterator it;
  for ( it = m_RenderWindowList.begin(); it != m_RenderWindowList.end(); ++it )
  {
    // Immediately repaint this window (implementation platform specific)
    it->first->Repaint();

    // Erase potentially pending requests
    it->second = false;
  }

  if ( m_UpdatePending )
  {
    this->StopTimer();
    m_UpdatePending = false;
  }
}

void
mitk::RenderingManager
::SetMinimumInterval( int msec )
{
  if ( msec < 0 )
  {
    msec = 0;
  }
  if ( msec > 5000 )
  {
    msec = 5000;
  }

  m_Interval = msec;
}

int
mitk::RenderingManager
::GetMinimumInterval() const
{
  return m_Interval;
}

void
mitk::RenderingManager
::UpdateCallback()
{
  m_UpdatePending = false;

  // Satisfy all pending update requests
  RenderWindowList::iterator it;
  for ( it = m_RenderWindowList.begin(); it != m_RenderWindowList.end(); ++it )
  {
    if ( it->second )
    {
      this->ForceImmediateUpdate( it->first );
      it->second = false;
    }
  }
}

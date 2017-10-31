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
#ifndef itkTractClusteringFilter_h
#define itkTractClusteringFilter_h

// MITK
#include <mitkPlanarEllipse.h>
#include <mitkFiberBundle.h>
#include <mitkFiberfoxParameters.h>

// ITK
#include <itkProcessObject.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>

namespace itk{

/**
* \brief    */

class TractClusteringFilter : public ProcessObject
{
public:

  struct Cluster
  {
    vnl_matrix<float> h;
    std::vector< long > I;
    int n;

    bool operator <(Cluster const& b) const
    {
      return this->n < b.n;
    }
  };

  enum Metric
  {
    MDF,
    MDF_VAR,
    MAX_MDF
  };

  typedef TractClusteringFilter Self;
  typedef ProcessObject                                       Superclass;
  typedef SmartPointer< Self >                                Pointer;
  typedef SmartPointer< const Self >                          ConstPointer;
  typedef itk::Image< float, 3 >                              FloatImageType;

  itkFactorylessNewMacro(Self)
  itkCloneMacro(Self)
  itkTypeMacro( TractClusteringFilter, ProcessObject )

  itkSetMacro(NumPoints, unsigned int)
  itkGetMacro(NumPoints, unsigned int)
  itkSetMacro(MinClusterSize, unsigned int)
  itkGetMacro(MinClusterSize, unsigned int)
  itkSetMacro(MaxClusters, unsigned int)
  itkGetMacro(MaxClusters, unsigned int)
  itkSetMacro(Scale, float)
  itkGetMacro(Scale, float)

  itkSetMacro(Tractogram, mitk::FiberBundle::Pointer)
  itkSetMacro(ScalarMap, FloatImageType::Pointer)

  virtual void Update() override{
    this->GenerateData();
  }

  void SetDistances(const std::vector<float> &Distances);

  std::vector<mitk::FiberBundle::Pointer> GetOutTractograms() const;

  void SetMetric(const Metric &Metric);

protected:

  void GenerateData() override;
  std::vector< vnl_matrix<float> > ResampleFibers();
  float CalcMDF(vnl_matrix<float>& s, vnl_matrix<float>& t, bool &flipped);
  float CalcMDF_VAR(vnl_matrix<float>& s, vnl_matrix<float>& t, bool &flipped);
  float CalcMAX_MDF(vnl_matrix<float>& s, vnl_matrix<float>& t, bool &flipped);

  std::vector< Cluster > ClusterStep(std::vector< long > f_indices, std::vector< float > distances);

  void AppendCluster(std::vector< Cluster >& a, std::vector< Cluster >&b);

  TractClusteringFilter();
  virtual ~TractClusteringFilter();

  unsigned int                                m_NumPoints;
  std::vector< float >                        m_Distances;
  mitk::FiberBundle::Pointer                  m_Tractogram;
  std::vector< mitk::FiberBundle::Pointer >   m_OutTractograms;
  std::vector<vnl_matrix<float> >             T;
  unsigned int                                m_MinClusterSize;
  unsigned int                                m_MaxClusters;
  Metric                                      m_Metric;
  FloatImageType::Pointer                     m_ScalarMap;
  float                                       m_Scale;
};
}

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkTractClusteringFilter.cpp"
#endif

#endif

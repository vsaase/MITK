#include "mitkSplineVtkMapper3D.h"
#include <vtkProp.h>
#include <vtkAssembly.h>
#include <vtkCardinalSpline.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkCellArray.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkTubeFilter.h>


mitk::SplineVtkMapper3D::SplineVtkMapper3D()
        : m_SplineProp( NULL ), m_Assembly( NULL )
{}




mitk::SplineVtkMapper3D::~SplineVtkMapper3D()
{
    if ( m_SplineProp != NULL )
        m_SplineProp->Delete();

    if ( m_Assembly != NULL )
        m_Assembly->Delete();
}




vtkProp*
mitk::SplineVtkMapper3D::GetProp()
{
    if ( GetDataTreeNode() != NULL && m_Assembly != NULL )
    {
        m_Assembly->SetUserTransform( GetDataTreeNode()->GetVtkTransform() );
    }

    return m_Assembly;
}


void
mitk::SplineVtkMapper3D::GenerateData()
{
    Superclass::GenerateData();
    std::cout << "GenerateData called" << std::endl;
    mitk::PointSet::Pointer input = const_cast<mitk::PointSet*>( this->GetInput( ) );
    input->Update();

    // Number of points on the spline
    unsigned int numberOfOutputPoints = 400;
    unsigned int numberOfInputPoints = input->GetSize();


    if ( numberOfInputPoints >= 2 )
    {
        std::cout << "splines calculated" << std::endl;
        vtkCardinalSpline* splineX = vtkCardinalSpline::New();
        vtkCardinalSpline* splineY = vtkCardinalSpline::New();
        vtkCardinalSpline* splineZ = vtkCardinalSpline::New();

        for ( unsigned int i = 0 ; i < numberOfInputPoints; ++i )
        {
            mitk::PointSet::PointType point = input->GetPoint( i );
            splineX->AddPoint( i, point[ 0 ] );
            splineY->AddPoint( i, point[ 1 ] );
            splineZ->AddPoint( i, point[ 2 ] );
        }
        vtkPoints* points = vtkPoints::New();
        vtkPolyData* profileData = vtkPolyData::New();


        // Interpolate x, y and z by using the three spline filters and
        // create new points
        double t = 0.0f;
        for ( unsigned int i = 0; i < numberOfOutputPoints; ++i )
        {
            t = ( ( ( ( double ) numberOfInputPoints ) - 1.0f ) / ( ( ( double ) numberOfOutputPoints ) - 1.0f ) ) * ( ( double ) i );
            points->InsertPoint( i, splineX->Evaluate( t ), splineY->Evaluate( t ), splineZ->Evaluate( t ) ) ;
        }

        // Create the polyline.
        vtkCellArray* lines = vtkCellArray::New();
        lines->InsertNextCell( numberOfOutputPoints );
        for ( unsigned int i = 0; i < numberOfOutputPoints; ++i )
            lines->InsertCellPoint( i );

        profileData->SetPoints( points );
        profileData->SetLines( lines );

        // Add thickness to the resulting line.
        //vtkTubeFilter* profileTubes = vtkTubeFilter::New();
        //profileTubes->SetNumberOfSides(8);
        //profileTubes->SetInput(profileData);
        //profileTubes->SetRadius(.005);

        vtkPolyDataMapper* profileMapper = vtkPolyDataMapper::New();
        profileMapper->SetInput( profileData );

        m_SplineProp = vtkActor::New();
        m_SplineProp->SetMapper( profileMapper );
        float rgba[ 4 ] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_SplineProp->GetProperty()->SetColor( rgba );
    }
    else
    {
        m_SplineProp = NULL;
    }
}

void
mitk::SplineVtkMapper3D::GenerateOutputInformation()
{
    Superclass::GenerateOutputInformation();
    mitk::Image::Pointer output = this->GetOutput();
    mitk::PixelType pt( typeid( int ) );
    unsigned int dim[] = {256, 256};
    output->Initialize( mitk::PixelType( typeid( short int ) ), 2, dim, 10 );
    SlicedData::RegionType::SizeType size = {0, 0, 0, 0, 0};
    SlicedData::RegionType region( size );
    output->SetRequestedRegion( &region );
}




void mitk::SplineVtkMapper3D::Update( mitk::BaseRenderer* renderer )
{
    //std::cout << "Update called" << std::endl;
    Superclass::Update( renderer );
    
    m_Assembly = vtkAssembly::New();
    if ( m_SplineProp != NULL )
        m_Assembly->AddPart( m_SplineProp );
    if ( m_Prop3D != NULL )
        m_Assembly->AddPart( m_Prop3D );
    
    if ( IsVisible( renderer ) == false )
    {
        if ( m_SplineProp != NULL )
            m_SplineProp->VisibilityOff();

        if ( m_Assembly != NULL )
            m_Assembly->VisibilityOff();
    }
    else
    {
        if ( m_SplineProp != NULL )
            m_SplineProp->VisibilityOn();

        if ( m_Assembly != NULL )
            m_Assembly->VisibilityOn();
    }
    StandardUpdate();
}

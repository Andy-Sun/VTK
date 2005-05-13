/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkExecutive.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkPolyDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkMath.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkHierarchicalPolyDataMapper, "1.2");
vtkStandardNewMacro(vtkHierarchicalPolyDataMapper);

class vtkHierarchicalPolyDataMapperInternals
{
public:
  vtkstd::vector<vtkPolyDataMapper*> Mappers;
};

vtkHierarchicalPolyDataMapper::vtkHierarchicalPolyDataMapper()
{
  this->Internal = new vtkHierarchicalPolyDataMapperInternals;
}

vtkHierarchicalPolyDataMapper::~vtkHierarchicalPolyDataMapper()
{
  cout << "There are " << this->Internal->Mappers.size() << " mappers " << endl;
  
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    cout << "ref count is " << this->Internal->Mappers[i]->GetReferenceCount() << endl;
    
    this->Internal->Mappers[i]->Delete();
    }
  this->Internal->Mappers.clear();
  
  delete this->Internal;
}

// Specify the type of data this mapper can handle. If we are
// working with a regular (not hierarchical) pipeline, then we
// need vtkPolyData. For composite data pipelines, then
// vtkHierarchicalDataSet is required, and we'll check when
// building our structure whether all the part of the composite
// data set are polydata.
int vtkHierarchicalPolyDataMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkHierarchicalDataSet");
  return 1;
}    

// When the structure is out-of-date, recreate it by
// creating a mapper for each input data.
void vtkHierarchicalPolyDataMapper::BuildPolyDataMapper()
{
  int warnOnce = 0;
  
  //Delete pdmappers if they already exist.
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->Delete();
    }
  this->Internal->Mappers.clear();
  
  //Get the HierarchicalDataSet from the input
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkHierarchicalDataSet *input = vtkHierarchicalDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  
  // If it isn't hierarchical, maybe it is just a plain vtkPolyData
  if(!input) 
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));
    if ( pd )
      {
      vtkPolyDataMapper *pdmapper = vtkPolyDataMapper::New();
      pdmapper->SetInput(pd);
      this->Internal->Mappers.push_back(pdmapper);
      }
    }
  else
    {
    //for each data set build a vtkPolyDataMapper
    vtkCompositeDataIterator* iter = input->NewIterator();
    iter->GoToFirstItem();  
    while (!iter->IsDoneWithTraversal())
      {
      vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());      
      if (pd)
        {
        vtkPolyDataMapper *pdmapper = vtkPolyDataMapper::New();
        pdmapper->SetInput(pd);
        this->Internal->Mappers.push_back(pdmapper);
        }
      // This is not polydata - warn the user that there are non-polydata
      // parts to this data set which will not be rendered by this mapper
      else
        {
        if ( !warnOnce )
          {
          vtkErrorMacro("All data in the hierachical dataset must be polydata.");
          warnOnce = 1;
          }
        }
      iter->GoToNextItem();
      }
    }
  
  this->InternalMappersBuildTime.Modified();
  
}

void vtkHierarchicalPolyDataMapper::Render(vtkRenderer *ren, vtkActor *a)
{
  //If the PolyDataMappers are not up-to-date then rebuild them
  vtkCompositeDataPipeline * executive = 
    vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
  
  if(executive->GetPipelineMTime() > this->InternalMappersBuildTime.GetMTime())
    {
    this->BuildPolyDataMapper();    
    }
  
  //Call Render() on each of the PolyDataMappers
  for(unsigned int i=0;i<this->Internal->Mappers.size();i++)
    {
    this->Internal->Mappers[i]->SetClippingPlanes( this->GetClippingPlanes() );
    this->Internal->Mappers[i]->Render(ren,a);    
    }
}
vtkExecutive* vtkHierarchicalPolyDataMapper::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//Looks at each DataSet and finds the union of all the bounds
void vtkHierarchicalPolyDataMapper::ComputeBounds()
{
  vtkMath::UninitializeBounds(this->Bounds);
  
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0,0);
  vtkHierarchicalDataSet *input = vtkHierarchicalDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));

  // If we don't have hierarchical data, test to see if we have
  // plain old polydata. In this case, the bounds are simply
  // the bounds of the input polydata.
  if(!input) 
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(
      this->GetExecutive()->GetInputData(0, 0));
    if ( pd )
      {
      pd->GetBounds( this->Bounds );
      }
    return;
    }
  
  // We do have hierarchical data - so we need to loop over
  // it and get the total bounds.
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->GoToFirstItem();  
  double bounds[6];
  int i;
  
  while (!iter->IsDoneWithTraversal())
    {
    vtkPolyData *pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());    
    if (pd)
      {
      // If this isn't the first time through, expand bounds
      // we've compute so far based on the bounds of this
      // block
      if ( vtkMath::AreBoundsInitialized(this->Bounds) )
        {
        pd->GetBounds(bounds);
        for(i=0; i<3; i++)
          {
          this->Bounds[i*2] = 
            (bounds[i*2]<this->Bounds[i*2])?
            (bounds[i*2]):(this->Bounds[i*2]);
          this->Bounds[i*2+1] = 
            (bounds[i*2+1]>this->Bounds[i*2+1])?
            (bounds[i*2+1]):(this->Bounds[i*2+1]);
          }
        }
      // If this is our first time through, just get the bounds
      // of the data as the initial bounds
      else
        {
        pd->GetBounds(this->Bounds);
        }
      }
    iter->GoToNextItem();
    }
  
  this->BoundsMTime.Modified();
}

double* vtkHierarchicalPolyDataMapper::GetBounds()
{
  static double bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};
  
  if ( ! this->GetInput() ) 
    {
    return bounds;
    }
  else
    {

    this->Update();
    
    //only compute bounds when the input data has changed
    vtkCompositeDataPipeline * executive = vtkCompositeDataPipeline::SafeDownCast(this->GetExecutive());
    if( executive->GetPipelineMTime() > this->BoundsMTime.GetMTime() )
      {
      ComputeBounds();
      }
    
    return this->Bounds;
    }
}

void vtkHierarchicalPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


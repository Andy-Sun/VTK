vtk_module(vtkRenderingParallel
  DEPENDS
    vtkParallelCore
    vtkFiltersParallel
    vtkRenderingOpenGL
  TEST_DEPENDS
    vtkParallelMPI
    vtkFiltersParallelMPI
    vtkTestingRendering
    vtkImagingSources
    vtkRenderingOpenGL
  )
#ifndef __vtkEllipticalSRep_h
#define __vtkEllipticalSRep_h

#include "vtkMeshSRepInterface.h"
#include "vtkSRepSkeletalPoint.h"

#include "vtkSlicerSRepModuleMRMLExport.h"

class VTK_SLICER_SREP_MODULE_MRML_EXPORT vtkEllipticalSRep
  : public vtkMeshSRepInterface
{
public:
  static vtkEllipticalSRep* New();
  ~vtkEllipticalSRep();

  /// Standard methods for a VTK class.
  vtkTypeMacro(vtkEllipticalSRep, vtkMeshSRepInterface);

  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  /////////////////////////////////////////////////////////
  // vtkMeshSRepInterface overrides
  /////////////////////////////////////////////////////////
  //using covariant return type for Clone
  VTK_NEWINSTANCE vtkEllipticalSRep* Clone() const override;
  /// @{
  /// Any changes the SRep via Resize/Clear/Set methods will not invalidate these
  /// pointers/references (but they could invalidate iterators into the vectors or
  /// any pointer returned from the spoke meshes)
  bool IsEmpty() const override;
  const vtkSRepSpokeMesh* GetUpSpokes() const;
  const vtkSRepSpokeMesh* GetDownSpokes() const;
  const vtkSRepSpokeMesh* GetCrestSpokes() const;
  const std::vector<IndexType>& GetCrestToUpSpokeConnections() const override;
  const std::vector<IndexType>& GetCrestToDownSpokeConnections() const override;
  const std::vector<IndexType>& GetUpSpine() const override;
  const std::vector<IndexType>& GetDownSpine() const override;
  /// @}

  /////////////////////////////////////////////////////////
  // vtkEllipticalSRep methods
  /////////////////////////////////////////////////////////
  vtkSmartPointer<vtkEllipticalSRep> SmartClone() const;
  /// @{
  /// Gets/sets the skeletal point. Shallow copy on the set.
  /// \throws std::out_of_range if InBounds(line, step) returns false
  const vtkSRepSkeletalPoint* GetSkeletalPoint(IndexType line, IndexType step) const
    VTK_EXPECTS(InBounds(line,step));
  vtkSRepSkeletalPoint* GetSkeletalPoint(IndexType line, IndexType step)
    VTK_EXPECTS(InBounds(line,step));
  void SetSkeletalPoint(IndexType line, IndexType step, vtkSRepSkeletalPoint* skeletalPoint)
    VTK_EXPECTS(CanSet(line, step, skeletalPoint));
  /// @}
  /// Similar to SetSkeletalPoint, but takes ownership of the pointer.
  /// \throws std::out_of_range if InBounds(line, step) returns false
  void TakeSkeletalPoint(IndexType line, IndexType step, vtkSRepSkeletalPoint* skeletalPoint)
    VTK_EXPECTS(CanSet(line, step, skeletalPoint));

  /// This will resize the SRep, filling in the new spaces with default constructed
  /// SkeletalPoints
  void Resize(IndexType lines, IndexType steps);

  /// Clears the SRep down to 0 lines, 0 steps
  void Clear();

  /// Returns true if the step is the crest. Only the last step will ever be the crest.
  bool IsCrestStep(IndexType step) const;

  /// Returns true if line/step are in bounds of srep, false otherwise
  bool InBounds(IndexType line, IndexType step) const;

  /// Returns true if the skeletal point can be set at line and step.
  ///
  /// To returns true, all the following must be true: InBounds(line, step) returns true,
  /// skeletalPoint is not nullptr, skeletalPoint must if a crest point if and only if
  /// IsCrestStep(step) is true.
  bool CanSet(IndexType line, IndexType step, vtkSRepSkeletalPoint* skeletalPoint) const;

  /// Gets the number of lines in the SRep
  IndexType GetNumberOfLines() const;

  /// Gets the number of steps in the SRep. The spine (step 0) is counted in this number.
  IndexType GetNumberOfSteps() const;

  /// Update the modification time for this object.
  ///
  /// This is overridden so a modified blocker mechanism can be implemented
  /// for batching the modified call
  void Modified() override;

  /// RAII class for blocking and unblocking modify for a vtkEllipticalSRep
  /// \sa BlockModify, UnblockModify
  class ModifiedBlocker {
  public:
    ModifiedBlocker(vtkEllipticalSRep* srep);
    ~ModifiedBlocker();
  private:
    vtkSmartPointer<vtkEllipticalSRep> Parent;
  };

  /// Will block all Modified calls from producing a modified event
  /// \sa UnblockModify, ModifiedBlocker
  void BlockModify();

  /// Will allow Modified calls to produce modified events.
  ///
  /// If there were any calls to Modified() while the block was active, a modified event
  /// will be produced by this function. If this is called when there was no block from
  /// BlockModify(), then this function does nothing.
  /// \sa BlockModify, ModifiedBlocker
  void UnblockModify();
protected:
  vtkEllipticalSRep();
  vtkEllipticalSRep(const vtkEllipticalSRep&) = delete;
  vtkEllipticalSRep(vtkEllipticalSRep&&) = delete;
  vtkEllipticalSRep& operator=(const vtkEllipticalSRep&) = delete;
  vtkEllipticalSRep& operator=(vtkEllipticalSRep&&) = delete;
private:
  using LineOutFromSpine = std::vector<vtkSmartPointer<vtkSRepSkeletalPoint>>;
  using UnrolledEllipticalGrid = std::vector<LineOutFromSpine>;
  struct MeshRepresentation {
      vtkNew<vtkSRepSpokeMesh> UpSpokes;
      vtkNew<vtkSRepSpokeMesh> DownSpokes;
      vtkNew<vtkSRepSpokeMesh> CrestSpokes;
      std::vector<IndexType> CrestToUpSpokeConnections;
      std::vector<IndexType> UpSpine;
      std::vector<IndexType> CrestToDownSpokeConnections;
      std::vector<IndexType> DownSpine;
  };
  
  UnrolledEllipticalGrid Skeleton;
  std::vector<std::vector<unsigned long>> SkeletonObservationTags;
  int ModifiedBlocks;
  bool WasModifiedDuringBlock;
  MeshRepresentation SkeletonAsMesh;

  void CheckInBounds(IndexType line, IndexType step) const;
  void CheckCanSet(IndexType line, IndexType step, vtkSRepSkeletalPoint* skeletalPoint) const;
  void onSkeletalPointModified(vtkObject *caller, unsigned long event, void* callData);

  IndexType NumberOfSpinePointsWithoutDuplicates() const;
  vtkSRepSpokeMesh::IndexType LineStepToUpDownMeshIndex(IndexType line, IndexType step) const;
  std::vector<vtkSRepSpokeMesh::IndexType> GetNeighbors(IndexType line, IndexType step) const;
  void CreateMeshRepresentation();

  // if you call this function, you must update the mesh rep yourself and call this->Modified yourself
  void SetSkeletalPointNoMeshUpdate(IndexType line, IndexType step, vtkSRepSkeletalPoint* skeletalPoint);
};

#endif

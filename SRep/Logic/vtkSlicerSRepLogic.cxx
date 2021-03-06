/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// SRep Logic includes
#include "vtkSlicerSRepLogic.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSRepDisplayNode.h>
#include <vtkMRMLSRepStorageNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <cassert>

#include "vtkMRMLSRepNode.h"
#include "SRepInterpolation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSRepLogic);

//----------------------------------------------------------------------------
vtkSlicerSRepLogic::vtkSlicerSRepLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSRepLogic::~vtkSlicerSRepLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSRepLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSRepLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSRepLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);

  vtkMRMLScene *scene = this->GetMRMLScene();

  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLEllipticalSRepNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLSRepDisplayNode>::New());
  scene->RegisterNodeClass(vtkSmartPointer<vtkMRMLSRepStorageNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSRepLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerSRepLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerSRepLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//----------------------------------------------------------------------------
std::string vtkSlicerSRepLogic::AddNewEllipticalSRepNode(const std::string& name, vtkMRMLScene* scene) {
  std::string id;
  if (!scene && !this->GetMRMLScene()) {
    vtkErrorMacro("AddNewEllipticalSRepNode: no scene to add a srep node to!");
    return id;
  }

  vtkMRMLScene *addToThisScene = scene ? scene : this->GetMRMLScene();

  // create and add the node
  auto mnode = vtkSmartPointer<vtkMRMLEllipticalSRepNode>::New();
  addToThisScene->AddNode(mnode);

  // add a display node
  std::string displayID = this->AddFirstDisplayNodeForSRepNode(mnode);

  if (displayID.compare("") != 0) {
    // get the node id to return
    id = std::string(mnode->GetID());
    if (!name.empty()) {
      mnode->SetName(name.c_str());
    }
  }

  return id;
}

//----------------------------------------------------------------------------
std::string vtkSlicerSRepLogic::AddFirstDisplayNodeForSRepNode(vtkMRMLSRepNode *srepNode) {
  const std::string emptyId;
  if (!srepNode || !srepNode->GetScene()) {
    vtkErrorMacro("AddNewDisplayNodeForSRepNode: unable to add a srep display node!");
    return emptyId;
  }

  if (srepNode->GetDisplayNode()) {
    return srepNode->GetDisplayNodeID();
  }

  srepNode->CreateDefaultDisplayNodes();
  auto* displayNode = vtkMRMLSRepDisplayNode::SafeDownCast(srepNode->GetDisplayNode());
  if (!displayNode) {
    vtkErrorMacro("AddNewDisplayNodeForSRepNode: error creating new display node");
    return emptyId;
  }

  return displayNode->GetID();
}

//----------------------------------------------------------------------------
const char* vtkSlicerSRepLogic::LoadSRep(const char* fileName, const char* nodeName) {
  if (!fileName) {
    vtkErrorMacro("LoadSRep: null file, cannot load");
    return nullptr;
  }

  vtkDebugMacro("LoadSRep, file name = " << fileName << ", nodeName = " << (nodeName ? nodeName : "null"));

  vtkMRMLSRepStorageNode* storageNode = vtkMRMLSRepStorageNode::SafeDownCast(
    this->GetMRMLScene()->AddNewNodeByClass("vtkMRMLSRepStorageNode"));

  if (!storageNode) {
    vtkErrorMacro("LoadSRep: failed to instantiate srep storage node by class vtkMRMLSRepStorageNode");
    return nullptr;
  }

  storageNode->SetFileName(fileName);
  vtkMRMLSRepNode* srepNode = storageNode->CreateSRepNode(nodeName);
  if (!srepNode) {
    return nullptr;
  }
  return srepNode->GetID();

}

//----------------------------------------------------------------------------
std::string vtkSlicerSRepLogic::InterpolateSRep(vtkMRMLEllipticalSRepNode* srepNode, size_t interpolationlevel, const std::string& newNodeName) {
  auto scene = this->GetMRMLScene();
  if (!scene) {
    vtkErrorMacro("InterpolateSRep: no scene to add a srep node to!");
    return "";
  }

  const auto nodeID = AddNewEllipticalSRepNode(newNodeName, scene);
  if (nodeID.empty()) {
    vtkErrorMacro("InterpolateSRep: Error making Elliptical SRep node");
    return "";
  }

  auto interpolatedSRepNode = vtkMRMLEllipticalSRepNode::SafeDownCast(scene->GetNodeByID(nodeID));
  if (!interpolatedSRepNode) {
    vtkErrorMacro("InterpolateSRep: Unable to find newly created SRep node: " << nodeID);
    return "";
  }

  const bool success = this->InterpolateSRep(srepNode, interpolationlevel, interpolatedSRepNode);
  if (!success) {
    scene->RemoveNode(interpolatedSRepNode);
    return "";
  }
  return nodeID;
}

//----------------------------------------------------------------------------
bool vtkSlicerSRepLogic::InterpolateSRep(vtkMRMLEllipticalSRepNode* srepNode, size_t interpolationlevel, vtkMRMLEllipticalSRepNode* destination) {
  if (!destination) {
    vtkErrorMacro("InterpolateSRep: no destination");
    return false;
  }
  
  if (!srepNode) {
    vtkErrorMacro("InterpolateSRep: input node is nullptr");
    return false;
  }

  auto srep = srepNode->GetEllipticalSRep();
  if (!srep) {
    vtkErrorMacro("InterpolateSRep: input node does not have an SRep");
    return false;
  }

  if (interpolationlevel == 0) {
    destination->SetEllipticalSRep(srep->SmartClone());
    return true;
  } else {

    auto interpolatedSRep = this->SmartInterpolateSRep(*srep, interpolationlevel);
    if (!interpolatedSRep) {
      vtkErrorMacro("InterpolateSRep: Unable to interpolate SRep");
      return false;
    }

    destination->SetEllipticalSRep(interpolatedSRep);
    return true;
  }
}

//----------------------------------------------------------------------------
vtkEllipticalSRep* vtkSlicerSRepLogic::InterpolateSRep(const vtkEllipticalSRep& srep, size_t interpolationlevel) {
  auto ret = SmartInterpolateSRep(srep, interpolationlevel);
  if (ret) {
    ret->Register(nullptr);
  }
  return ret;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkEllipticalSRep> vtkSlicerSRepLogic::SmartInterpolateSRep(const vtkEllipticalSRep& srep, size_t interpolationlevel) {
  return sreplogic::SmartInterpolateSRep(interpolationlevel, srep);
}

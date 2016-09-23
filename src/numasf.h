#ifndef NUMASF_H_
#define NUMASF_H_

#include <iostream>
#include <sstream>
#include <vector>

#include "misc.h"

#ifdef _WIN32
#include <windows.h>
typedef BOOL (WINAPI *GLPIEX)(LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);
typedef BOOL (WINAPI *STGA)(HANDLE, GROUP_AFFINITY*, PGROUP_AFFINITY);
#else
#include <numa.h>
#endif

class NumaNode {

public:
  // The node number assigned by OS
  int nodeNumber; // FIXME this is a DWORD on windows

  // The number of physical cores in this node, this will be <= the number of
  // logical processors in this node.
  size_t coreCount;

#ifdef _WIN32
  // GROUP_AFFINITY structure contains a group number and a 64bit mask
  GROUP_AFFINITY groupMask;

  // When processor groups is not used groupMask should be -1, in this case
  // fallback on a 64bit mask.
  ULONGLONG mask;
#else
  // cpuBitset is a pointer to an allocated bitmask struct which itself contains
  // a pointer to a bitmask of variable size provided by libnuma.
  bitmask* cpuBitset;
#endif

#ifdef _WIN32
  // To use with Windows's processor groups
  NumaNode(DWORD _nodeNumber, const GROUP_AFFINITY& _groupMask) {
    nodeNumber = _nodeNumber;
    coreCount = 0;
    groupMask = _groupMask;
    mask = 0;
  }
  // To use withouth Windows's processor groups
  NumaNode(DWORD _nodeNumber, ULONGLONG _mask) {
    nodeNumber = _nodeNumber;
    coreCount = 0;
    groupMask.Group = 0xFFFF;
    groupMask.Mask = 0;
    mask = _mask;
  }

#else
  NumaNode(int _nodeNumber, bitmask* _cpuBitset) {
    nodeNumber = _nodeNumber;
    coreCount = 0;
    cpuBitset = _cpuBitset;
  }
#endif

  NumaNode(const NumaNode& nn) {
    nodeNumber = nn.nodeNumber;
    coreCount = nn.coreCount;
#ifdef _WIN32
    groupMask = nn.groupMask;
    mask = nn.mask;
#else
    cpuBitset = numa_allocate_cpumask();
    copy_bitmask_to_bitmask(source.cpuBitset, cpuBitset);
#endif
  }

  ~NumaNode() {
#ifndef _WIN32
    numa_bitmask_free(cpuBitset);
#endif
  }

  std::string print() {
    std::ostringstream ss;
    ss << "nodeNumber: " << nodeNumber << "\n";
    ss << "coreCount:  " << coreCount << "\n";

#ifdef _WIN32
    if (groupMask.Group != 0xFFFF) // Use processors groups?
    {
        ss << "Group:      " << groupMask.Group << "\n";
        ss << "Mask:       " << (void*)groupMask.Mask;
    } else
        ss << "mask:       " << (void*)mask;
#else
    ss << "cpuBitset: ";
    for (unsigned int i = 0; i < 8 * numa_bitmask_nbytes(cpuBitset); i++)
        if (numa_bitmask_isbitset(cpuBitset, i))
            ss << " " << i;
#endif
    return ss.str();
  }
};


class NumaState {

public:
  NumaState();

  // Preferred node for a given search thread
  NumaNode* nodeForThread(size_t threadNo);

  // Bind current thread to node
  void bindThread(NumaNode* numaNode);

  // Print out all of the nodes to stdout
  void display() {
    for (auto& nn : nodeVector)
        sync_cout << nn.print() << sync_endl;
  }

  // Imported functions that are not present in all kernel32.dll's
#ifdef _WIN32
  GLPIEX imp_GetLogicalProcessorInformationEx;
  STGA   imp_SetThreadGroupAffinity;
#endif

  // If numa functions are not present, there should be a dummy node in it with
  // nodeNumber = -1, this vector should not be empty.
  std::vector<NumaNode> nodeVector;

  // Total number of cores in all nodes. It is set to 1 when using a dummy node
  size_t coreCount;
};

extern NumaState NumaInfo;

#endif

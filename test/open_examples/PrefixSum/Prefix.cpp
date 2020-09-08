/*
 * Copyright (c) 2017, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cm_rt.h"

// Includes bitmap_helpers.h for bitmap file open/save/compare operations.
#include "../common/bitmap_helpers.h"

// Include cm_rt_helpers.h to convert the integer return code returned from
// the CM runtime to a meaningful string message.
#include "../common/cm_rt_helpers.h"

// Includes isa_helpers.h to load the ISA file generated by the CM compiler.
#include "../common/isa_helpers.h"
#include <time.h>

#include "Prefix.h"

using namespace std;


void cmk_sum_tuple_count(SurfaceIndex table);
void cmk_prefix(SurfaceIndex table);

void  compute_prefixsum(unsigned int input[], unsigned int prefixSum[], unsigned int size) {

  for (int j = 0; j < TUPLE_SZ; j++) // init first entry
    prefixSum[j] = input[j];

  for (int i = 1; i < size; i++) {
    for (int j = 0; j < TUPLE_SZ; j++) {
      prefixSum[i*TUPLE_SZ + j] = input[i*TUPLE_SZ + j] +
        prefixSum[(i - 1)*TUPLE_SZ + j];
    }
  }
}



void dump_table(unsigned int prefixSum[], unsigned int size) {

  for (int i = 0; i < size; i++) {
    cout << "[" << i << "]";
    for (int j = 0; j < TUPLE_SZ; j++)
      cout << "\t" << prefixSum[i*TUPLE_SZ + j];
    cout << endl;
  }
}


int main(int argc, char * argv[])
{
  unsigned int * pInputs;

  if (argc < 2) {
    cout << "Usage: prefix [N]. N is 2^N entries x TUPLE_SZ" << endl;
    exit(1);
  }
  unsigned log2_element = atoi(argv[1]);
  unsigned int size = 1 << log2_element;

  pInputs = (unsigned int*)CM_ALIGNED_MALLOC(
             size * TUPLE_SZ * sizeof(unsigned int), 0x1000);
  for (unsigned int i = 0; i < size * TUPLE_SZ; ++i) {
    pInputs[i] = rand() % 128;
  }
  // prepare output buffer for sorted result
  float *t = new float[20];
  unsigned int * pExpectOutputs = (unsigned int*)malloc(
		  size * TUPLE_SZ * sizeof(unsigned int));
  // cpu time
  clock_t s = clock();
  compute_prefixsum(pInputs, pExpectOutputs, size);
  unsigned int cpu_time = clock() - s;

  // Creates a CmDevice
  CmDevice *device = nullptr;
  unsigned int version = 0;
  cm_result_check(::CreateCmDevice(device, version));
  // The file Prefix_genx.isa is generated when the kernels in the file
  // Prefix_genx.cpp are compiled by the CM compiler.
  // Reads in the virtual ISA from "Prefix_genx.isa" to the code
  // buffer.
  std::string isa_code = cm::util::isa::loadFile("Prefix_genx.isa");
  if (isa_code.size() == 0) {
    cout << "Error: empty ISA binary.\n";
    exit(1);
  }
  // Creates a CmProgram object
  CmProgram *program = nullptr;
  cm_result_check(device->LoadProgram(const_cast<char*>(isa_code.data()),
    isa_code.size(),
    program));

  // create the segment sum kernel
  CmKernel *sum_kernel = nullptr;
  cm_result_check(device->CreateKernel(program,
    CM_KERNEL_FUNCTION(cmk_sum_tuple_count),
    sum_kernel));
  // create the per-segment prefix-update kernel
  CmKernel *prefix_kernel = nullptr;
  cm_result_check(device->CreateKernel(program,
    CM_KERNEL_FUNCTION(cmk_prefix),
    prefix_kernel));
  // determine how many threads we need
  // each thread handling PREFIX_ENTRIES elements
  unsigned int width, height; // thread space width and height
  unsigned int total_threads = size / PREFIX_ENTRIES;
  if (total_threads < MAX_TS_WIDTH) {
    width = total_threads;
    height = 1;
  }
  else {
    width = MAX_TS_WIDTH;
    height = total_threads / MAX_TS_WIDTH;
  }
  // create buffers for input and output and prefix sum table
  CmBufferUP *inBuf;
  int c = device->CreateBufferUP(size * TUPLE_SZ * sizeof(unsigned int), (void *)pInputs, inBuf);
  cout << c << endl;

  // Gets the input surface index.
  SurfaceIndex *input_idx = nullptr;
  cm_result_check(inBuf->GetIndex(input_idx));

  // Creates a CmThreadSpace object.
  CmThreadSpace *thread_space = nullptr;
  cm_result_check(device->CreateThreadSpace(width, height, thread_space));

  // Creates a task queue.
  CmQueue *cmd_queue = nullptr;
  cm_result_check(device->CreateQueue(cmd_queue));

  // only call GPU prefix kernel when we have enough work to offload
  if (size >= 1024) {
    cm_result_check(sum_kernel->SetKernelArg(0, sizeof(SurfaceIndex), input_idx));
    cm_result_check(prefix_kernel->SetKernelArg(0, sizeof(SurfaceIndex), input_idx));
  }

  // Creates CmTask objects.
  // A CmTask object is a container for CmKernel pointers. It is used to
  // enqueue the kernels for execution.
  // Here we creates two tasks, one for each kernel.
  CmTask *sum_task = nullptr;
  cm_result_check(device->CreateTask(sum_task));
  CmTask *prefix_task = nullptr;
  cm_result_check(device->CreateTask(prefix_task));
  // Adds a CmKernel pointer to a CmTask.
  cm_result_check(sum_task->AddKernel(sum_kernel));
  cm_result_check(prefix_task->AddKernel(prefix_kernel));

  cout << "Prefix (" << size << ") Start..." << endl;

  clock_t start = clock(); // start timer

  // create a sync event
  CmEvent *event = nullptr;
  unsigned long time_out = -1;

#ifdef _DEBUG
  cout << "Input" << endl;
  dump_table(pInputs, size);
#endif
  // For small input size, we only have a small number of threads
  // we don't call kernel to compute prefix sum. intead of,
  // CPU simply performs the job
  if (size >= 1024) {
    // enqueue the task for per-segment sum
    cm_result_check(cmd_queue->Enqueue(sum_task, event, thread_space));
    cm_result_check(event->WaitForTaskFinished(time_out));
#ifdef _DEBUG
    cout << "[255] expected " << pExpectOutputs[255] << endl;
    cout << "Local Sum" << endl;
    dump_table(pInputs, size);
#endif
    // doing prefix-sum of those segement sum on CPU
    for (int j = 2; j <= size / PREFIX_ENTRIES; j++) {
      for (int m = 0; m < TUPLE_SZ; m++) {
        pInputs[(j * PREFIX_ENTRIES - 1)*TUPLE_SZ + m] +=
		pInputs[((j - 1)* PREFIX_ENTRIES - 1)*TUPLE_SZ + m];
      }
    }
#ifdef _DEBUG
    cout << "Sum" << endl;
    dump_table(pInputs, size);
#endif
    // enqueue the task that updates prefix-sums witin each segement
    cm_result_check(cmd_queue->Enqueue(prefix_task, event, thread_space));
    cm_result_check(event->WaitForTaskFinished(time_out));
#ifdef _DEBUG
    // validate prefix sum
    cout << "Prefix Sum" << endl;
    dump_table(pInputs, size);
#endif
  }
  clock_t end = clock(); // end timer
  cout << " GPU Prefix Time = " << end - start << " msec " << endl;
  cout << " CPU Prefix Time = " << cpu_time << " msec " << endl;
  bool pass = memcmp(pInputs, pExpectOutputs, size*TUPLE_SZ * sizeof(unsigned int)) == 0;
  cout << "Prefix " << (pass ? "=> PASSED" : "=> FAILED") << endl << endl;

  // Destroy a CmThreadSpace object.
  cm_result_check(device->DestroyThreadSpace(thread_space));
  // Destroys the CmDevice.
  // Also destroys surfaces, kernels, tasks, thread spaces, and queues that
  // were created using this device instance that have not explicitly been
  // destroyed by calling the respective destroy functions.
  cm_result_check(::DestroyCmDevice(device));

  CM_ALIGNED_FREE(pInputs);
  delete pExpectOutputs;
}

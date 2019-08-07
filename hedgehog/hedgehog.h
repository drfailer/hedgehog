//
// Created by 775backup on 2019-04-17.
//

#ifndef HEDGEHOG_HEDGEHOG_H
#define HEDGEHOG_HEDGEHOG_H

#include "api/graph.h"
#include "api/task/abstract_task.h"
#ifdef HH_USE_CUDA
#include "api/cuda/abstract_cuda_task.h"
#endif // HH_USE_CUDA
#include "api/memory_manager/memory_data.h"
#include "api/memory_manager/abstract_static_memory_manager.h"
#include "api/memory_manager/abstract_dynamic_memory_manager.h"
#include "api/state_manager/default_state_manager.h"
#include "api/execution_pipeline/abstract_execution_pipeline.h"

#endif //HEDGEHOG_HEDGEHOG_H

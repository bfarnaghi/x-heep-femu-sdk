/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#pragma message "hello_world_test.cc"
extern "C" {
  #include "lenet5_test.h"
  #include <math.h>
  #include <stdio.h>
  #include "core_v_mini_mcu.h"
}

#include "models/lenet5_input.h"
//#include "models/lenet5.h"
#include "models/lenet5_stolen.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/recording_micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {
constexpr int kTensorArenaSize = 0x4000;
uint8_t tensor_arena[kTensorArenaSize];
const tflite::Model* model = nullptr;

using Lenet5OpResolver = tflite::MicroMutableOpResolver<7>;

TfLiteStatus RegisterOps(Lenet5OpResolver& op_resolver) {
  TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
  TF_LITE_ENSURE_STATUS(op_resolver.AddConv2D());
  TF_LITE_ENSURE_STATUS(op_resolver.AddAveragePool2D());
  TF_LITE_ENSURE_STATUS(op_resolver.AddTanh());
  TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
  TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
  TF_LITE_ENSURE_STATUS(op_resolver.AddLogistic());
  return kTfLiteOk;
}
}  // namespace

TfLiteStatus LoadModel() {
  if (model != nullptr) {
    return kTfLiteOk;
  }
  model = ::tflite::GetModel(tflite_rom);
  TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);
  return kTfLiteOk;
}

TfLiteStatus Infer(const char *data, size_t len, int8_t **out, size_t *out_len) {
  if (model == nullptr) {
    return kTfLiteError;
  }
  Lenet5OpResolver op_resolver;
  TF_LITE_ENSURE_STATUS(RegisterOps(op_resolver));

  tflite::MicroInterpreter interpreter(model, op_resolver, tensor_arena,
                                       kTensorArenaSize);
  TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());
  TfLiteTensor* input = interpreter.input(0);
  TFLITE_CHECK_NE(input, nullptr);
  TfLiteTensor* output = interpreter.output(0);
  TFLITE_CHECK_NE(output, nullptr);
  memcpy(input->data.int8, data, len);
  TF_LITE_ENSURE_STATUS(interpreter.Invoke());
  *out = output->data.int8;
  *out_len = output->bytes;

  return kTfLiteOk;
}

extern "C" int init_tflite() {
  tflite::InitializeTarget();
  TF_LITE_ENSURE_STATUS(LoadModel());
  return kTfLiteOk;
}

extern "C" int infer(const char *data, size_t len, int8_t **out, size_t *out_len) {
  return Infer(data, len, out, out_len);
}
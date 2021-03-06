// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/cpu/math/clip.h"
#include "core/framework/data_types_internal.h"
#include "core/util/math_cpuonly.h"

namespace onnxruntime {

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Clip,
    6,
    10,
    KernelDefBuilder().MayInplace(0, 0).TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
    Clip_6<float>);

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Clip,
    11,
    11,
    KernelDefBuilder().MayInplace(0, 0).TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
    Clip);

#define REG_KERNEL_NONTEMPL(OP_TYPE, VERSION, KERNEL_CLASS, ...)          \
  ONNX_CPU_OPERATOR_KERNEL(                                                     \
      OP_TYPE,                                                                  \
      VERSION,                                                                  \
      KernelDefBuilder()                                                        \
          .MayInplace(0, 0)                                                     \
          .TypeConstraint("T", BuildKernelDefConstraints<__VA_ARGS__>()), \
      KERNEL_CLASS);

REG_KERNEL_NONTEMPL(Clip, 12, Clip, float, double, int8_t, uint8_t, int64_t, uint64_t);

template<typename T>
Status Clip_6<T>::Compute(OpKernelContext* ctx) const {
    const auto* X = ctx->Input<Tensor>(0);
    Tensor* Y = ctx->Output(0, X->Shape());
    EigenVectorMap<T>(Y->template MutableData<T>(), Y->Shape().Size()) =
        ConstEigenVectorMap<T>(X->template Data<T>(), X->Shape().Size())
            .cwiseMax(this->min_)
            .cwiseMin(this->max_);
    return Status::OK();
}

template <typename T>
struct Clip::ComputeImpl {
  void operator()(const Tensor* X, const Tensor* min, const Tensor* max, Tensor* Y) const {
    auto min_val = std::numeric_limits<T>::lowest();
    auto max_val = std::numeric_limits<T>::max();
    if (min) {
      ORT_ENFORCE(min->Shape().NumDimensions() == 0, "min should be a scalar.");
      min_val = *(min->template Data<T>());
    }
    if (max) {
      ORT_ENFORCE(max->Shape().NumDimensions() == 0, "max should be a scalar.");
      max_val = *(max->template Data<T>());
    }

    EigenVectorMap<T>(Y->template MutableData<T>(), Y->Shape().Size()) =
        ConstEigenVectorMap<T>(X->template Data<T>(), X->Shape().Size())
            .cwiseMax(min_val)
            .cwiseMin(max_val);
  }
};

Status Clip::Compute(OpKernelContext* ctx) const {
  const auto* X = ctx->Input<Tensor>(0);
  const auto* min = ctx->Input<Tensor>(1);
  const auto* max = ctx->Input<Tensor>(2);
  Tensor* Y = ctx->Output(0, X->Shape());

  utils::MLTypeCallDispatcher<ComputeImpl, float, double, int8_t, uint8_t, int64_t, uint64_t>
      t_disp(X->GetElementType());

  t_disp.Invoke(X, min, max, Y);

  return Status::OK();
}

}  // namespace onnxruntime

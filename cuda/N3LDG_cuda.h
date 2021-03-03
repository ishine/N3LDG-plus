#ifndef N3LDG_CUDA_N3LDG_CUDA_H
#define N3LDG_CUDA_N3LDG_CUDA_H

#include "Def.h"

#include "Memory_cuda.h"
#include <iostream>
#include <cassert>
#include <cuda.h>
#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <vector>
#include <cmath>
#include <exception>

namespace n3ldg_cuda {

class CudaVerificationException: public std::exception {
public:
    CudaVerificationException(int index) : index_(index) {}

    int getIndex() const {
        return index_;
    }

private:
    int index_;
};

template<typename T>
struct GPUArray {
    T *value = nullptr;
    int len = 0;

    GPUArray() = default;
    GPUArray(GPUArray<T>&&) {
        abort();
    }
    GPUArray(const GPUArray &) {
        abort();
    }

    void init(T *host_arr, int len);
    void init(int len);
    ~GPUArray();

    vector<T> toCpu() const;
};

cudaError_t MyCudaMemcpy(void *dest, void *src, size_t count, cudaMemcpyKind kind);
void CallCuda(cudaError_t status);

template <typename T>
void GPUArray<T>::init(T *host_arr, int len) {
    if (value != nullptr) {
        CallCuda(MemoryPool::Ins().Free(value));
        value = nullptr;
    }
    CallCuda(MemoryPool::Ins().Malloc((void**)&value, len * sizeof(T)));
    CallCuda(MyCudaMemcpy(value, host_arr, len * sizeof(T), cudaMemcpyHostToDevice));
    this->len = len;
}

template <typename T>
void GPUArray<T>::init(int len) {
    if (value != nullptr) {
        CallCuda(MemoryPool::Ins().Free(value));
        value = nullptr;
    }
    CallCuda(MemoryPool::Ins().Malloc((void**)&value, len * sizeof(T)));
    this->len = len;
}

template <typename T>
vector<T> GPUArray<T>::toCpu() const {
    vector<T> result;
    result.resize(len);
    if (value == nullptr) {
        cerr << "GPUArray::toCpu - value is nullptr" << endl;
        abort();
    }
    CallCuda(MyCudaMemcpy(result.data(), value, sizeof(T) * len, cudaMemcpyDeviceToHost));
    return result;
}

template <typename T>
GPUArray<T>::~GPUArray() {
    if (value != nullptr) {
        CallCuda(MemoryPool::Ins().Free(value));
        value = nullptr;
    }
}

typedef GPUArray<dtype> NumberArray;
typedef GPUArray<bool> BoolArray;
typedef GPUArray<bool *> BoolPointerArray;
typedef GPUArray<dtype *> NumberPointerArray;
typedef GPUArray<dtype **> NumberPointerPointerArray;
typedef GPUArray<int> IntArray;
typedef GPUArray<int *> IntPointerArray;

struct DeviceNumber {
    dtype *value = nullptr;
    dtype v = 0.0f;

    DeviceNumber() = default;
    DeviceNumber(DeviceNumber &&) {
        abort();
    }
    DeviceNumber(const DeviceNumber&) {
        abort();
    }

    void init();
    void copyFromDeviceToHost();
    ~DeviceNumber();
};

struct DeviceInt {
    int *value = nullptr;
    int v = 0;

    DeviceInt() = default;
    DeviceInt(DeviceInt &&) {
        abort();
    }
    DeviceInt(const DeviceInt&) {
        abort();
    }

    void init();
    void copyFromDeviceToHost();
    void copyFromHostToDevice();
    ~DeviceInt();
};

bool Verify(bool *host, bool *device, int len, const char* message);
bool Verify(int *host, int *device, int len, const char* message);

void Assert(bool v, const string &message = "",
        const function<void(void)> &call = []() {});
void Memset(dtype *p, int len, dtype value);
void Memset(bool *p, int len, bool value);
void *Malloc(int size);
void BatchMemset(vector<dtype*> &vec, int count, vector<int> &dims, dtype value);
void PrintNums(dtype* p, int len);
void PrintInts(int* p, int len);

void InitCuda(int device_id = 0, float memory_in_gb = 0.0f);
void EndCuda();

void CopyFromMultiVectorsToOneVector(vector<dtype*> &src, dtype *dest, int count,
        int len);
void CopyFromOneVectorToMultiVals(dtype *src, vector<dtype*> &vals,
        int count,
        int len);
void CopyFromHostToDevice(vector<dtype*> &src,
        vector<dtype*> &dest, int count, int dim);
void CopyFromDeviceToHost(vector<dtype*> &src,
        vector<dtype*> &dest, int count, int dim);

enum PoolingEnum {
    MAX,
    MIN,
    SUM,
    AVG
};

void ActivationForward(ActivatedEnum activated, vector<dtype*> &xs,
        int count,
        vector<int> &dims,
        vector<dtype*> &ys);
void ActivationBackward(ActivatedEnum activated, vector<dtype*> &losses,
        vector<dtype*> &vals,
        int count,
        vector<int> &dims,
        vector<dtype*> &in_losses);
void DropoutForward(vector<dtype*> &xs, int count, vector<int> &dims, int max_dim,
        vector<int> &offsets,
        bool is_training,
        dtype *drop_mask,
        dtype drop_factor,
        vector<dtype*> &ys);
void DropoutBackward(vector<dtype*> &grads, int count, vector<int> &dims, int max_dim,
        vector<int> &offsets,
        bool is_training,
        dtype *drop_mask,
        dtype drop_factor,
        vector<dtype*> &in_grads);
void BucketForward(vector<dtype> input, int count, int dim, vector<dtype*> &ys);
void CopyForUniNodeForward(vector<dtype*> &xs, dtype* b,
        dtype* xs_dest,
        dtype* b_dest,
        int count,
        int x_len,
        int b_len,
        bool use_b);
void MatrixMultiplyMatrix(dtype *W, dtype *x, dtype *y, int row, int col,
        int count,
        bool useb,
        bool should_x_transpose = false,
        bool should_W_transpose = false);
void LinearForward(vector<dtype *> &in_vals, int count, vector<int> &in_cols, int in_row,
        int out_row,
        dtype *W,
        dtype *bias,
        vector<dtype *> &vals);
void LinearBackward(vector<dtype *> &grads, int count, vector<int> &cols, int in_row, int out_row,
        dtype *W_val,
        vector<dtype *> &in_vals,
        dtype *bias_grad,
        vector<dtype *> &in_grads,
        dtype *W_grad);
void AddLtyToParamBiasAndAddLxToInputLossesForUniBackward(dtype *lty,
        dtype *lx, dtype *b, vector<dtype*> &losses, int count,
        int out_dim, int in_dim, bool use_b);
void AddLtyToParamBiasAndAddLxToInputLossesForBiBackward(dtype *lty,
        dtype *lx1,
        dtype *lx2,
        dtype *b,
        vector<dtype*> &losses1,
        vector<dtype*> &losses2,
        int count,
        int out_dim,
        int in_dim1,
        int in_dim2,
        bool use_b);
void CalculateDropoutMask(dtype dropout_ratio, int dim, dtype *mask);
void ConcatForward(vector<dtype*> &in_vals, vector<int> &in_dims, vector<dtype*> &vals, int count,
        int in_count,
        int out_dim,
        vector<int> &cols);
void ConcatBackward(vector<dtype*> &in_grads, vector<int> &in_rows, vector<dtype*> &grads,
        int count,
        int in_count,
        int out_row,
        vector<int> &cols);
void ScalarConcatForward(vector<dtype *> &ins, int count, vector<int> &dims,
        int max_dim,
        vector<dtype *> &results);
void ScalarConcatBackward(vector<dtype *> &losses, int count, vector<int> &dims,
        int max_dim,
        vector<dtype *> in_losses);
void LookupForward(vector<int> &xids, dtype *vocabulary,
        int count,
        int dim,
        vector<dtype*> &vals);
void LookupBackward(vector<int> &xids,
        vector<int> &should_backward,
        vector<dtype*> &losses,
        int count,
        int dim,
        dtype *grad,
        bool *indexers);
void LookupBackward(vector<int> &xids, vector<int> &should_backward,
        vector<dtype*> &losses,
        int count,
        int dim,
        dtype *grad);
void ParamRowForward(dtype *param, int row_index, int param_row_count, int count, int dim,
        vector<dtype*> &vals);
void PoolForward(PoolingEnum pooling, vector<dtype*> &in_vals,
        vector<dtype*> &vals,
        int count,
        vector<int> &in_counts,
        int dim,
        int *hit_inputs);
void PoolBackward(vector<dtype*> &losses,
        vector<dtype*> &in_losses,
        vector<int> &in_counts,
        int *hit_inputs,
        int count,
        int dim);
void SumPoolForward(PoolingEnum pooling, vector<dtype*> &in_vals,
        int count,
        int dim,
        vector<int> &in_counts,
        vector<dtype*> &vals);
void SumPoolBackward(PoolingEnum pooling, vector<dtype*> &losses,
        vector<int> &in_counts,
        int count,
        int dim,
        vector<dtype*> &in_losses);
void MatrixConcatForward(vector<dtype*> &in_vals, int count, int in_dim, vector<int> &in_counts,
        vector<dtype*> &vals);
void MatrixConcatBackward(vector<dtype *> &grads, int count, int in_dim, vector<int> &in_counts,
        vector<dtype *> &in_grads);
void TranMatrixMulVectorForward(vector<dtype *> &matrices, vector<dtype *> &vectors, int count,
        vector<int> &cols,
        int row,
        vector<dtype *> &vals);
void TranMatrixMulVectorBackward(vector<dtype *> &grads, vector<dtype *> &matrix_vals,
        vector<dtype *> &vector_vals,
        int count,
        vector<int> &cols,
        int row,
        vector<dtype *> &matrix_grads,
        vector<dtype *> &vector_grads);
void TranMatrixMulMatrixForward(vector<dtype *> &input_a_vals, vector <dtype *> &input_b_vals,
        int count,
        vector<int> &a_cols,
        vector<int> &b_cols,
        int row,
        bool use_lower_triangle_mask,
        vector<dtype *> &vals);
void TranMatrixMulMatrixBackward(vector<dtype *> &grads, vector<dtype *> &a_vals,
        vector<dtype *> &b_vals,
        int count,
        vector<int> &a_cols,
        vector<int> &b_cols,
        int row,
        vector<dtype *> &a_grads,
        vector<dtype *> &b_grads);
void MatrixMulMatrixForward(vector<dtype *> &a, vector<dtype *> &b, int count, vector<int> &ks,
        vector<int> &b_cols,
        int row,
        vector<dtype *> &vals);
void MatrixMulMatrixBackward(vector<dtype *> &grads, vector<dtype *> &a_vals,
        vector<dtype *> &b_vals,
        int count,
        vector<int> &ks,
        vector<int> &b_cols,
        int row,
        vector<dtype *> &a_grads,
        vector<dtype *> &b_grads);
void MatrixAndVectorMultiForward(vector<dtype *> &matrices, vector<dtype *> &vectors, int count,
        int row,
        vector<int> &cols,
        vector<dtype *> &vals);
void MatrixAndVectorMultiBackward(vector<dtype *> &grads, vector<dtype *> &matrices,
        vector<dtype *> &vectors,
        int count,
        int row,
        vector<int> &cols,
        vector<dtype *> &matrix_grads,
        vector<dtype *> &vector_grads);
void PMultiForward(vector<dtype*> &ins1,
        vector<dtype*> &ins2,
        int count,
        int dim,
        vector<dtype*> &vals);
void FullDivForward(vector<dtype*> &numerators, vector<dtype*> &denominators, int count,
        vector<int> &dims,
        vector<dtype*> &results);
void FullDivBackward(vector<dtype*> &grads,
        vector<dtype*> &denominator_vals,
        vector<dtype*> &numerator_vals,
        int count,
        vector<int> &dims,
        vector<dtype*> &numerator_grads,
        vector<dtype*> &denominator_grads);
void SplitForward(vector<dtype*> &inputs, vector<int> &offsets, int count, vector<int> &rows,
        vector<int> &in_rows,
        vector<int> &cols,
        vector<dtype*> &results);
void SplitBackward(vector<dtype*> &grads, vector<int> offsets, int count, vector<int> &rows,
        vector<int> &in_rows,
        vector<int> &cols,
        vector<dtype*> &input_grads);
void SubForward(vector<dtype*> &minuend, vector<dtype*> &subtrahend, int count, vector<int> &dims,
        vector<dtype*> &results);
void SubBackward(vector<dtype*> &losses, int count, vector<int> &dims,
        vector<dtype*> &minuend_losses,
        vector<dtype*> &subtrahend_losses);
void PMultiBackward(vector<dtype*> &losses, vector<dtype*> &in_vals1, vector<dtype*> &in_vals2,
        int count,
        int dim,
        vector<dtype*> &in_losses1,
        vector<dtype*> &in_losses2);
void PAddForward(vector<dtype*> &ins, int count, vector<int> &dims, int max_dim, int in_count,
        vector<dtype*> &vals,
        IntArray &dim_arr);
void PAddBackward(vector<dtype*> &grads, int count, int max_dim, int in_count,
        vector<dtype*> &in_grads,
        IntArray &dim_arr);
dtype CrossEntropyLoss(vector<dtype *> &vals, vector<int> &answers, int count, dtype batchsize,
        vector<dtype *> &losses);
dtype MultiCrossEntropyLoss(vector<dtype*> &vals, vector<vector<int>> &answers, int count, int dim,
        dtype factor,
        vector<dtype*> &losses);
dtype KLCrossEntropyLoss(vector<dtype*> &vals, vector<shared_ptr<vector<dtype>>> &answers,
        int count,
        int dim,
        dtype factor,
        vector<dtype*> &losses);
void MaxScalarForward(vector<dtype*> &inputs, int count, int head_count, vector<int> &head_dims,
        vector<dtype*> &results,
        vector<int> *max_indexes = nullptr);
void MaxScalarBackward(vector<dtype *> &losses, vector<int> &indexes,
        int count,
        vector<dtype*> &input_losses);
void VectorSumForward(vector<dtype *> &inputs, int count, int col, vector<int> &dims,
        vector<dtype*> &results);
void VectorSumBackward(vector<dtype*> &losses, int count, int col, vector<int> &dims,
        vector<dtype*> &input_losses);
void SoftmaxForward(vector<dtype *> &in_vals, int count, vector<int> &rows, vector<int> &cols,
        vector<dtype *> &vals);
void SoftmaxBackward(vector<dtype *> &grads, vector<dtype *> &vals, int count, vector<int> &rows,
        vector<int> &cols,
        vector<dtype *> &in_grads);
void ScaledForward(vector<dtype *> &in_vals, int count, vector<int> &dims, vector<dtype> &factors,
        vector<dtype *> &vals);
void ScaledBackward(vector<dtype *> &grads, int count, vector<int> &dims, vector<dtype> &factors,
        vector<dtype *> &in_grads);
void ScalarToVectorForward(vector<dtype*> &inputs, int count, int input_col, vector<int> &rows,
        vector<dtype*> &results);
void ScalarToVectorBackward(vector<dtype*> &losses, int count, int input_col, vector<int> &rows,
        vector<dtype*> &input_losses);
void BiasForward(vector<dtype*> &in_vals, dtype *bias, int count, int dim,
        vector<dtype *> &vals);
void BiasBackward(vector<dtype *> &losses, int count, int dim, dtype *bias_loss,
        vector<dtype *> input_losses);
void StandardLayerNormForward(dtype **in_vals, int count, int row, int *cols, int max_col,
        dtype **vals,
        dtype *sds);
void StandardLayerNormBackward(vector<dtype *> &grads, int count, int dim, vector<dtype *> &vals,
        dtype *sds,
        vector<dtype *> &in_grads);
void PointwiseLinearForward(vector<dtype *> &in_vals, int count, int dim, dtype *g, dtype *b,
        vector<dtype *> &vals);
void PointwiseLinearBackward(vector<dtype *> &grads, vector<dtype *> &in_vals, dtype *g_vals,
        int count,
        int dim,
        vector<dtype *> &in_grads,
        dtype *g_grads,
        dtype *bias_grads);
vector<int> Predict(vector<dtype*> &vals, int count, int dim);
int Predict(dtype* val, int dim);
void Max(dtype **v, int count, int dim, int *max_indexes, dtype *max_vals);
pair<dtype, vector<int>> SoftMaxLoss(vector<dtype *> &vals_vector,
        int count,
        int dim,
        vector<int> &gold_answers,
        int batchsize,
        vector<dtype *> &losses_vector);
dtype SquareSum(dtype *v, int len);
dtype SquareSum(dtype *v, bool *indexers, int count, int dim);
void Rescale(dtype *v, int len, dtype scale);
void UpdateAdam(dtype *val, dtype *grad, int row, int col, bool is_bias, dtype *aux_mean,
        dtype *aux_square,
        int iter,
        dtype belta1,
        dtype belta2,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdam(dtype *val, dtype *grad, int row, int col, dtype *aux_mean,
        dtype *aux_square,
        bool *indexers,
        int *iters,
        dtype belta1,
        dtype belta2,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdamW(dtype *val, dtype *grad, int row, int col, bool is_bias, dtype *aux_mean,
        dtype *aux_square,
        int iter,
        dtype belta1,
        dtype belta2,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdagrad(dtype *val, dtype *grad, int row, int col,
        dtype *aux_square,
        dtype alpha,
        dtype reg,
        dtype eps);
void UpdateAdagrad(dtype *val, dtype *grad, int row, int col,
        dtype *aux_square,
        bool *indexers,
        dtype alpha,
        dtype reg,
        dtype eps);
void *GraphHostAlloc();
}

#endif

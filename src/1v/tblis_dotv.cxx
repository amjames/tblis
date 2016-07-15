#include "tblis.hpp"

namespace tblis
{

template <typename T>
void tblis_dotv_ref(ThreadCommunicator& comm,
                    bool conj_A, bool conj_B, idx_type n,
                    const T* restrict A, stride_type inc_A,
                    const T* restrict B, stride_type inc_B, T& restrict dot)
{
    dot = T();

    if (n == 0) return;

    T subdot = T();

    idx_type n_min, n_max;
    std::tie(n_min, n_max, std::ignore) = comm.distribute_over_threads(n);

    conj_B = conj_A ^ conj_B;

    if (inc_A == 1)
    {
        if (conj_B)
        {
            for (idx_type i = n_min;i < n_max;i++)
            {
                subdot += A[i]*conj(B[i]);
            }
        }
        else
        {
            for (idx_type i = n_min;i < n_max;i++)
            {
                subdot += A[i]*B[i];
            }
        }
    }
    else
    {
        A += n_min*inc_A;
        B += n_min*inc_B;

        if (conj_B)
        {
            for (idx_type i = n_min;i < n_max;i++)
            {
                subdot += (*A)*conj(*B);
                A += inc_A;
                B += inc_B;
            }
        }
        else
        {
            for (idx_type i = n_min;i < n_max;i++)
            {
                subdot += (*A)*(*B);
                A += inc_A;
                B += inc_B;
            }
        }
    }

    if (conj_A) subdot = conj(subdot);
    comm.reduce(subdot);
    dot = subdot;
}

template <typename T>
void tblis_dotv(const_row_view<T> A, const_row_view<T> B, T& dot)
{
    assert(A.length() == B.length());
    tblis_dotv(false, false,
               A.length(), A.data(), A.stride(),
                           B.data(), B.stride(), dot);
}

template <typename T>
T tblis_dotv(const_row_view<T> A, const_row_view<T> B)
{
    T dot;
    tblis_dotv(A, B, dot);
    return dot;
}

template <typename T>
void tblis_dotv(bool conj_A, bool conj_B, idx_type n,
                const T* A, stride_type inc_A,
                const T* B, stride_type inc_B, T& dot)
{
    parallelize
    (
        [&](ThreadCommunicator& comm)
        {
            tblis_dotv_ref(comm, conj_A, conj_B, n, A, inc_A, B, inc_B, dot);
        }
    );
}

template <typename T>
T tblis_dotv(bool conj_A, bool conj_B, idx_type n,
             const T* A, stride_type inc_A,
             const T* B, stride_type inc_B)
{
    T dot;
    tblis_dotv(conj_A, conj_B, n, A, inc_A, B, inc_B, dot);
    return dot;
}

#define INSTANTIATE_FOR_TYPE(T) \
template void tblis_dotv_ref(ThreadCommunicator& comm, bool conj_A, bool conj_B, idx_type n, const T* A, stride_type inc_A, const T* B, stride_type inc_B, T& dot); \
template void tblis_dotv(bool conj_A, bool conj_B, idx_type n, const T* A, stride_type inc_A, const T* B, stride_type inc_B, T& dot); \
template    T tblis_dotv(bool conj_A, bool conj_B, idx_type n, const T* A, stride_type inc_A, const T* B, stride_type inc_B); \
template void tblis_dotv(const_row_view<T> A, const_row_view<T> B, T& dot); \
template    T tblis_dotv(const_row_view<T> A, const_row_view<T> B);
#include "tblis_instantiate_for_types.hpp"

}
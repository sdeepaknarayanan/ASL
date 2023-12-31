#include "polytope.h"
#include "XoshiroCpp.hpp"
#include <cassert>
#include <algorithm>

#define M m
#define N n

// Specifies how many data types (float or double) fit into one vector register
const size_t N_VEC = 8;
float* bound;
#define align_pad(sz) (((sz)*sizeof(float))/32 + 1)*32
float* A_arr;

static inline
// Takes a vector, and computes the sum of the individual elements of the vector.
const float vec_hadd(const __m256& v) {
    __m128 hiQuad = _mm256_extractf128_ps(v, 1);
    __m128 loQuad = _mm256_castps256_ps128(v);
    __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);

    __m128 loDual = sumQuad;
    __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
    __m128 sumDual = _mm_add_ps(loDual, hiDual);

    __m128 lo = sumDual;
    __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
    __m128 sum = _mm_add_ss(lo, hi);

    return _mm_cvtss_f32(sum); 
}
// Takes a vector, and computes the maximum of the individual elements of the vector.
static inline
const float vec_hmax(const __m256& v) {
    __m128 hiQuad = _mm256_extractf128_ps(v, 1);
    __m128 loQuad = _mm256_castps256_ps128(v);
    __m128 maxQuad = _mm_max_ps(loQuad, hiQuad);

    __m128 loDual = maxQuad;
    __m128 hiDual = _mm_movehl_ps(maxQuad, maxQuad);
    __m128 maxDual = _mm_max_ps(loDual, hiDual);

    __m128 lo = maxDual;
    __m128 hi = _mm_shuffle_ps(maxDual, maxDual, 0x1);
    __m128 max = _mm_max_ss(lo, hi);

    return _mm_cvtss_f32(max); 
}
// Takes a vector, and computes the minimum of the individual elements of the vector.
static inline
const float vec_hmin(const __m256& v) {
    __m128 hiQuad = _mm256_extractf128_ps(v, 1);
    __m128 loQuad = _mm256_castps256_ps128(v);
    __m128 minQuad = _mm_min_ps(loQuad, hiQuad);

    __m128 loDual = minQuad;
    __m128 hiDual = _mm_movehl_ps(minQuad, minQuad);
    __m128 minDual = _mm_min_ps(loDual, hiDual);

    __m128 lo = minDual;
    __m128 hi = _mm_shuffle_ps(minDual, minDual, 0x1);
    __m128 min = _mm_min_ss(lo, hi);

    return _mm_cvtss_f32(min); 
}

static const double unitBallVol(size_t n)
{
  // Added a DP-Like structure, avoid function calls.
  double vol[n + 1];

  vol[0] = 1;
  vol[1] = 2;
  double scale = (2 * M_PI);
  for (size_t i = 2; i < n + 1; i++)
  {
    vol[i] = (scale / i) * vol[i - 2];
  }
  return vol[n];
}

const void polytope::walk(float* norm, float* x, float *Ax, const float* B, const float* A_negrecp, const __m256* Agt,  const __m256* Alt, const float rk, XoshiroCpp::Xoshiro128PlusPlus &rng) const
{
  // Choose coordinate direction
  int dir = (rng() % N);

  float r, max, min, C = 0;

  C = *norm;
  C -= x[dir] * x[dir];

  r = sqrt(rk - C);
  max = r - x[dir], min = -r - x[dir];

  float* A_dir = A_arr + M*dir;
  // const double *A_negrecp_dir = A_negrecp + m * dir;
  // A_negrecp.col(dir);
  const float *B_ptr = B + M * dir, *A_negrecp_dir_ptr = A_negrecp + M*dir, *A_dir_ptr = A_dir;
  float *bound_ptr = bound, *Ax_ptr = Ax;

  __m256 max_all = _mm256_set1_ps(max), min_all = _mm256_set1_ps(min);
  __m256 maxd = max_all, mind = min_all;
  for (size_t i = 0, ii=0; i < (M / N_VEC) * N_VEC; i += N_VEC, ii+=1){
    __m256 A_negrecp_dir_vec = _mm256_loadu_ps(A_negrecp_dir_ptr + i);
    __m256 Ax_vec = _mm256_load_ps(Ax_ptr + i);
    __m256 B_vec = _mm256_loadu_ps(B_ptr + i);
    __m256 Agt_curr = Agt[(M/N_VEC)*dir + ii];
    __m256 Alt_curr = Alt[(M/N_VEC)*dir + ii];
    __m256 bb = _mm256_fmadd_ps(Ax_vec, A_negrecp_dir_vec, B_vec);    
    __m256 bbgt = _mm256_blendv_ps(max_all, bb, Agt_curr);
    maxd = _mm256_min_ps(maxd, bbgt);
    __m256 bblt = _mm256_blendv_ps(min_all, bb, Alt_curr);
    mind = _mm256_max_ps(mind, bblt);
  }
  min = vec_hmax(mind), max = vec_hmin(maxd);
  for (size_t i = (M / N_VEC) * N_VEC ; i < M; i++)
  {
    float aa = A_dir[i];
    float bb = B[M * dir + i] + (Ax[i] * A_negrecp_dir_ptr[i]);
    if (aa > 0 && bb < max)
      max = bb;
    else if (aa < 0 && bb > min)
      min = bb;
  }

  float randval = (XoshiroCpp::FloatFromBits(rng()))*(max - min) + min;
  float t = x[dir] + randval;
  x[dir] = t;
  // assert((min - 0.00001) <= randval && randval <= (max + 0.00001));
  
  __m256 randval_vec = _mm256_set1_ps(randval); 
  for (size_t i = 0; i < (M / N_VEC) * N_VEC; i += N_VEC){
    __m256 Ax_vec = _mm256_load_ps(Ax_ptr + i);
    __m256 A_dir_vec = _mm256_loadu_ps(A_dir_ptr + i);
    __m256 result = _mm256_fmadd_ps(randval_vec, A_dir_vec, Ax_vec);
    _mm256_store_ps(Ax_ptr + i, result);
  }
  // Cleanup code
  for (size_t i = (M / N_VEC) * N_VEC; i < M; i++){
    Ax[i] += A_dir[i] * randval;
  }
  *norm = C + t * t;
}

const double polytope::estimateVol() const
{
  // Re Declaring it Here -- Also no need to initialize Alpha Array
  double res = gamma;
  // Moved this from the bottom, got rid of the alpha array - avoid memory accesses full
  
  long l = ceill(N * log2(2 * N));
  long step_sz = 1600 * l;
  long count = 0;
  float* x = (float *) aligned_alloc(32, align_pad(N));
  memset((void *)x,0,align_pad(N));
  // long t[l + 1];
  float* t = (float *) aligned_alloc(32, align_pad(l+1));
  memset((void *)t,0,align_pad(l+1));
  bound = (float *) aligned_alloc(32, align_pad(M));
  // Move factor computation outside.
  float factor = pow(2.0, -1.0 / N);
  float factor_sq = factor * factor;

  // Initialization of Ax and B
  float* B = (float *) aligned_alloc(32, align_pad(N*M));
  float* Ax = (float *) aligned_alloc(32, align_pad(M));
  memset((void *)Ax,0,align_pad(M));

  // Ax.zeros();
  // Initialize b and A here
  float *b_arr = (float *) aligned_alloc(32, align_pad(M));
  A_arr = (float *) aligned_alloc(32, align_pad(M*N));
  float *A_negrecp = (float *) aligned_alloc(32, align_pad(M*N));

  
  // memcpy(b_arr, b.memptr(), m*sizeof(float)); // TODO: Change
  // memcpy(A_arr, A.memptr(), m*n*sizeof(float)); // TODO: Change
  
  const double* A_memptr = A.memptr();
  for(size_t i=0;i<M*N;i++){
    A_arr[i] = (float)A_memptr[i];
  }

  const double* b_memptr = b.memptr();
  for(size_t i=0;i<M;i++){
    b_arr[i] = (float)b_memptr[i];
  }
  

  // Precomputing the reciprocal of elements in A
  // mat A_negrecp = -1.0 / A;

  size_t i;
  __m256 sign_flip = _mm256_set1_ps(-1);
  for (i = 0; i < (M * N) / N_VEC; i+=N_VEC){
    __m256 A_vec = _mm256_load_ps(A_arr + i);
    __m256 temp = _mm256_div_ps(sign_flip, A_vec);
    _mm256_store_ps(A_negrecp + i, temp);
  }

  // Cleanup Loop
  for (; i < (M*N); i++){
    A_negrecp[i] = -1 / A_arr[i];
  }
  

  size_t cl;
  size_t rw;

  const float *b_ptr = b_arr;
  float *A_rcp_ptr = A_negrecp;
  float *B_ptr = B;

  for (cl = 0; cl < N; cl++) {
    // .colptr(cl);
    __m256 A_rcp_val, b_val, B_to_store;
    for (rw = 0; rw < (M / N_VEC) * N_VEC; rw += N_VEC) {
      b_val = _mm256_load_ps(b_ptr + rw);
      // Arma -- Change b to aligned.
      b_val = _mm256_mul_ps(b_val, sign_flip);
      A_rcp_val = _mm256_loadu_ps(A_rcp_ptr + rw);
      // Arma - A reciprocal - change...
      B_to_store = _mm256_mul_ps(b_val, A_rcp_val);
      _mm256_storeu_ps(B_ptr + rw, B_to_store);
    }

    for (rw = (M / N_VEC) * N_VEC; rw < M; rw++) {
      *(B_ptr + rw) = -b[rw] * A_rcp_ptr[rw];
    }
    B_ptr += M;
    A_rcp_ptr += M;

  }


  // Precomputing radii
  float* r2  = (float *) aligned_alloc(32, align_pad(l+1));
  float pow_precomputed = pow ((float) 2.0, (float) 2.0 / N);
  // Replace Power with Just Multiplication at Each Loop
  r2[0] = 1;
  for (long i = 1; i <= l; ++i)
    r2[i] = pow_precomputed * r2[i - 1];


  // Precomputing vectorization mask
  __m256* Agt = (__m256*) aligned_alloc(sizeof(__m256),(M / N_VEC)*N*sizeof(__m256));
  __m256* Alt = (__m256*) aligned_alloc(sizeof(__m256),(M / N_VEC)*N*sizeof(__m256));
  const __m256 zeros = _mm256_setzero_ps();
  float* A_ptr = A_arr;
  for (size_t i = 0, ii = 0; i < N; i++, ii+=(M / N_VEC))
  {
    // Column Pointer -- Need aligned allocate for A? 
    for (size_t j = 0, jj = 0; j < M / N_VEC; j++, jj+=N_VEC)
    {
      __m256 aa = _mm256_loadu_ps(A_ptr + jj);
      // Change to Aligned Load I guess.
      Agt[ii + j] = _mm256_cmp_ps(aa, zeros, _CMP_GT_OQ);
      Alt[ii + j] = _mm256_cmp_ps(aa, zeros, _CMP_LT_OQ);
    }
    A_ptr += M;
  }


  // Random Generator
  XoshiroCpp::Xoshiro128PlusPlus rng(time(0));

  float norm = 0;

  for (int k = l - 1; k >= 0; k--)
  {
    for (long i = count; i < step_sz; i++)
    {
      walk(&norm, x, Ax, B, A_negrecp, Agt, Alt, r2[k + 1], rng);
      if (norm <= r2[0]) {
        t[0]++;
      } else if (norm <= r2[k]) {
        // Change divide by 2 to multiply by 0.5
        long m = ceill(((double)N) * 0.5 * log2(norm));
        t[m]++;
        // assert(m <= k+1);
      }
    }
    count = 0;
    __m256 count_vec = _mm256_setzero_ps();
    __m256 t_vec;
    size_t i;
    for (i = 0; i < (k / N_VEC)*N_VEC; i+=N_VEC) {
      t_vec = _mm256_load_ps(t + i);
      count_vec = _mm256_add_ps(count_vec, t_vec);
    }
    count = vec_hadd(count_vec);
    // Clean Up Loop
    for(; i <= (size_t)k; i++){
      count += t[i];
    }
    // Alpha has to be >= 1
    count = count > step_sz ? step_sz : count;
    res *= ((double)step_sz) / count;


    float *x_ptr = x;

    __m256 factor_vec = _mm256_set1_ps(factor);
    __m256 x_vec, temp;

    for (i = 0; i < (N / N_VEC) * N_VEC; i += N_VEC) {
      x_vec = _mm256_load_ps(x_ptr + i);
      temp = _mm256_mul_ps(x_vec, factor_vec);
      _mm256_store_ps(x_ptr + i, temp);
    }
    for (; i < N; i++){
      x[i] *= factor;
    }
    norm *= factor_sq;
    __m256 Ax_vec, Ax_fact;
    for (i = 0; i < (M / N_VEC) * N_VEC; i += N_VEC) {
      Ax_vec = _mm256_load_ps(Ax + i);
      Ax_fact = _mm256_mul_ps(Ax_vec, factor_vec);
      _mm256_store_ps(Ax + i, Ax_fact);
    }
    for (; i < M; i++){
      Ax[i] *= factor;
    }
  }

  res *= unitBallVol(N);
  return res;
}

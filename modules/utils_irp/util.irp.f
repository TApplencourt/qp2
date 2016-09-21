integer function align_double(n)
  use constants, only: SIMD_vector
  implicit none
  BEGIN_DOC
  ! Compute 1st dimension such that it is aligned for vectorization.
  END_DOC
  integer                        :: n
  if (mod(n,SIMD_vector/4) /= 0) then
    align_double= n + SIMD_vector/4 - mod(n,SIMD_vector/4)
  else
    align_double= n
  endif
end


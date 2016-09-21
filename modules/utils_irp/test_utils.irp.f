program test_utils
  use constants,  only: dtwo_pi
  implicit none
  integer, external :: align_double
  print *,  'zezfio_initialized : ', zezfio_initialized
  print *,  '2xpi : ', dtwo_pi
  print *,  'align_double(7)', align_double(7)
end

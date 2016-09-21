BEGIN_PROVIDER [ integer, elec_alpha_num  ]
  implicit none
  BEGIN_DOC
  ! Number of alpha electrons ("down")
  END_DOC
  
  PROVIDE zezfio_initialized
  
  logical, external              :: zezfio_has
  integer, external              :: zezfio_get
  integer                        :: rc
  
  if (zezfio_has('electrons.elec_alpha_num')) then
    rc = zezfio_get('electrons.elec_alpha_num', elec_alpha_num)
    if (rc < 0) then
      print *,  rc
      print *,  'Unable to get elec_alpha_num'
    endif
  else
    print *, 'electrons.elec_alpha_num not found in EZFIO file'
    stop 1
  endif
  
END_PROVIDER

BEGIN_PROVIDER [ integer, elec_beta_num  ]
  implicit none
  BEGIN_DOC
  ! Number of beta electrons ("down")
  END_DOC
  
  PROVIDE zezfio_initialized
  
  logical, external              :: zezfio_has
  integer, external              :: zezfio_get
  integer                        :: rc
  
  if (zezfio_has('electrons.elec_beta_num')) then
    rc = zezfio_get('electrons.elec_beta_num', elec_beta_num)
    if (rc < 0) then
      print *,  rc
      print *,  'Unable to get elec_beta_num'
    endif
  else
    print *, 'electrons.elec_beta_num not found in EZFIO file'
    stop 1
  endif
  
END_PROVIDER


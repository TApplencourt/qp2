BEGIN_PROVIDER [ character*(32), nucl_label , (nucl_num) ]
  implicit none
  BEGIN_DOC
! Nuclear labels
  END_DOC

  logical, external              :: zezfio_has
  integer, external              :: zezfio_get
  integer                        :: rc

  PROVIDE zezfio_initialized

  rc = zezfio_get('nuclei.nucl_label', nucl_label)
  if (rc < 0) then
    print *,  rc
    print *,  'Unable to get nuclei.nucl_label'
    stop 1
  endif

END_PROVIDER

BEGIN_PROVIDER [ double precision, nucl_charge , (nucl_num) ]
  implicit none
  BEGIN_DOC
! Nuclear charges
  END_DOC

  logical, external              :: zezfio_has
  integer, external              :: zezfio_get
  integer                        :: rc

  PROVIDE zezfio_initialized

  rc = zezfio_get('nuclei.nucl_charge', nucl_charge)
  if (rc < 0) then
    print *,  rc
    print *,  'Unable to get nuclei.nucl_charge'
    stop 1
  endif


END_PROVIDER


BEGIN_PROVIDER [ integer, nucl_num  ]
  implicit none
  BEGIN_DOC
! Number of nuclei
  END_DOC

  logical, external              :: zezfio_has
  integer, external              :: zezfio_get
  integer                        :: rc
  PROVIDE zezfio_initialized

  rc = zezfio_get('nuclei.nucl_num', nucl_num)
  if (rc < 0) then
      print *,  rc
      print *,  'Unable to get nuclei.nucl_num'
      stop 1
  endif
END_PROVIDER

BEGIN_PROVIDER [ double precision, nucl_coord, (nucl_num,3) ]
  implicit none
  BEGIN_DOC
! Nuclear coordinates as in the input file
  END_DOC

  logical, external              :: zezfio_has
  integer, external              :: zezfio_get
  integer                        :: rc

  PROVIDE zezfio_initialized

  rc = zezfio_get('nuclei.nucl_coord', nucl_coord)
  if (rc < 0) then
    print *,  rc
    print *,  'Unable to get nuclei.nucl_coord'
    stop 1
  endif

END_PROVIDER


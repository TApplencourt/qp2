BEGIN_PROVIDER [ logical, zezfio_initialized ]
 implicit none
 BEGIN_DOC
 ! True when the Zezfio connection has been initialized
 END_DOC
 character(len=128) :: zezfio_address

 call zezfio_initialize('')
 zezfio_initialized = .True.
END_PROVIDER


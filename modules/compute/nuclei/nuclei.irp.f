BEGIN_PROVIDER [ integer, nucl_num_aligned ]
   implicit none
   BEGIN_DOC
   ! Number of nuclei aligned
   END_DOC
   integer, external             :: align_double
   nucl_num_aligned = align_double(nucl_num)
END_PROVIDER

BEGIN_PROVIDER [ double precision, nucl_coord_transp, (3,nucl_num) ]
   implicit none
   BEGIN_DOC
   ! Transposed array of nucl_coord
   END_DOC
   integer                        :: i, k
   nucl_coord_transp = 0.d0

   do i=1,nucl_num
     nucl_coord_transp(1,i) = nucl_coord(i,1)
     nucl_coord_transp(2,i) = nucl_coord(i,2)
     nucl_coord_transp(3,i) = nucl_coord(i,3)
   enddo
END_PROVIDER

BEGIN_PROVIDER [ double precision, nucl_dist, (nucl_num,nucl_num) ]
   implicit none
   BEGIN_DOC
   ! nucl_dist     : Nucleus-nucleus distances
   END_DOC
   
   integer                        :: ie1, ie2, l
   double precision               :: x, y, z
   nucl_dist = 0.d0
   
   do ie2 = 1,nucl_num
     do ie1 = 1,nucl_num
       x = nucl_coord(ie1,1) - nucl_coord(ie2,1)
       y = nucl_coord(ie1,2) - nucl_coord(ie2,2)
       z = nucl_coord(ie1,3) - nucl_coord(ie2,3)
       nucl_dist(ie1,ie2) = dsqrt( x*x + y*y + z*z )
     enddo
   enddo
   
END_PROVIDER
 
BEGIN_PROVIDER [ double precision, positive_charge_centroid, (3)]
  implicit none
  BEGIN_DOC
  ! Centroid of the positive charges
  END_DOC
  integer                        :: l
  positive_charge_centroid = 0.d0
  do l=1, nucl_num
    positive_charge_centroid(1) += nucl_charge(l) * nucl_coord(l,1)
    positive_charge_centroid(2) += nucl_charge(l) * nucl_coord(l,2)
    positive_charge_centroid(3) += nucl_charge(l) * nucl_coord(l,3)
  enddo
END_PROVIDER

BEGIN_PROVIDER [ double precision, nuclear_repulsion ]
  implicit none
  BEGIN_DOC
  ! Nuclear repulsion energy
  END_DOC
  integer                        :: k,l
  double precision               :: Z12, r2, x(3)
  nuclear_repulsion = 0.d0
  do l = 1, nucl_num
    do  k = 1, nucl_num
      if(k == l) then
        cycle
      endif
      Z12 = nucl_charge(k)*nucl_charge(l)
      x(1) = nucl_coord(k,1) - nucl_coord(l,1)
      x(2) = nucl_coord(k,2) - nucl_coord(l,2)
      x(3) = nucl_coord(k,3) - nucl_coord(l,3)
      r2 = x(1)*x(1) + x(2)*x(2) + x(3)*x(3)
      nuclear_repulsion += Z12/dsqrt(r2)
    enddo
  enddo
  nuclear_repulsion *= 0.5d0
  
END_PROVIDER


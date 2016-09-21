BEGIN_PROVIDER [ integer, elec_num ]
  
  implicit none
  BEGIN_DOC
  ! Total number of electrons
  END_DOC
  
  elec_num = elec_alpha_num + elec_beta_num
  
END_PROVIDER

BEGIN_PROVIDER [ integer, elec_num_tab, (2)]
  
  implicit none
  BEGIN_DOC
  ! Numbers of alpha ("up") and beta ("down") electrons
  END_DOC
  
  elec_num_tab(1) = elec_alpha_num
  elec_num_tab(2) = elec_beta_num
  
END_PROVIDER


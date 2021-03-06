safe() { # If the command fails, exit with code -1
    $@ || exit -1
}

safe_depend() {
    msg "$(gettext "Checking dependencies...")"

    if [ -n "${bin_depends+x}" ]; then
        safe check_depend bin
    fi

    if [ -n "${lib_depends+x}" ]; then
        safe check_depend lib
    fi

    if [ -n "${python_depends+x}" ]; then
        safe check_depend python
    fi

    if [ -n "${ocaml_depends+x}" ]; then
        safe check_depend ocaml
    fi
    msg "OK"
}

unsafe_source() {
    msg "$(gettext "Retrieving sources...")"

    if [ -n "${source+x}" ]; then
        fn_source 1 $startdir $srcdir ${source[@]}
    else
        error "$(gettext "No source array in PKGBUILD (%s)")" "${pkgfile_cur}"
        exit 1
    fi    

    if ! [ -z "${irp_depends+x}" ]; then

        #We need to copy the dir beacause
        #if we use symlink, irpf90 will create bin in here symlink...
        #So we need  to filter all the module who have been copy. This is the raison for grep -v /src/
        #Yeah... I know...
        l_pkgbuild=$(find $qp_module -type f -name PKGBUILD | grep -v /src/ | xargs gardener irp_depends | sequoia $pkgname)
        for child_pkgname in ${l_pkgbuild[@]}; do
            #Get the full path of all the dependancy of pkgfile
            local child_pkgfile=$(find $qp_module -type f -name PKGBUILD | grep -v /src/ | xargs egrep -l "pkgname=$child_pkgname")

            if [ -z ${child_pkgfile} ]; then
                error "$(gettext "Cannot find the full path of module in irp_depends")"
                exit 1
            fi

            if [ ! -f $child_pkgfile ]; then
                error "$(gettext "%s is not a valid name for irp_depends")" $pkgname
                exit 1
            else
                startdir_children=$(dirname $child_pkgfile)
                srcdir_parent="${startdir}/src/$child_pkgname"
#                if [ ! -L "$srcdir_parent" ]; then
#                    ln -s -- $startdir_children $srcdir_parent 
#                fi
                if [ ! -d "$srcdir_parent" ]; then
                    cp -a -- $startdir_children $srcdir_parent 
                fi

                
            fi
        done
    fi

}

safe_source() {
    safe unsafe_source $@
}


#Build
safe_build() {
    msg "$(gettext "Building...")"

    if fn_exists build; then
        safe travel ${srcdir} build
    else
        warning "$(gettext "No build function in PKGBUILD (%s)")" "${startdir}"
    fi
}

update_db() {
    pkg_output=$(find ${pkgdir} \( -type l -o -type f \) -printf '%P\n')
    if [ -z "${pkg_output}" ]; then
        error "$(gettext "%s has not packaged any file"). Check %s" "${pkgname}" "${pkgdir}"
        exit 1
    else

        if [ -z ${irp_depend+x} ]; then
            alexandria add immutable ${pkgname} ${pkgfile_rel} ${pkg_output}            
        else
            alexandria add mutable ${pkgname} ${pkgfile_rel} ${pkg_output}
        fi
    fi
}

#Package
unsafe_package() {
    msg "$(gettext "Packaging...")"

    if fn_exists package; then

        mkdir -p  ${pkgdir}/usr/{bin,local,include,share,lib}  

        travel ${srcdir} package
        update_db
        cp -a ${pkgdir}/* ${qp_root}

    else
        error "$(gettext "No package function in PKGBUILD (%s)")" "${startdir}"
        exit 1
    fi
}

safe_package() {
        safe unsafe_package $@
}

unsafe_uninstall(){
    msg "$(gettext "Uninstalling...")"

    if [ -z $(alexandria installed | grep -w "${pkgname}") ]; then
        warning "$(gettext "%s package is not in the database")" "${pkgname}"
    else
        #Get the list of file installed ans remove them
        pushd $qp_root > /dev/null
        alexandria children ${pkgname} | xargs rm --
        popd > /dev/null
        #Remove empty directiy if exist
        travel $qp_root find -mindepth 1 -type d -empty -delete 
        #Now remove it from the db
        alexandria remove ${pkgname}
    fi
}

safe_uninstall(){
        safe unsafe_uninstall $@
}

safe_clean(){
    msg "$(gettext "Removing...")"

    if [ -d "${pkgdir}" ]; then
        safe rm -Rf -- ${pkgdir}
    else
        warning "$(gettext "%s has already been removed")" "${pkgdir}"
    fi

     if [ -d "${srcdir}" ]; then
        safe rm -Rf -- ${srcdir}
    else
        warning "$(gettext "%s has already been removed")" "${srcdir}"
    fi
}

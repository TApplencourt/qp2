safe_depend() {
    msg "$(gettext "Checking dependancy...")"

    if [ -n "${depends+x}" ]; then
        check_depend
    fi

    if [ -n "${python_depends+x}" ]; then
        check_python
    fi

    if [ -n "${ocaml_depends+x}" ]; then
        check_ocaml
    fi    
}

safe_source() {
    msg "$(gettext "Retrieving sources...")"

    if [ -n "${source+x}" ]; then
        fn_source 1 $startdir $srcdir ${source[@]}
    else
        error "$(gettext "No source array in PKGBUILD (%s)")" "${pkgfile_cur}"
        exit 1
    fi    

    if ! [ -z "${irp_depends+x}" ]; then

        l_pkgbuild=$(find $qp_module -type f -name PKGBUILD | xargs gardener irp_depends | sequoia $pkgname)
        for child_pkgname in ${l_pkgbuild[@]}; do

            local child_pkgfile=$(find $qp_module -type f -name PKGBUILD | xargs egrep -l "pkgname=\W$child_pkgname\W")

            if [ ! -f $child_pkgfile ]; then
                error "$(gettext "%s is not a valid name for irp_depend")" $pkgname
                exit 1
            else

                startdir_children=$(dirname $child_pkgfile)
                srcdir_parent="${startdir}/src/$child_pkgname/"

                echo $child_pkgfile
                (
                source $child_pkgfile
                fn_source 1 $startdir_children $srcdir_parent ${source[@]}
                )
            fi
        done
    fi

}

#Build
safe_build() {
    msg "$(gettext "Building...")"

    if fn_exists build; then
        travel ${srcdir} build
    else
        warning "$(gettext "No build function in PKGBUILD (%s)")" "${startdir}"
    fi
}

update_db() {
    pkg_output=$(find ${pkgdir} -type f -printf '%P\n')
    if [ -z "${pkg_output}" ]; then
        error "$(gettext "%s have not packaged any file"). Check %s" "${pkgname}" "${pkgdir}"
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
safe_package() {
    msg "$(gettext "Packaging...")"

    if fn_exists package; then

        mkdir -p  ${pkgdir}/usr/bin ${pkgdir}/usr/include ${pkgdir}/usr/share ${pkgdir}/usr/lib        

        travel ${srcdir} package
        update_db
        cp -a ${pkgdir}/* ${qp_root}

    else
        error "$(gettext "No package function in PKGBUILD (%s)")" "${startdir}"
        exit 1
    fi
}

safe_uninstall(){
    msg "$(gettext "Uninstalling...")"

    if [ -z $(alexandria installed | grep -w "${pkgname}") ]; then
        warning "$(gettext "%s package is not in the database")" "${pkgname}"
    else
        #Get the list of file installed ans remove them
        travel $qp_root/usr/ alexandria children ${pkgname} | xargs rm --
        #Remove empty directiy if exist
        travel $qp_root/usr/ find -mindepth 1 -type d -empty -delete 
        #Now remove it from the db
        alexandria remove ${pkgname}
    fi
}

safe_clean(){
    msg "$(gettext "Removing...")"

    if [ -d "${pkgdir}" ]; then
        rm -Rf -- ${pkgdir}
    else
        warning "$(gettext "%s has already been removed")" "${pkgdir}"
    fi

     if [ -d "${srcdir}" ]; then
        rm -Rf -- ${srcdir}
    else
        warning "$(gettext "%s has already been removed")" "${srcdir}"
    fi
}

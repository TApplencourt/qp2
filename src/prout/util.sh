fn_exists() {
    declare -f -F $1 > /dev/null
    return $?
}

cd_safe() {
    if ! cd "$1"; then
        error "$(gettext "Failed to change to directory %s")" "$1"
        plain "$(gettext "Aborting...")"
        exit 1
    fi
}

travel() {
    local destination=$1; shift

    mkdir -p $destination
    pushd $destination &>/dev/null
    $@
    popd &>/dev/null
}


function join { local d=$1; shift; echo -n "$1"; shift; printf "%s" "${@/#/$d}"; }

irp_include() {
    #In the $srcdir print all folder, and then print put the "-I" between 
    d="$(find $srcdir -maxdepth 1 ! -path $srcdir  -type d ! -path $srcdir/IRPF90_temp ! -path $srcdir/IRPF90_man  -printf "%f\n")"
    if [ -n "${d}" ]; then
        s="$(join ' -I ' $d)"
        echo " -I $s"
    fi
}


#!/bin/bash
#Unofficial Bash Strict Mode
set -o pipefail
IFS=$'\n\t'

#                  _               
#   \  / _  ._ o _|_      _. ._ _  
#    \/ (/_ |  |  | \/   (_| | (_| 
#                   /           _| 
error_usage() {
cat << EOF

PROUT : Package Runtime Optimized User Toolkit

USAGE 
  prout [OPTION] (<PKGBUILD>...)

OPTION 
    -s, --source    : Download source files
    -b, --build     : Compile
    -p, --package   : Install
    -c, --clean     : Remove temporary files
    -u, --unpackage : Uninstall

EOF
exit 1
}

#   ___                      
#    |  ._ _  ._   _  ._ _|_ 
#   _|_ | | | |_) (_) |   |_ 
#             |              
if [ -n "${qp_root+x}" ]; then
    source $qp_root/src/prout/safe_fct.sh
    source $qp_root/src/prout/util.sh
    source $qp_root/src/prout/source.sh
    source $qp_root/src/prout/path.sh
    source $qp_root/src/prout/dependency.sh
    source $qp_root/src/prout/message.sh
else
    echo "Please, source qp.rc"
    exit 1
fi

colorize

declare -a FUNCTION

# extract options and their arguments into variables.
while getopts "sbpuc --long source,build,package,uninstall,clean" opt; do
    case "$opt" in
        s|--source)
            FUNCTION+=("safe_source")
            ;;
        b|--build)
            FUNCTION+=("safe_depend")
            FUNCTION+=("safe_build")
            ;;
        p|--package)
            FUNCTION+=("safe_package")
            ;;
        c|--clean)
            FUNCTION+=("safe_clean")
            ;;
        u|--uninstall)
            FUNCTION+=("safe_uninstall")
            ;;
        *)   
            error_usage
            ;;

    esac
done

if [[ -z ${FUNCTION+x} ]]; then
    error_usage
    exit 1
fi


shift $((OPTIND-1))

if [[ -z $@ ]]; then
    error_usage
    exit 1
fi

for pkgfile in $@; do

    if [ ! -f $pkgfile ]; then
        error "PKGBUILD not found"
        exit 1
    else
        pkgfile=$(fn_abs "$pkgfile")
    fi

    pkgfile_rel=$(fn_relpath $pkgfile $qp_root)
    startdir=$(dirname $pkgfile)
    srcdir="${startdir}/src"
    pkgdir="${startdir}/pkg"
    
    cd $startdir
    source $pkgfile    
    plain "Handle $pkgname"

    for f in "${FUNCTION[@]}"; do
        $f;
    done
    plain ""

done

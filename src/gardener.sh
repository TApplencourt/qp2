#!/usr/bin/env bash
#Unofficial Bash Strict Mode
set -eo pipefail
IFS=$'\n\t'

#                  _               
#   \  / _  ._ o _|_      _. ._ _  
#    \/ (/_ |  |  | \/   (_| | (_| 
#                   /           _| 
error_usage() {
cat << EOF
Print the dependency graph of all the <PKGBUILD>... relative to the <attribute> (graphiz format)

Usage:
  gardener <attribute> <PKGBUILD>...

Option:
    <attribute>: The name of the attribute (source, irp_source, ...)
    <PKGBUILD> The list of PKGBUILD
EOF
exit 1
}


if [[ $# -lt 2 ]]; then
  error_usage
  exit 1
fi

#                 
# \    / _  ._ |  
#  \/\/ (_) |  |< 
#                 
attribute=$1
shift 

echo "digraph $attribute {"

for PKGBUILD  in $@; do
    (
    source $PKGBUILD
    eval var=\( \${${attribute}[@]} \)

    if [ -n "${var+x}" ]; then
        eval var=\( \${${attribute}[@]} \)
        for s in ${var[@]}; do
            echo "   $pkgname -> $s ;"
        done
    fi
    )
done

echo "}"
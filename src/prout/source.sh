# function Extract for common file formats in a directory ($2)
function extract {
 if [ -z "$1" ]; then
    # display usage if no parameters given
    echo "Usage: extract <path/file_name>.<zip|rar|bz2|gz|tar|tbz2|tgz|Z|7z|xz|ex|tar.bz2|tar.gz|tar.xz>"
 else
    if [ -f "$1" ] ; then
        case "$1" in
          *.tar.bz2)   tar xvjf  "$1"  -C $2 ;;
          *.tar.gz)    tar xvzf  "$1"  -C $2 ;;
          *.tar.xz)    tar xvJf  "$1"  -C $2 ;;
          *.tar)       tar xvf   "$1"  -C $2 ;;
          *.tbz2)      tar xvjf  "$1"  -C $2 ;;
          *.tgz)       tar xvzf  "$1"  -C $2 ;;
          *.zip)       unzip     "$1"  -d $2 ;;
          *)           echo "extract: '$1' - unknown archive method" ;;
        esac
    else
        echo "'$1' - file does not exist"
    fi
fi
}

function extract_strip {
    local zip=$1
    local dest=${2:-.}
    local temp

    temp=$(mktemp -d) && extract "$zip" "$temp" && mkdir -p "$dest" &&
    
    shopt -s dotglob && local f=("$temp"/*) &&
    if (( ${#f[@]} == 1 )) && [[ -d "${f[0]}" ]] ; then
        mv "$temp"/*/* "$dest"
    else
        mv "$temp"/* "$dest"
    fi && rmdir "$temp"/* "$temp"
}

#fn_source <stardir> <srcdir> <source>...
#Dowload all the source!
#If they are .tar.gz, they will be dowloaded
#If they are local file they will be linked
fn_source() {

    local no_extract=$1 ; shift
    local temp_startdir=$1; shift
    local temp_srcdir=$1 ; shift

    temp_source=("${@}")

    mkdir -p ${temp_srcdir}

    for s in "${temp_source[@]}"; do

        IFS=' ' read -a array <<< "$(dowser $s)"

        if [[ ${array[0]} == "url" ]]; then
            local url=${array[1]}
            local target_file=${temp_srcdir}/${array[2]}

            if [ ! -f "${target_file}" ]; then
                wget $url -O ${target_file}
            else
                 warning "$(gettext "%s file already exists (%s)")" "${array[2]}" "$(fn_abs ${target_file})" 
            fi
        elif [[ ${array[0]} == "local" ]]; then

            local url=${temp_startdir}/${array[1]}
            local target_file=${temp_srcdir}/${array[2]}

            if [ ! -f "${target_file}" ]; then          
                rel_symlink "${url}" "${target_file}"
            else
                warning "$(gettext "%s file already exists (%s)")" "${s}" "${target_file})" 
            fi
        fi

        if [[ ${#array[@]} -ge 4 && ${array[3]} == "directory" ]] && [ "${no_extract}" -eq 1 ]; then
            local directory_name=${temp_srcdir}/${array[4]}

            if [ ! -d "${directory_name}" ]; then
                extract_strip ${target_file} ${directory_name}
            else
                warning "$(gettext "%s directory already exists (%s)")" "${array[4]}" "${directory_name})" 
            fi
        fi

    done
}

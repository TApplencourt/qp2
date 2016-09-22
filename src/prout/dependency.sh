check_one_depend(){
  TYPE=$1
  depend=$2
  case $TYPE in
    bin)
      command -v $depend &>/dev/null 
      exit_code=$?
      ;;

    lib)
        tempfile=$(mktemp --suffix=.c)
        if ! $(command -v gcc >/dev/null 2>&1); then
            error "$(gettext "Please install gcc")"
            exit 1
        fi
        echo "int main() {}" > $tempfile
        gcc -l${depend/#lib/} ${tempfile} -o /dev/null &>/dev/null
        exit_code=$?
        rm -- ${tempfile}
        ;;
    ocaml)
        if ! $(command -v opam >/dev/null 2>&1); then
            error "$(gettext "Please install opam.")"
            exit 1
        fi
        opam list -i "$i" &> /dev/null
        exit_code=$?
        ;;

    python)
        python -c "import $depend" &> /dev/null
        exit_code=$?
        ;;

    *)
        error "$(gettext "%s : Unknown dependency type.")" $TYPE
        exit 1
        ;;
  esac
  return $exit_code
}

check_depend() {
    # Verify if all the modules present in the depends array have been installed
    TYPE=$1
    declare -a depends
    case $TYPE in
      bin)
        depends=(${bin_depends[@]})
        ;;
      lib)
        depends=(${lib_depends[@]})
        ;;
      ocaml)
        depends=(${ocaml_depends[@]})
        ;;
      python)
        depends=(${list_depends[@]})
        ;;
      *)
        error "$(gettext "%s : Unknown dependency type.")" $TYPE
        exit 1
        ;;
    esac

    #All the packages already installed
    installed=$(alexandria installed)

    #Get the packages that are not installed
    diff=()
    for i in "${depends[@]}"; do
        skip=
        for j in $installed; do
            [[ $i == $j ]] && { skip=1; break; }
        done
        [[ -n $skip ]] || diff+=("$i")
    done

    #Check if the needed packages are already installed by the system
    if [ ${#diff[@]} -ne 0 ]; then

        not_present=()

        warning "$(gettext "%s are not in the db ")" "${diff[@]}"
        msg2 "$(gettext "Check if it is already installed in the system..." )"

        for depend in "${diff[@]}"; do
            if $(check_one_depend $TYPE $depend); then
                alexandria add external $depend external
            else
                not_present+=("$depend")
            fi
        done

        if [ ${#not_present[@]} -ne 0 ]; then
            error "$(gettext "($TYPE) Please install : %s" )" "${not_present[@]}"
            exit 1
        else
            msg2 "$(gettext "Sucess" )"
        fi

    fi
}



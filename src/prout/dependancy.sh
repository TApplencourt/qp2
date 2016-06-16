check_depend() {
    # Verify is all the module prensend in the depends array have been installed


    #All the package already installed
    installed=$(alexandria installed)

    #Get the package not installed
    diff=()
    for i in "${depends[@]}"; do
        skip=
        for j in $installed; do
            [[ $i == $j ]] && { skip=1; break; }
        done
        [[ -n $skip ]] || diff+=("$i")
    done

    #Check if the package needed are already installed by the sytem
    if [ ${#diff[@]} -ne 0 ]; then

        not_present=()
        for depend in "${sym_diff[@]}"; do
            warning "$(gettext "%s is not in the db ")" "$depend"
            msg2 "$(gettext "Check if it is already installed in the system" )"
            if $(command -v $depend >/dev/null 2>&1); then
                location=$(command -v $depend)
                alexandria add external $depend $location
            else
                not_present+=("$depend")
            fi
        done

        if [ ${#not_present[@]} -ne 0 ]; then
            error "$(gettext "Please install %s" )" "${not_present[@]}"
            exit 1
        fi

    fi
}
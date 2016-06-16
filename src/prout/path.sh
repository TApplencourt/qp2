fn_relpath() {
 local absolute=$1
 local current=$2
 perl -MFile::Spec -e 'print File::Spec->abs2rel(@ARGV)' "$absolute" "$current"
}

fn_abs() {
    local path="$1"
    path=$(readlink -e ${path})

    if [ -z ${path} ]; then
        error "$1 not found"
        exit 1
    else
    	echo $path
    fi

}

rel_symlink() {
	local target=$1
	local link_name=$2	
	relpath=$(fn_relpath $target $(dirname $link_name))
	ln -fs $relpath $link_name
}
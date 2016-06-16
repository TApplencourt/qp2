# Rule for developper
1. File installed need to follow our Filesystem Hierarchy Standard (greatly inspired by the [seccondary hierary of Unix](http://www.pathname.com/fhs/pub/fhs-2.3.html#THEUSRHIERARCHY)):
    - `$QP_ROOT/usr/bin`: This is the primary directory of executable commands on the system.
    - `$QP_ROOT/usr/include`: This is where all of the system's general-use include files for the C programming language should be placed.
    - `$QP_ROOT/usr/lib` includes object files, libraries, and internal binaries that are not intended to be executed directly by users or shell scripts. 
    - `$QP_ROOT/usr/share`is for all read-only architecture independent data files
2. The Python and Ocaml package are not handle by the Quantum Package package manager. Please instal it manualay (with [pip](https://pip.pypa.io/en/stable/installing/) or [opam](https://opam.ocaml.org/) for exemple).

# PKGBUILD
## Options and Directives

The following is a list of standard options and directives available for use in a PKGBUILD. These are all understood and interpreted by `prout`, and most of them will be directly transferred to the built package. 

If you need to create any custom variables for use in your build process, it is recommended to prefix their name with an _ (underscore). This will prevent any possible name clashes with internal `prout` variables.

##### pkgname
The name of the package. Valid characters are alphanumerics, and any of the following characters: “@ . _ + -”. Additionally, names are not allowed to start with hyphens or dots.

##### source (array)
An source files required to build the package. To simplify the maintenance of PKGBUILDs, use the $pkgname variables when specifying the download location, if possible. Compressed files will be extracted automatically in $pkgname. This folder will be created if not existing. 

## Dependancy

All these dependancy will be checked whenever you will `build` the package. 

##### depends (optional/array)
An array of packages this package depends on to run. Entries in this list should be surrounded with single quotes and contain at least the package name. If the package where not in the database, `prout` will check is the package is avalaible via `command -v`.

##### irp_depends (optional/array)
An array of irp_packages this irp_package depends on to run. Entries in this list should be surrounded with single quotes and contain at least the package name.  `prout` will create symlink in the `srcdir` relative the these packages.

##### python_depends (optional/array)
An array of irp_packages this irp_package depends on to run. Entries in this list should be surrounded with single quotes and contain at least the package name. `prout` will check is the package is avalaible via `python -c "import ${package}"`.

##### ocaml_depends (optional/array)
An array of irp_packages this irp_package depends on to run. Entries in this list should be surrounded with single quotes and contain at least the package name.  `prout` will check is the package is avalaible via `opam list`.

## Packaging Functions

##### package() Function
The package() function is used to install files into the directory that will become the root directory of the built package and is run after all the optional functions listed below. All other functions will be run as the user calling `prout`.

##### build() Function
The optional build() function is use to compile and/or adjust the source files in preparation to be installed by the package() function. This is directly sourced and executed by `makepkg`, so anything that Bash or the system has available is available for use here. Be sure any exotic commands used are covered by the depends array.

If you create any variables of your own in the build() function, it is recommended to use the Bash `local` keyword to scope the variable to inside the build() function.

## Variable
All of the above variables such as `$pkgname` are available for use in the packaging functions. In addition, `prout` defines the following variables:

##### qp_root
Path of the quantum_package folder

##### srcdir
This contains the directory where makepkg extracts, or copies, all source files.
All of the packaging functions defined above are run starting inside $srcdir

##### pkgdir
This contains the directory where makepkg bundles the installed package. This directory will become the root directory of your built package. This variable should only be used in the package() function.

##### startdir
This contains the absolute path to the directory where the PKGBUILD is located, which is usually the output of $(pwd) when makepkg is started. Use of this variable is deprecated and strongly discouraged.

## Example

The following is an example PKGBUILD for the patch package. For more examples, look through the `module` folder. You can be greadly inspired by the Arch Build System (ABS) tree.

```bash
# Maintainer: Joe User <joe.user@example.com>

pkgname=patch
pkgver=2.7.1
pkgdesc="A utility to apply patch files to original sources"
url="https://www.gnu.org/software/patch/patch.html"
depends=('glibc')
source=("ftp://ftp.gnu.org/gnu/$pkgname/$pkgname-$pkgver.tar.xz")

build() {
        cd "$srcdir/$pkgname-$pkgver"
        ./configure --prefix=/usr
        make
}

package() {
        cd "$srcdir/$pkgname-$pkgver"
        make DESTDIR="$pkgdir/" install
}
```

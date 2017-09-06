#!/bin/bash

plugins=$3

if ! [ $plugins ]; then
    plugins="platforms xcbglintegrations mediaservice imageformats"
fi

function useage()
{
    cat << EOU
Useage: bash $0 <path to the binary> <path to copy the dependencies> <list of qt plugins>
EOU
exit 1
}

function update_deps
{
    update_deps_ex $1
    [[ -e $lib_path ]] || exit -1
    depsf=$(find $lib_path -name "*XcbQpa.so.?" -or -name "*MultimediaQuick_p.so.?" -or -name "*MultimediaWidgets.so.?" -or -name "*OpenGL.so.?" -or -name "*qgsttools_p.so.?")
    for dep in $depsf
    do
        if echo $deps | grep -w $dep > /dev/null; then
            continue
        fi
        #echo "EXTRA: $dep"
        deps="$deps $dep"
        update_deps_ex $dep
    done
}

function update_deps_ex
{
    depsf=$(ldd $1 | grep 'Qt' | awk 'BEGIN{ORS=" "}$1 ~/^\//{print $1}$3~/^\//{print $3}' | sed 's/,$/\n/')
    for dep in $depsf
    do
        if echo $deps | grep -w $dep > /dev/null; then
            continue
        fi
        #echo "FOUND: $dep"
        deps="$deps $dep"
        if ! [ $lib_path ]; then
            lib_path=$(dirname $dep)
            echo "LIB_PATH: $lib_path"
        fi
        update_deps_ex $dep
    done
}

function do_cp
{
    src="$1"
    dest="$2"
    [[ -e "$dest/$(basename $src)" ]] && return
    echo "### LIB Copy $src" # : $dest/$(basename $src)"
    mkdir -p "$dest"
    cp -ru $3 "$src" "$dest"
}

#Validate the inputs
[[ $# < 2 ]] && useage

#Check if the paths are vaild
[[ ! -e $1 ]] && echo "Not a vaild input $1" && exit 1
[[ -d $2 ]] || echo "No such directory $2 creating..."&& mkdir -p "$2"

#Get the library dependencies
echo "Collecting the shared library dependencies for $1..."
#deps=$(ldd $1 | grep 'Qt' | awk 'BEGIN{ORS=" "}$1 ~/^\//{print $1}$3~/^\//{print $3}' | sed 's/,$/\n/')
update_deps $1
#echo "Copying the dependencies to $2"
#echo

#Copy the deps
for dep in $deps
do
    #echo "Copying $dep"
    #cp -u "$dep" "$2"
    do_cp "$dep" "$2" -L
    file_path=$(dirname $dep)
    file_name=$(basename $dep)
    file_base=${file_name%.so*}
    #find "$file_path" -name "${file_base}.so*" -exec cp -P {} "$2" \;
    #ldd "$dep" | grep 'Qt' #| grep 'Xcb'
done

#copy plugins
for dep in $plugins
do
    do_cp "$lib_path/../plugins/$dep" "$2/../plugins"
#    dep=
#    echo "Copying $dep"
#    mkdir -p "$2/../plugins"
#    cp -ru "$dep" "$2/../plugins"
done

#copy QML
#dep="$lib_path/../qml"
#echo "Copying $dep"
#mkdir -p "$2/../qml"
#cp -ru "$dep" "$2/.."
do_cp "$lib_path/../qml" "$2/.."



#echo
#echo "Done!"
#echo

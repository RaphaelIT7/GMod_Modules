#! /bin/bash

dir="_project_linux"
script=$(readlink -f "$0")
homedir=$(dirname "$script")
compileconfig=""
realmcolor=""
background="\033[40m"

if test -e "$homedir/working"; then
	printf "Skipping...\n\n"
	exit 0
fi
touch "$homedir/working"

printf "$background"

if ! test -d "$homedir/$dir"; then
	printf "Skipping $dir...\n\n"
	continue
fi

if test "$dir" = "$1"; then
	printf "Skipping $dir...\n\n"
	continue
fi

compileconfig="release32"
realmcolor="\033[36m" # Blue

printf "$background\033[1m${realmcolor}Compiling $dir...\033[0m$background\n\n"
printf "$realmcolor"

cd "$homedir/$dir"
rm -r -f "$homedir/$dir/obj/"

make -j 2 -e config="$compileconfig" homedir="$homedir"

if test $? -ge 1; then
	printf "\033[1m\033[31mCoudln't compile $dir!\033[0m$background\n\n\n\n"
	continue
fi

if test $? -eq 0; then
	printf "\033[1m\033[32mCompiling $dir done!\033[0m$background\n\n\n\n"
fi

rm "$homedir/working"

printf "\033[0m\n"
exit 0

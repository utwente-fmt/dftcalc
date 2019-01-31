#!/bin/sh

# Get the aliases and functions
export HOME=/home/dftcalc
if [ -f $HOME/.bashrc ]; then
        . $HOME/.bashrc
fi

d=`echo $0 |sed 's/[^/]*$//' `
# echo $d
. $d/settings


# var TOOL is set by settings
# env vars PUPTOLROOT and SESSION are set by puptol


PLOTSTYLE=linespoints
if [ "X$OMITPOINTS" = "Xomitpoints" ]
then
  PLOTSTYLE=lines
fi


# echo arg: "$@" 1>&2
#for arg in "$@"
#do
#    echo "arg: ($arg)" 1>&2
#done

mkdir $TOOLTMPDIR
# next cd is necessary because dft2bcg wants to write a temporary file in current working directory
cd $TOOLTMPDIR


    lastmttf=""
    for item in "$@"
    do
      a="data/session/$SESSION/$TOOL/uploads/$item"
      #a=`echo $item | sed -e 's/^own/data/g' -e 's/datasets/uploads/g' `
      #echo testing "($PUPTOLROOT/$a/mttf)" 1>&2
      if [ -r "$PUPTOLROOT/$a/mttf" ]
      then
	lastmttf=$PUPTOLROOT/$a/mttf
      fi
    done

    itemsLost=""
    sepLost=""
    files=""
    touch $filesToTar
    dirToTarFrom="$PUPTOLROOT/data/session/$SESSION/$TOOL/uploads"
    for item in "$@"
    do
      a="data/session/$SESSION/$TOOL/uploads/$item"
      #a=`echo $item | sed -e 's/^own/data/g' -e 's/datasets/uploads/g' `
      # echo testing: $PUPTOLROOT/$a/data 1>&2
     if [ -r "$PUPTOLROOT/$a/data" ]
     then
        echo "$item" >> $filesToTar
        files="$files $PUPTOLROOT/$a"
    else
        itemsLost="${itemsLost}${sepLost}${item}"
        sepLost=", "
	if [ -r "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$item" ]
	then
	    mv "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$item" "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$item.hidden"
	fi
    fi
    done

    if [ "X$itemsLost" !=  "X" ]
    then
	echo "One or more datasets ($itemsLost) could not be found. Maybe your session expired?" 1>&2
	echo "Rereading the list of datasets." 1>&2
	echo "refresh" 1>&$PUPTOL_DFTCALC_datasets_refresh_FD
	exit 0
    elif [ "X$files" = "X" ]
    then
	echo "Nothing to download" 1>&2
	exit 0
    fi

    cd $dirToTarFrom
    if tar cf - -T $filesToTar | gzip >  "$sesfilepfx/download$$.tar.gz"
    then
	# TODO: pass the tgz file to the browser, as result, with the right mime-type
	# configure a separate file descriptor for it in puptol config?
	echo "$sesurlpfx/download$$.tar.gz?Content-Disposition=selected-datasets.tar.gz   |-=-|  the gzip-ed tar-file containing your selected data set(s)"  1>&$PUPTOL_DOWNLOAD_FD
	# echo "Please save your selected data sets by saving <a href="'"'"$sesurlpfx/download$$.tar.gz"'"'">gzip-ed tar file containing datasets</a>"
    else
        echo "internal error: could not generate gzipped tar file" 1>&2
    fi


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


doplot () {
    format=$1
    shift
    output=$1
    shift

    touch  $sesfilepfx/$plotdata
    touch  $sesfilepfx/$plotdata.2

    echo 'set xlabel "Time Units"' > $plotcmd
    echo 'set yrange [0:1]' >> $plotcmd
    echo 'set ylabel "Unreliability"'  >> $plotcmd
    echo 'set terminal '$format  >> $plotcmd
    echo 'plot ' \\ >> $plotcmd

    comma=""

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
    for item in "$@"
    do
      a="data/session/$SESSION/$TOOL/uploads/$item"
      #a=`echo $item | sed -e 's/^own/data/g' -e 's/datasets/uploads/g' `
      # echo testing: $PUPTOLROOT/$a/data 1>&2
     if [ -r "$PUPTOLROOT/$a/data" ]
     then
        #title=`basename $item` 
        title=`cat $PUPTOLROOT/$a/data_title` 
        data=$PUPTOLROOT/$a/data
        echo $comma' "'$data'" with '$PLOTSTYLE' title "'$title'"' \\ >> $plotcmd
        comma=", "
    else
        itemsLost="${itemsLost}${sepLost}${item}"
        sepLost=", "
	if [ -r "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$item" ]
	then
	    mv "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$item" "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$item.hidden"
	fi
    fi
    done

    mttfT=""
    for item in "$@"
    do
      a="data/session/$SESSION/$TOOL/uploads/$item"
      #a=`echo $item | sed -e 's/^own/data/g' -e 's/datasets/uploads/g' `
      if [ -r "$PUPTOLROOT/$a/mttf" ]
      then
        mttfD=$PUPTOLROOT/$a/mttf
	if [ "X$mttfD" = "X$lastmttf" ]
	then
            mttfT=MTTF
        fi
        echo $comma' "'$mttfD'" using  1:2:(0):1 with xerror ps 0 lt 0 lw 1.5 lc rgb "#010101" title ""' \\ >>$plotcmd
        echo ', "" using 1:2                     with impulses    lt 0 lw 1.5 lc rgb "#010101" title ""' \\ >>$plotcmd
        echo ', "" using 1:2                                      pt 3 lw 2   lc rgb "#0000ff" title "'$mttfT'"' \\ >> $plotcmd
        comma=", "
      fi
    done

#    if [ "X$comma" = "X"  -a "$#" -gt 0 ]
    if [ "X$itemsLost" !=  "X" ]
    then
	echo "One or more datasets ($itemsLost) could not be found. Maybe your session expired?" 1>&2
	echo "Rereading the list of datasets." 1>&2
	echo "refresh" 1>&$PUPTOL_DFTCALC_datasets_refresh_FD
	exit 0
    elif [ "X$comma" = "X" ]
    then
	echo "Nothing to plot" 1>&2
	exit 0
    fi

    echo "" >> $plotcmd
    if [ "X$format" = "Xsvg" ]
    then
        $gnuplot < $plotcmd  | sed "s/stroke='rgb([ ]*1,[ ]*1,[ ]*1)'/stroke-dasharray='5,5' stroke='rgb( 1, 1, 1)'/g" >  $sesfilepfx/$svg
        return $?
    else
        $gnuplot < $plotcmd  >  $output
        return $?
    fi
}

    if doplot svg $sesfilepfx/$svg "$@" && \
       doplot png $sesfilepfx/$png "$@"
    then
       echo "$sesurlpfx/$svg |-=-| Plot showing unreliability of given DFT; click for an SVG version of the image |-=-| $sesurlpfx/$svg" 1>&$PUPTOL_IMG_FD
    else
        echo "internal error: could not generate svg via gnuplot" 1>&2
    fi


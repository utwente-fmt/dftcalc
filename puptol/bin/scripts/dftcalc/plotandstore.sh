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

                if test ! -d "$PUPTOLROOT/data/session/$SESSION"
                then
                        echo "internal error: cannot store plot data set" 1>&2
                        exit
                fi
                if test ! -d "$PUPTOLROOT/data/session/$SESSION/$TOOL"
                then
                        mkdir "$PUPTOLROOT/data/session/$SESSION/$TOOL"
                fi
                if test ! -d "$PUPTOLROOT/data/session/$SESSION/$TOOL/uploads"
                then
                        mkdir "$PUPTOLROOT/data/session/$SESSION/$TOOL/uploads"
                fi
                if test ! -d "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets"
                then
                        mkdir "$PUPTOLROOT/data/session/$SESSION/$TOOL/datasets"
                fi

                cd "$PUPTOLROOT/data/session/$SESSION/$TOOL/uploads"
                count=`ls -1 |wc -l`
                count=`echo $count`


		mkdir $count
		uploaddir=$PUPTOLROOT/data/session/$SESSION/$TOOL/uploads/$count

		name="dataset $count"


# function to clean away data directory
cleanup () {
    rm -rf $uploaddir
    if [ "X$canonical" != "X" ]
    then
        # make sure group members can clean up
        chmod -R g+w .
    fi
}


#!/bin/sh


sed 's///g'  > $tmpdft

#echo arguments: "$@" 1>&2
#for a in "$@"
#do
#	echo arg: "$a" 1>&2
#done
#echo env: 1>&2
#env 1>&2

testVersionValue VERSION 'version'
testRealValue ERRORBOUND 'error bound'
testIdentifierListValue EVIDENCE 'evidence'
testVerbosityValue VERBOSE 'verbosity'
testColorValue COLOR 'color'
testDataSetNameValue DATASETNAME 'data-set name'
if errorsfound
then
	cleanup
	exit
fi

ERRORB="-E 0.0001"
if [ "X$ERRORBOUND" != "X" ]
then
  ERRORB="-E $ERRORBOUND"
fi

EVDNCE=""
EV=`echo "$EVIDENCE" | sed -e 's/^[ 	]*//g' -e 's/[ 	]*$//g' -e's/[ 	][ 	]*/,/g' -e 's/,,*/,/g' `
if [ "X$EV" != "X" ]
then
  EVDNCE="-e $EV"
fi

if [ "X$MINMAXTIME" = "X" ]
then
  MINMAXTIME=$MINMAXPROB
fi

PLOTSTYLE=linespoints
if [ "X$OMITPOINTS" = "Xomitpoints" ]
then
  PLOTSTYLE=lines
fi

DSNAME=$name
LONGDSNAME=$name
DSN=`echo "$DATASETNAME" | sed -e 's/^[ 	]*//g' -e 's/[ 	]*$//g' `
if [ "X$DSN" != "X" ]
then
    LONGDSNAME="$name: $DSN"
    DSNAME="$DSN"
fi

# echo $coral -f $tmpdft "$@" 1>&2

mkdir $TOOLTMPDIR

# next cd is necessary because dft2bcg wants to write a temporary file in current working directory
cd $TOOLTMPDIR

cp $PUPTOLROOT/data/shared/dftcalc/erl2_2.bcg .
cp $tmpdft .

if [ "X$VERBOSE" != "X--verbose=-1" ]; then
	echo Using dftcalc version: $VERSION 1>&2
	echo "which corresponds to... (asking dftcalc for version info)" 1>&2
	echo "" 1>&2
	
	echo command: dftcalc -R --no-color $COLOR -q $VERBOSE --version 1>&2
	echo "" 1>&2
	dftcalc -R --no-color $COLOR -q $VERBOSE --version 1>&2
	echo "" 1>&2
fi


if [ "X$VERBOSE" != "X--verbose=-1" ]; then
	echo command: "dft2lntc --no-color $COLOR -q $VERBOSE $EVDNCE -t $tmpcandft < $tmpdft" 1>&2
	echo "" 1>&2
fi
if ! dft2lntc --no-color $COLOR -q $VERBOSE $EVDNCE -t $tmpcandft < $tmpdft 1>&2
then
        echo "could not canonicalize DFT" 1>&2
        exit
else
	echo "Canonicalizing by computing SHA-256 of (canonical) DFT and verbose dftcalc version info" 1>&2
	dftcalc -R --no-color $COLOR -q $VERBOSE --version >  $tmptxt 2>&1
	cat  $tmpdft >> $tmptxt
	canonical=`sha256sum  $tmptxt | awk '{print $1}' `
	canonical=$canonical-$VERSION
        if [ "X$VERBOSE" != "X--verbose=-1" ]; then
                echo "Canonical name: $canonical" 1>&2
		echo "" 1>&2
        fi
fi

# Now:
#  - create (if necessary) $sessiondir/dftcalc/canonical/$canonical
#  - store (if necessary)  $tmpdftcan in that directory
#  - cd to that directory?
#  - use that name as the dft name
#  - use -R flag with dftcalc

mkdir -p $sesfilepfx/dftcalc/canonical/$canonical
export TOOLTMPDIR=$sesfilepfx/dftcalc/canonical/$canonical

cd $TOOLTMPDIR

if ! cmp -s $PUPTOLROOT/data/shared/dftcalc/erl2_2.bcg erl2_2.bcg
then
        cp $PUPTOLROOT/data/shared/dftcalc/erl2_2.bcg .
fi
if ! cmp -s $tmpcandft $canonical.dft
then
        cp $tmpcandft $canonical.dft
fi

dft=$canonical.dft


NOPLOT=0

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

    for item in "$@"
    do
        title=`echo $item | cut -d ':' -f 1`
        data=`echo $item | cut -d ':' -f 2`
        mttfT=`echo $item | cut -d ':' -f 3`
        mttfD=`echo $item | cut -d ':' -f 4`
        echo $comma' "'$data'" with '$PLOTSTYLE' title "'$title'"' \\ >> $plotcmd
	comma=", "
	if [ "X$mttf" != "X" ]
	then
            echo $comma' "'$mttfD'" using  1:2:(0):1 with xerror ps 0 lt 0 lw 1.5 lc rgb "#010101" title ""' \\ >>$plotcmd
	    echo ', "" using 1:2                     with impulses    lt 0 lw 1.5 lc rgb "#010101" title ""' \\ >>$plotcmd
            echo ', "" using 1:2                                      pt 3 lw 2   lc rgb "#0000ff" title "'$mttfT'"' \\ >> $plotcmd
	fi
    done
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

 #echo "coral start" 1>&2

if [ X$COMPUTATION = "Xmttf" ]
then
  #date 1>&2

  if [ "X$MTTFPLOTUPB" != "X" ]
  then
    testRealValue MTTFPLOTUPB 'plot upb'
  fi
  if [ "X$MTTFPLOTSTEP" != "X" ]
  then
    testRealValue MTTFPLOTUPB 'plot step'
  fi
  if errorsfound
  then
    cleanup
    exit
  fi

  # NOTE here we invoke dftcalc with -p to make the MTTF value appear on the screen too
  # without -p we would only get the plot
  if [ "X$VERBOSE" != "X--verbose=-1" ]; then
    echo command: dftcalc $MODELCHECKER -R --no-color $COLOR  -q $VERBOSE $MINMAXTIME -m $EVDNCE $ERRORB -c $tmpcsv -p $dft 1>&2
    echo "" 1>&2
  fi

  mttf=0
  if dftcalc $MODELCHECKER -R --no-color $COLOR  -q $VERBOSE $MINMAXTIME -m $EVDNCE $ERRORB -c $tmpcsv -p $dft 1>&2
  then
    #echo "dftcalc finished" 1>&2
    #date 1>&2
    mttf=`tail -n +2 < $tmpcsv`
  else
    cleanup
    exit
  fi
  #date 1>&2

  if [ "X$mttf" != "X0" ]
  then
      if [ "X$VERBOSE" != "X--verbose=-1" ]; then
        echo "" 1>&2
        echo command: dftcalc  -R --no-color $COLOR  -q $VERBOSE --no-nd-warning --imca $MINMAXPROB -t "$mttf" $EVDNCE $ERRORB -c $tmppntcsv $dft 1>&2
        echo "" 1>&2
      fi
      if dftcalc  -R --no-color $COLOR  -q $VERBOSE --no-nd-warning --imca $MINMAXPROB -t "$mttf" $EVDNCE $ERRORB -c $tmppntcsv $dft 1>&2
      then
        #echo "dftcalc finished" 1>&2
        #date 1>&2
        mttf_y=`tail -n +2 < $tmppntcsv |awk '{print $2}'`
        echo "$mttf" "$mttf_y" > $sesfilepfx/$plotdata
      else
        cleanup
        exit
      fi
  else
     mttf_y=1
     echo "$mttf" "$mttf_y" > $sesfilepfx/$plotdata
  fi

  if [ "X$mttf" != "X0" ]
  then
      upb=` echo "$mttf * 5" | bc -l`
      step=` echo "$upb / 20" | bc -l`
  else
     upb=1
     step=0.1
  fi

  if [ "X$MTTFPLOTUPB" != "X" ]
  then
    upb="$MTTFPLOTUPB"
  fi
  if [ "X$MTTFPLOTSTEP" != "X" ]
  then
    step="$MTTFPLOTSTEP"
  fi

#  if [ -r output/$TOOL$pid.ma ]
#  then
#	cp output/$TOOL$pid.ma /tmp/$TOOL$pid.ma
#	echo "copied: output/$TOOL$pid.ma /tmp/$TOOL$pid.ma" 1>&2
#  fi
#  if [ -r output/$TOOL$pid.bcg ]
#  then
#	cp output/$TOOL$pid.bcg /tmp/$TOOL$pid.bcg
#	echo "copied: output/$TOOL$pid.bcg /tmp/$TOOL$pid.bcg" 1>&2
#  fi

  #date 1>&2
  if [ "X$VERBOSE" != "X--verbose=-1" ]; then
    echo "" 1>&2
    echo command: dftcalc -R --no-color $COLOR -q $VERBOSE --no-nd-warning --imca $MINMAXPROB -i 0 "$upb" "$step" $EVDNCE $ERRORB -c $tmpcurvecsv $dft 1>&2
    echo "" 1>&2
  fi
  if dftcalc -R --no-color $COLOR -q $VERBOSE --no-nd-warning --imca $MINMAXPROB -i 0 "$upb" "$step" $EVDNCE $ERRORB -c $tmpcurvecsv $dft 1>&2
  then
    #echo "dftcalc finished" 1>&2
    #date 1>&2
    tail -n +2 < $tmpcurvecsv | sed -e 's/,//g' -e 's/nan/0/g' > $sesfilepfx/$plotdata.2

    if doplot png $sesfilepfx/$png 'unreliability:'${sesfilepfx}/${plotdata}.2':MTTF:'${sesfilepfx}/${plotdata}  && \
       doplot svg $sesfilepfx/$svg 'unreliability:'${sesfilepfx}/${plotdata}.2':MTTF:'${sesfilepfx}/${plotdata}
    then
       echo "$sesurlpfx/$svg |-=-| Plot showing MTTF and unreliability of given DFT; click for an SVG version of the image |-=-| $sesurlpfx/$svg" 1>&$PUPTOL_IMG_FD
	cp ${sesfilepfx}/${plotdata}.2 $uploaddir/data
	echo "$DSNAME" >  $uploaddir/data_title
	cp ${sesfilepfx}/${plotdata} $uploaddir/mttf
	echo MTTF >  $uploaddir/mttf_title
    else
        echo "internal error: could not generate svg via gnuplot" 1>&2
    fi
  else
    cleanup
    exit
  fi
  #date 1>&2
elif  [ X$COMPUTATION = "Xunreliability" ]
then
  if [ "X$VERBOSE" != "X--verbose=-1" ]; then
    echo command: dftcalc $MODELCHECKER -R  --no-color $COLOR -q $VERBOSE  $EVDNCE $ERRORB -c $tmpcsv "$@" $dft 1>&2
    echo "" 1>&2
  fi
  if dftcalc $MODELCHECKER -R  --no-color $COLOR -q $VERBOSE  $EVDNCE $ERRORB -c $tmpcsv "$@" $dft 1>&2
  then
    #echo "dftcalc finished" 1>&2

    # NOTE user supplied list of values may be in wrong order
    # if we would not sort the x values, we would get a plot with jumps
    # we would like to use 'sort -n' but that gets confused by scientific notation like 1E-1
    # therefore, we use awk to translate x coordinate to nr.dec notation, and then use 'sort -n'
    tail -n +2 < $tmpcsv | sed 's/,//g'  | awk '{printf("%.20f %s\n", $1, $2)}' |sort -n > $sesfilepfx/$plotdata

    #cp output/$TOOL$pid.lab /tmp/$TOOL$pid.lab
    #echo "copied: output/$TOOL$pid.lab /tmp/$TOOL$pid.lab" 1>&2
    #cp output/$TOOL$pid.ctmdpi /tmp/$TOOL$pid.ctmdpi
    #echo "copied: output/$TOOL$pid.ctmdpi /tmp/$TOOL$pid.ctmdpi" 1>&2

    #doplot png $sesfilepfx/$png $sesurlpfx/$png 'unreliability:'${sesfilepfx}/${plotdata}':'
    if doplot svg $sesfilepfx/$svg ':'${sesfilepfx}/${plotdata}':' && \
       doplot png $sesfilepfx/$png ':'${sesfilepfx}/${plotdata}':'
    then
       echo "$sesurlpfx/$svg |-=-| Plot showing unreliability of given DFT; click for an SVG version of the image |-=-| $sesurlpfx/$svg" 1>&$PUPTOL_IMG_FD
	cp ${sesfilepfx}/${plotdata} $uploaddir/data
	echo "$DSNAME" >  $uploaddir/data_title
    else
        echo "internal error: could not generate svg via gnuplot" 1>&2
    fi
  else
    cleanup
    exit
  fi
else
  testRealValue TIMELWB 'mission time lwb'
  testRealValue TIMEUPB 'mission time upb'
  if errorsfound
  then
    cleanup
    exit
  else
    echo "No plot is shown for this, only textual output:" 1>&2
    NOPLOT=1
    echo "" 1>&2
    if [ "X$VERBOSE" != "X--verbose=-1" ]; then
      echo command: dftcalc $MODELCHECKER -R  --no-color $COLOR -q $VERBOSE $MINMAXPROB -I "$TIMELWB" "$TIMEUPB" -p $EVDNCE $ERRORB $dft 1>&2
      echo "" 1>&2
    fi
    if dftcalc $MODELCHECKER -R  --no-color $COLOR -q $VERBOSE $MINMAXPROB -I "$TIMELWB" "$TIMEUPB" -p $EVDNCE $ERRORB $dft 1>&2
    then
      :
    else
      cleanup
      exit
    fi
  fi
fi

# make sure group members can clean up
chmod -R g+w .

rm -f $tmpdft $plotcmd 
#rm -rf $CORALTMPDIR

if [ "X$NOPLOT" = "X1" ]
then
    echo "" 1>&2
    echo "Not storing plot data, because there is no plot data to be stored." 1>&2
    # clean away uploaddir
    cleanup
elif [ ! -r $uploaddir ]
then
    echo "" 1>&2
    echo "Not storing plot data, because data directory seems to have been removed already (due to earlier error?)." 1>&2
else
    echo '{ "Name": "'$count'.dataset", "ParamName":"datasets", "Type":"checkbox", "Container":"div", "Title":"'"$LONGDSNAME"'", "Value":"'"$count"'", "DeleteInfo": "'"$TOOL/datasets/$count"'" }'  > $PUPTOLROOT/data/session/$SESSION/$TOOL/datasets/$count
    echo "$count" 1>&$PUPTOL_DFTCALC_datasets_FD
fi

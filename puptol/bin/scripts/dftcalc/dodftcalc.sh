#!/bin/sh

# Get the aliases and functions
export HOME=/home/dftcalc
if [ -f $HOME/.bashrc ]; then
	. $HOME/.bashrc
fi

d=`echo $0 |sed 's/[^/]*$//' `
# echo $d
. $d/settings

sed 's///g'  > $tmpdft

# env 1>&2
# echo $coral -f $tmpdft "$@" 1>&2

testVersionValue VERSION 'version'
testRealValue ERRORBOUND 'error bound'
testIdentifierListValue EVIDENCE 'evidence'
testVerbosityValue VERBOSE 'verbosity'
testColorValue COLOR 'color'
if errorsfound
then
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

ATTACK=""
if [ "X$AT" != "X" ]
then
  ATTACK="$AT"
fi

mkdir $TOOLTMPDIR

# next cd is necessary because dft2bcg wants to write a temporary file in current working directory
cd $TOOLTMPDIR

cp $PUPTOLROOT/data/shared/dftcalc/erl2_2.bcg .
cp $tmpdft .

if [ "X$VERBOSE" != "X--verbose=-1" ]; then
	echo Using dftcalc version: $VERSION 1>&2
	echo "which corresponds to... (asking dftcalc for version info)" 1>&2
	echo "" 1>&2
	
	echo command: dftcalc $ATTACK -R --no-color $COLOR -q $VERBOSE --version 1>&2
	echo "" 1>&2
	dftcalc $ATTACK -R --no-color $COLOR -q $VERBOSE --version 1>&2
	echo "" 1>&2
fi


if [ "X$VERBOSE" != "X--verbose=-1" ]; then
	echo command: "dft2lntc $ATTACK --no-color $COLOR -q $VERBOSE $EVDNCE -t $tmpcandft < $tmpdft" 1>&2
	echo "" 1>&2
fi
if ! dft2lntc $ATTACK --no-color $COLOR -q $VERBOSE $EVDNCE -t $tmpcandft < $tmpdft 1>&2
then
	echo "could not canonicalize DFT" 1>&2
	exit
else
	echo "Canonicalizing by computing SHA-256 of (canonical) DFT and verbose dftcalc version info" 1>&2
	dftcalc $ATTACK -R --no-color $COLOR -q $VERBOSE --version >  $tmptxt 2>&1
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

#echo COMPUTATION $COMPUTATION 1>&2
#echo arguments "$@" $tmpdft 1>&2
#for a in "$@"
#do
#	echo arg: "$a" 1>&2
#done
#echo PATH $PATH 1>&2
#echo env:  1>&2
#env  1>&2

# $dftcalc "$@" $tmpdft
# $dftcalc --no-color -v -v -v -v -v -v -p $tmpdft
#$dftcalc --no-color -v -v -v -v -v -v -p "$@" $tmpdft
#$dftcalc --no-color -v -v -v -p "$@" $tmpdft

MODUL="";
echo "Mod: $MODULARIZE"
if [ "$MODULARIZE" != "" ]; then
	MODUL="-M ";
fi

if [ X$COMPUTATION = Xunreliability ]; then
	if [ "X$VERBOSE" != "X--verbose=-1" ]; then
		echo command: dftcalc $MODELCHECKER $ATTACK -R --no-color $COLOR -q $VERBOSE -p $EVDNCE $ERRORB $MODUL "$@" $dft 1>&2
		echo "" 1>&2
	fi
	dftcalc $MODELCHECKER $ATTACK -R --no-color $COLOR -q $VERBOSE -p $EVDNCE $ERRORB $MODUL "$@" $dft
	#dftcalc -R --no-color -q -p $ERRORB "$@" $dft
	#dftcalc -R --no-color -q --imca -p "$@" $dft
fi
if [ X$COMPUTATION = Xmttf ]; then
	if [ "X$VERBOSE" != "X--verbose=-1" ]; then
		echo command: dftcalc $MODELCHECKER $ATTACK -R --no-color $COLOR -q $VERBOSE -p $MINMAXTIME -m $EVDNCE $ERRORB $MODUL $dft 1>&2
		echo "" 1>&2
	fi
	dftcalc $MODELCHECKER -R --no-color $COLOR -q $VERBOSE -p $MINMAXTIME -m $EVDNCE $ERRORB $MODUL $dft
	#echo "" 1>&2
	#echo "" 1>&2
	#echo "memrec statistics:" 1>&2
	#cat $tmpmemrec 1>&2
	#echo "end of memrec statistics" 1>&2

#	if $dftcalc -R  --no-color -q  --imca -p -f "-et -max"  -c $tmpcsv $dft 1>&2
#	then
#		cat $tmpcsv
#	else
#		echo "internal error: could not compute mean time to to failure" 1>&2
#	fi
fi
if [ "$COMPUTATION" = "steadyState" ]; then
	if [ "X$VERBOSE" != "X--verbose=-1" ]; then
		echo command: dftcalc $MODELCHECKER $ATTACK -R --no-color $COLOR -q $VERBOSE -p $MINMAXTIME -s $EVDNCE $ERRORB $MODUL $dft 1>&2
		echo "" 1>&2
	fi
	dftcalc $MODELCHECKER -R --no-color $COLOR -q $VERBOSE -p $MINMAXTIME -s $EVDNCE $ERRORB $MODUL $dft
fi
if [ X$COMPUTATION = Xlwbupb ]; then
	testRealValue TIMELWB 'mission time lwb'
	testRealValue TIMEUPB 'mission time upb'
	if ! errorsfound
	then
		if [ "X$VERBOSE" != "X--verbose=-1" ]; then
			echo command: dftcalc $MODELCHECKER $ATTACK -R --no-color $COLOR -q $VERBOSE -p $MINMAXPROB -I "$TIMELWB" "$TIMEUPB" $EVDNCE $ERRORB $MODUL $dft 1>&2
			echo "" 1>&2
		fi
		dftcalc $MODELCHECKER $ATTACK -R --no-color $COLOR -q $VERBOSE -p $MINMAXPROB -I "$TIMELWB" "$TIMEUPB" $EVDNCE $ERRORB $MODUL $dft
	fi
fi

# make sure group members can clean up
chmod -R g+w .

rm -f $tmpdft
#rm -rf $CORALTMPDIR

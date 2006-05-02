#!/bin/bash
#
# Copyright Paul Reeves 2006
#
#


ShowHelp(){
echo ""
echo "  Command line Options:"
echo ""
echo "  No parameters are required. All params default to sensible values  "
echo ""
echo "    `basename $0`  [option ..]"
echo "    `basename $0`  --help"
echo ""
echo "  You can supply the following arguments:"
echo ""
# Maybe we'll add this support later, but for now the build depends on
# locations relative to the root dir.
#echo "--vulcan=path       Location of compiled binaries.                   OPT."
#echo "                      Defaults to ../../install                          "
#echo ""
#echo "--vulcan_root=path  Location of root of source tree.                 OPT."
#echo "                      Defaults to ../../                                 "
#echo ""
echo "  --platform      Build platform                                   OPT."
echo "                    Defaults to linux32                                "
echo "                    e.g. --platform=linux32                            "
echo "                    Support for other platforms is incomplete          "
echo "                    Possible values are darwin darwin64 linux64 solaris64 "
echo ""
echo "  --debug         Do a debug build                                 OPT."
echo "                    Defaults to false                                  "
echo "                    e.g. --debug                                       "
echo ""
echo "  --prepare       Prepare tree for build including:                OPT."
echo "                    - configure                                        "
echo "                    - build and run VSRelo                             "
echo "                    - build and run buildgen                           "
echo "                  Default is to prepare and build (no params reqd.)    "
echo ""
echo "  --skipautoconf  Skip running the autoconf/configure code         OPT."
echo "                    This switch is ignored if autoconf                 "
echo "                    has not yet been run.                              "
echo ""
echo "  --build         Skip preparation and only do a build             OPT."
echo "                  Default is to prepare and build (no params reqd.)    "
echo ""
echo "  --quiet         Disable script commentary in execution           OPT."
echo ""
echo "  --clean         Clean source tree                                OPT."
echo ""
echo "  --cleanonly     Clean source tree and do nothing else            OPT."
echo ""
echo "  --help          This help screen"
echo ""

exit 1

}

ClearEnv() {
unset VULCAN_SOURCE VULCAN_DEBUG VULCAN_ROOT VULCAN VULCAN_QUIET
unset VULCAN_PREPARE VULCAN_SKIP_AUTOCONF VULCAN_BUILD_ONLY
unset VULCAN_PLATFORM VULCAN_PLATFORM_DIR
unset VULCAN_CLEAN VULCAN_CLEAN_ONLY VULCAN_START_BUILD VULCAN_END_BUILD

}

GetTime(){
	if [ -z $VULCAN_START_BUILD ]; then
		VULCAN_START_BUILD=$(date +%s)
	else
		VULCAN_END_BUILD=$(date +%s)
	fi
}

EvaluateCommandLine(){
for Option in $*
do
	OptionValue=`echo "$Option" | cut -d'=' -f2`
	OptionName=`echo "$Option" | cut -d'=' -f1`
	case $OptionName in
		--help)
			ShowHelp
			;;

		--vulcan)
			VULCAN_ROOT=$OptionValue
			;;

		--platform)
			VULCAN_PLATFORM=$OptionValue
			;;

		--debug)
			VULCAN_DEBUG="DEBUG"
			;;

		--prepare)
			VULCAN_PREPARE="1"
			;;

		--build)
			VULCAN_BUILD_ONLY="1"
			;;

		--clean)
			VULCAN_CLEAN="1"
			;;
		--cleanonly)
			VULCAN_CLEAN="1"
			VULCAN_CLEAN_ONLY="1"
			;;

		--mapback)
			echo
			echo This option has not yet been implemented on this platform
			echo
			exit 1
			;;

		--quiet)
			VULCAN_QUIET="1"
			;;
			
		--skipautoconf)
			VULCAN_SKIP_AUTOCONF="1"
			;;
#		--string)
#			ASTRING=$OptionValue ;;
#		--boolean)
#			if [ "$OptionValue" = "true" -o "$OptionValue" = "yes" -o "$OptionValue" = "TRUE" -o "$OptionValue" = "YES" ]; #then
#				ABOOL="1"
#            else
#			    ABOOL="0"
#			fi
#			;;
		*)
			echo "**** Unknown option $Option ****" 1>&2
			ShowHelp
			;;
	esac
done


}


SetEnv() {
	if [ -z $VULCAN_QUIET ]; then
		echo Setting environment...
	fi

#get directory that script is stored in.
	startdir=`dirname $0`

#This may not be the same directory that we started in,
#so switch to the script dir.
# and move to top of source tree.
	cd ../..

#Now we can set our main env. vars.
	if [ -z $ISC_USER ]; then
		ISC_USER=SYSDBA
	fi

	if [ -z $ISC_PASSWORD ]; then
		ISC_PASSWORD=masterkey
	fi

	export ISC_USER ISC_PASSWORD

	if [ -z $VULCAN_PLATFORM ]; then
		VULCAN_PLATFORM=linux32
	fi

	VULCAN_ROOT=$PWD
	VULCAN=$VULCAN_ROOT/install
	VULCAN_BUILD_DIR=$VULCAN_ROOT/src
	VULCAN_SCRIPT_DIR=$VULCAN_ROOT/builds/posix

	if [ "$VULCAN_PLATFORM" = "darwin" -o "$VULCAN_PLATFORM" = "darwin64" ]; then
		VULCAN_PLATFORM_DIR=$VULCAN_ROOT/builds/mac_os_x
	fi

	if [ "$VULCAN_PLATFORM" = "linux32" -o "$VULCAN_PLATFORM" = "linux64" ]; then
		VULCAN_PLATFORM_DIR=$VULCAN_ROOT/builds/posix
	fi

	#symlink gmake if not exists
    if [ ! -L /usr/bin/gmake ]; then
		cp 	-l /usr/bin/make /usr/bin/gmake
	fi

	if [ ! -L $VULCAN_ROOT/src/FbDbc ]; then
		ln -s $VULCAN_ROOT/src/IscDbc $VULCAN_ROOT/src/FbDbc
	fi

	PATH=$VULCAN/bin:$VULCAN/bin64:$PATH
	LD_LIBRARY_PATH=$VULCAN/bin:$VULCAN/bin64:$LD_LIBRARY_PATH

	export VULCAN_ROOT VULCAN VULCAN_BUILD_DIR VULCAN_SCRIPT_DIR VULCAN_PLATFORM_DIR VULCAN_PLATFORM
	export PATH LD_LIBRARY_PATH

	main_module_list="why config gpre remote jrd burp msgs qli isql server intlcpp FbDbc lock alice gsec gstat Services "

	support_module_list="XLoad ThreadTest"
	
	if [ ! -f $VULCAN_ROOT/config.status ]; then
	  unset VULCAN_SKIP_AUTOCONF
	fi
}


RunConfigure() {

	if [ -z $VULCAN_QUIET ]; then
	echo Running autogen.sh and configure...
	fi
	cd $VULCAN_ROOT

	if [ ! -f autogen.sh ]; then
		cp $VULCAN_PLATFORM_DIR/autogen.sh $VULCAN_ROOT
#Warning - path to configure.in is hardcoded to posix directory.
#As the posix build sub-system evolves this _may_ lead to complications.
		cp $VULCAN_SCRIPT_DIR/configure.in $VULCAN_ROOT
		chmod +x $VULCAN_ROOT/autogen.sh
	fi
	source autogen.sh $VULCAN_DEBUG
}


BuildVSRelo() {
	if [ -z $VULCAN_QUIET ]; then
	echo Building VSRelo...
	fi
	cd $VULCAN_ROOT/src/VSRelo
	make install
}


RunVSRelo() {
	if [ -z $VULCAN_QUIET ]; then
		echo Running VSRElo...
		echo -e " ** WARNING ** Next execution line is currently hard-coded paths
		We want to use VULCAN_BUILD_DIR in place of ../../src but
		vcproj files need a relative path which means we need to calculate
		where  VULCAN_BUILD_DIR is in relation to VULCAN_SCRIPT_DIR
		./VSRelo ../../builds/MasterBuildConfig -s -o ../../src -c "
	fi
	cd $VULCAN_SCRIPT_DIR
	./VSRelo ../../builds/MasterBuildConfig -s -o ../../src -c

# Now walk the $VULCAN_ROOT/builds/MasterConfig tree and
# symlink the conf files $VULCAN_BUILD_DIR
	if [ -z $VULCAN_QUIET ]; then
		echo Linking Config and boot files...
	fi
	ln -s $VULCAN_ROOT/builds/MasterBuildConfig/*.conf $VULCAN_BUILD_DIR
	for adir in `find $VULCAN_ROOT/builds/MasterBuildConfig/ -type d -print | grep -v CVS`
	do
		apath=`basename $adir`
		if [ -f $VULCAN_ROOT/builds/MasterBuildConfig/$apath/vulcan.conf ]; then
			ln -s $VULCAN_ROOT/builds/MasterBuildConfig/$apath/vulcan.conf $VULCAN_BUILD_DIR/$apath/vulcan.conf
		fi
		if [ -f $VULCAN_ROOT/builds/MasterBuildConfig/$apath/boot ]; then
			cp $VULCAN_ROOT/builds/MasterBuildConfig/$apath/boot $VULCAN_BUILD_DIR/$apath/boot
		fi
	done

}

Buildbuildgen() {
	if [ -z $VULCAN_QUIET ]; then
		echo Building buildgen...
	fi
	cd $VULCAN_ROOT/src/buildgen
	make install
}


Runbuildgen() {
	if [ -z $VULCAN_QUIET ]; then
		echo Creating makefiles for $VULCAN_PLATFORM platform...
	fi

#	if [ -e $VULCAN_SCRIPT_DIR/buildgen ] ; then
#		rm $VULCAN_SCRIPT_DIR/buildgen
#	fi

#	ln -s  $VULCAN_SCRIPT_DIR/buildgen.$VULCAN_PLATFORM $VULCAN_SCRIPT_DIR/buildgen

cat <<end >$VULCAN_ROOT/src/setup.sed
s/proto/$VULCAN_PLATFORM/
end

	cd $VULCAN_ROOT/src
	for x in $main_module_list $support_module_list
	do
		if [ -z $VULCAN_QUIET ]; then
			echo Working in $x
		fi

		cd $x

		if [ -e gen ] ; then
			rm gen;
		fi;

		sed  -f ../setup.sed gen.proto > gen.temp

		mv gen.temp gen.$VULCAN_PLATFORM

		chmod +x gen.$VULCAN_PLATFORM

		ln -s gen.$VULCAN_PLATFORM gen

		if [ ! -e makefile.in ] ; then
			cp /dev/null makefile.in
		fi

		if [ -e boot ] ; then
			chmod +x boot
		fi

		if [ -e clean ] ; then
			chmod +x clean ;
		fi

		cd $OLDPWD

	done
	rm setup.sed

	#Now set up install dir directory tree
	for x in bin bin64 databases help intl intl64 lib
	do
		mkdir  -p $VULCAN/$x
	done

}

CleanLastBuildOutput() {
	rm -f $VULCAN/bin/*.so \
		$VULCAN/bin/config \
		$VULCAN/bin/gpre \
		$VULCAN/bin/gbak \
		$VULCAN/bin/qli \
		$VULCAN/bin/isql \
		$VULCAN/firebird.msg \
		$VULCAN/security.fdb \
		$VULCAN/databases/msg.fdb \
		$VULCAN/help/help.fdb > /dev/null 2>&1

}


MakeClean() {
	if [ -z $VULCAN_QUIET ]; then
		echo Running make clean target for each module...
	fi

	cd $VULCAN_BUILD_DIR
	for x in $main_module_list $support_module_list
	do
		cd $x
		if [ -f gen ]; then
			if [ -z $VULCAN_QUIET ]; then
				echo Cleaning $x
			fi
			./gen $*
			gmake clean
		fi
		cd ..
	done

    cd $VULCAN_ROOT
}

PrintBuildTime() {
buildtime=`expr $VULCAN_END_BUILD - $VULCAN_START_BUILD`
buildhours=`expr $buildtime / 60 / 60`
buildmins=`expr \( $buildtime - \( $buildhours \* 60 \* 60 \) \) / 60 `
buildsecs=`expr $buildtime % 60`
echo Build took $buildhours:$buildmins:$buildsecs
}


Build() {

	cd $VULCAN_BUILD_DIR

	for x in $main_module_list
	do
		cd $x

		if [ -z $VULCAN_QUIET ]; then
			echo
			echo "Building module $x in directory \`$PWD'"
			echo "build_vulcan.sh: Entering directory \`$PWD'"
			echo
		fi

		if [ -e boot ] ; then
			./boot
		fi

		./gen
		gmake
		gmake install

		cd $OLDPWD

	done

}

RealClean(){
#echo RealClean is currently disabled.
#return 0
	if [ -z $VULCAN_QUIET ]; then
		echo "Cleaning parts that others don't reach..."
	fi

	if [ -z $VULCAN_QUIET ]; then
		echo "Removing files generated by autogen and configure..."
	fi

	rm -Rf autom4te.cache 2>/dev/null
	rm -Rf gen 2>/dev/null
	rm -Rf temp 2>/dev/null

	rm aclocal.m4 2>/dev/null
	rm config.log 2>/dev/null
	rm config.status 2>/dev/null
	rm configure 2>/dev/null
	rm libtool 2>/dev/null
	rm Makefile 2>/dev/null
	rm autogen.sh 2>/dev/null
	rm configure.in 2>/dev/null

	rm src/include/gen/autoconfig.h 2>/dev/null

	if [ -z $VULCAN_QUIET ]; then
		echo Removing files in src directory
	fi

	#this isn't yet re-created, don't remove it for now...
	#rm src/include/gen/blrtable.h 2>/dev/null
	rm src/Makefile 2>/dev/null
	rm src/v5_examples/Makefile 2>/dev/null

	# removing databases
	rm src/indicator.* 2>/dev/null
	rm -f src/*.fdb 2>/dev/null
	rm -f src/*.FDB 2>/dev/null
	rm -f src/*.lnk 2>/dev/null

	# Cpp from epp files
	rm src/burp/backup.cpp 2>/dev/null
	rm src/burp/restore.cpp 2>/dev/null
	rm src/dsql/array.cpp 2>/dev/null
	rm src/dsql/blob.cpp 2>/dev/null
	rm src/dsql/metd.cpp 2>/dev/null
	rm src/dudley/exe.cpp 2>/dev/null
	rm src/gpre/gpre_meta.cpp 2>/dev/null
	rm src/isql/extract.cpp 2>/dev/null
	rm src/isql/isql.cpp 2>/dev/null
	rm src/isql/show.cpp 2>/dev/null
	rm src/jrd/codes.cpp 2>/dev/null
	rm src/jrd/dfw.cpp 2>/dev/null
	rm src/jrd/dpm.cpp 2>/dev/null
	rm src/jrd/dyn.cpp 2>/dev/null
	rm src/jrd/dyn_def.cpp 2>/dev/null
	rm src/jrd/dyn_del.cpp 2>/dev/null
	rm src/jrd/dyn_mod.cpp 2>/dev/null
	rm src/jrd/dyn_util.cpp 2>/dev/null
	rm src/jrd/fun.cpp 2>/dev/null
	rm src/jrd/grant.cpp 2>/dev/null
	rm src/jrd/ini.cpp 2>/dev/null
	rm src/jrd/met.cpp 2>/dev/null
	rm src/jrd/pcmet.cpp 2>/dev/null
	rm src/jrd/scl.cpp 2>/dev/null
	rm src/msgs/build_file.cpp 2>/dev/null
	rm src/msgs/change_msgs.cpp 2>/dev/null
	rm src/msgs/check_msgs.cpp 2>/dev/null
	rm src/msgs/enter_msgs.cpp 2>/dev/null
	rm src/msgs/modify_msgs.cpp 2>/dev/null
	rm src/qli/help.cpp 2>/dev/null
	rm src/qli/meta.cpp 2>/dev/null
	rm src/qli/proc.cpp 2>/dev/null
	rm src/qli/show.cpp 2>/dev/null

	rm $VULCAN_SCRIPT_DIR/VSRelo
	rm $VULCAN_SCRIPT_DIR/buildgen
}


Main(){
	ClearEnv
	GetTime
	EvaluateCommandLine $*
	SetEnv $0

	if [ $VULCAN_CLEAN ]; then
		echo Cleaning build tree...
		MakeClean
		RealClean
		if [ $VULCAN_CLEAN_ONLY ]; then
		    return 0
		fi
		
	fi

	if [ -z $VULCAN_BUILD_ONLY ]; then
		echo Preparing Vulcan build environment
		if [ -z $VULCAN_SKIP_AUTOCONF ]; then
		    RunConfigure
		fi
		BuildVSRelo
		RunVSRelo
		Buildbuildgen
		Runbuildgen
	fi

	if [ -z $VULCAN_PREPARE ]; then
#		echo Building Vulcan
		CleanLastBuildOutput
		Build
	fi

	GetTime
	PrintBuildTime

#go back to where we started
	cd $startdir
}


Main $*

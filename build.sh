#!/bin/bash
# Original Live by cybojenix <anthonydking@gmail.com>
# New Live/Menu by Caio Oliveira aka Caio99BR <caiooliveirafarias0@gmail.com>
# Colors by Aidas Lukošius aka aidasaidas75 <aidaslukosius75@yahoo.com>
# Toolchains by Suhail aka skyinfo <sh.skyinfo@gmail.com>
# Rashed for the base of zip making
# And the internet for filling in else where

# You need to download https://github.com/TeamVee/android_prebuilt_toolchains
# Clone in the same folder as the kernel to choose a toolchain and not specify a location

# Device Choice
device_choice() {
clear
# Devices available
#
# L1 II
device_variants_1="E410 E411 E415 E420" device_defconfig_1="cyanogenmod_v1_defconfig" device_name_1="LG-L1II"
# L3 II
device_variants_2="E425 E430 E431 E435" device_defconfig_2="cyanogenmod_vee3_defconfig" device_name_2="LG-L3II"
# L5
device_variants_3="E610" device_defconfig_3="cyanogenmod_m4_defconfig" device_name_3="LG-L5-NFC"
# L5 NoNFC
device_variants_4="E612 E617" device_defconfig_4="cyanogenmod_m4_nonfc_defconfig" device_name_4="LG-L5-NoNFC"
# L7
device_variants_5="P700" device_defconfig_5="cyanogenmod_u0_defconfig" device_name_5="LG-L7-NFC"
# L7 NoNFC
device_variants_6="P705" device_defconfig_6="cyanogenmod_u0_nonfc_defconfig" device_name_6="LG-L7-NoNFC"
# L7 8m
device_variants_7="P708" device_defconfig_7="cyanogenmod_u0_8m_defconfig" device_name_7="LG-L7-NFC-8m"
# Menu
echo "${x} | ${color_green}Device choice${color_stock}"
echo
echo "1 | ${device_variants_1} | ${device_name_1}"
defconfig_updater ${device_name_1}
echo "2 | ${device_variants_2} | ${device_name_2}"
defconfig_updater ${device_name_2}
echo "3 | ${device_variants_3} | ${device_name_3}"
defconfig_updater ${device_name_3}
echo "4 | ${device_variants_4} | ${device_name_4}"
defconfig_updater ${device_name_4}
echo "5 | ${device_variants_5} | ${device_name_5}"
defconfig_updater ${device_name_5}
echo "6 | ${device_variants_6} | ${device_name_6}"
defconfig_updater ${device_name_6}
echo "7 | ${device_variants_7} | ${device_name_7}"
defconfig_updater ${device_name_7}
echo
echo "* | Any other key to Exit"
echo
read -p "  | Choice | " -n 1 -s x
case "${x}" in
	1) device_defconfig=${device_defconfig_1} device_name=${device_name_1};;
	2) device_defconfig=${device_defconfig_2} device_name=${device_name_2};;
	3) device_defconfig=${device_defconfig_3} device_name=${device_name_3};;
	4) device_defconfig=${device_defconfig_4} device_name=${device_name_4};;
	5) device_defconfig=${device_defconfig_5} device_name=${device_name_5};;
	6) device_defconfig=${device_defconfig_6} device_name=${device_name_6};;
	7) device_defconfig=${device_defconfig_7} device_name=${device_name_7};;
	a)
		if [ -f .config ]
		then
			echo "${x} | Working on ${device_name} defconfig!"
			make -j${build_cpu_usage}${kernel_build_output_enable} savedefconfig
			mv defconfig arch/${ARCH}/configs/${device_defconfig}
		fi;;
	b)
		if [ -f .config ]
		then
			cp .config arch/${ARCH}/configs/${device_defconfig}
		fi;;
esac
if ! [ "${device_defconfig}" == "" ]
then
	echo "${x} | Working on ${device_name} defconfig!"
	make -j${build_cpu_usage}${kernel_build_output_enable} ${device_defconfig}
	sleep 2
fi
}

# Toolchain Choice
toolchain_choice() {
echo "${x} | Toolchain choice"
echo
if [ -f ../android_prebuilt_toolchains/aptess.sh ]
then
	. ../android_prebuilt_toolchains/aptess.sh
else
	if [ -d ../android_prebuilt_toolchains ]
	then
		echo "  | You not have APTESS Script in Android Prebuilt Toolchain folder"
		echo "  | Check the folder"
		echo "  | We will use Manual Method now"
	else
		echo "  | You don't have TeamVee Prebuilt Toolchains"
	fi
	echo
	echo "  | Please specify a location"
	echo "  | and the prefix of the chosen toolchain at the end"
	echo "  | GCC 4.6 ex. ../arm-eabi-4.6/bin/arm-eabi-"
	echo
	echo "  | Stay blank if you want to exit"
	echo
	read -p "  | Place | " CROSS_COMPILE
	if ! [ "${CROSS_COMPILE}" == "" ]
	then
		ToolchainCompile="${CROSS_COMPILE}"
	fi
fi
}

# Kernel Build Process
kernel_build() {
if [ "${CROSS_COMPILE}" == "" ]
then
	wrong_choice
	unset CROSS_COMPILE ToolchainCompile
elif [ "${ToolchainCompile}" == "" ]
then
	wrong_choice
	unset CROSS_COMPILE ToolchainCompile
elif [ "${device_defconfig}" == "" ]
then
	wrong_choice
	unset device_name device_defconfig
elif [ "${device_name}" == "" ]
then
	wrong_choice
	unset device_name device_defconfig
elif [ ! -f .config ]
then
	wrong_choice
	unset device_name device_defconfig
else
	echo "${x} | Building ${builder} ${custom_kernel}"

	if [ $(which ccache) ]
	then
		kernel_build_ccache="ccache "
	fi

	echo "  | ${color_blue}Building ${custom_kernel} with ${build_cpu_usage} jobs at once${ccache_build}${color_stock}"

	start_build_time=$(date +"%s")
	make -j${build_cpu_usage}${kernel_build_output_enable} CROSS_COMPILE="${kernel_build_ccache}${CROSS_COMPILE}"
	sleep 2
	build_time=$(($(date +"%s") - ${start_build_time}))
	build_time_minutes=$((${build_time} / 60))
fi
}

# Zip Packer Process
zip_packer() {
if ! [ "${device_defconfig}" == "" ]
then
	if [ -f arch/${ARCH}/boot/zImage ]
	then
		echo "${x} | Ziping ${builder} ${custom_kernel}"

		zip_out="zip-creator_out"
		rm -rf ${zip_out}
		mkdir -p ${zip_out}/META-INF/com/google/android/

		cp zip-creator/base/update-binary ${zip_out}/META-INF/com/google/android/
		cp zip-creator/base/mkbootimg ${zip_out}/
		cp zip-creator/base/unpackbootimg ${zip_out}/
		cp arch/${ARCH}/boot/zImage ${zip_out}/

		echo "${builder}" >> ${zip_out}/device.prop
		echo "${custom_kernel}" >> ${zip_out}/device.prop
		echo "${device_name}" >> ${zip_out}/device.prop
		echo "Release ${release}" >> ${zip_out}/device.prop

		mkdir ${zip_out}/modules
		find . -name *.ko | xargs cp -a --target-directory=${zip_out}/modules/ &> /dev/null
		${CROSS_COMPILE}strip --strip-unneeded ${zip_out}/modules/*.ko

		cd ${zip_out}
		zip -r ${zipfile} * -x .gitignore &> /dev/null
		cd ..

		cp ${zip_out}/${zipfile} zip-creator/
		rm -rf ${zip_out}
	else
		wrong_choice
	fi
else
	wrong_choice
fi
}

# Updater of defconfigs
defconfig_updater() {
if [ -f .config ]
then
	if [[ "${device_defconfig}" == "${1}" || "${device_name}" == "${1}" ]]
	then
		if [ $(cat arch/${ARCH}/configs/${device_defconfig} | grep "Automatically" | wc -l) == "0" ]
		then
			defconfig_format="a | Default Linux Kernel format  | Small"
		else
			defconfig_format="b | Usual copy of .config format | Complete"
		fi
		echo "  | Update defconfig from:"
		echo "${defconfig_format}"
		echo
		echo "  | to:"
		echo "a | Default Linux Kernel format  | Small"
		echo "b | Usual copy of .config format | Complete"
		echo
	fi
fi
}

# Copy zip's via ADB
zip_copy_adb() {
if [ -f zip-creator/${zipfile} ]
then
	echo "${x} | Coping ${builder} ${custom_kernel}-"
	echo
	adb shell rm -rf /data/media/0/${zipfile} &> /dev/null
	adb push zip-creator/${zipfile} /data/media/0/${zipfile} &> /dev/null
	if ! [ "$?" == "0" ]
	then
		echo "  | Copy failed!"
		if [ ! "$(which adb)" ]
		then
			echo "  | ADB not installed!"
		else
			echo "  | Check connection!"
		fi
		sleep 2
	fi
else
	wrong_choice
fi
}

# Wrong choice
wrong_choice() {
echo "${x} | This option is not available! | Something is wrong! | Check ${color_green}Choice Menu${color_stock}!"; sleep 2
}

if [ ! "${BASH_VERSION}" ]
then
	echo "  | Please do not use ${0} to run this script, just use '. build.sh'"
elif [ -e build.sh ]
then
	# Stock Color
	color_stock=$(tput sgr0)
	# Bold Colors
	color_red=$(tput bold)$(tput setaf 1)
	color_green=$(tput bold)$(tput setaf 2)
	color_yellow=$(tput bold)$(tput setaf 3)
	color_blue=$(tput bold)$(tput setaf 4)
	# Main Variables
	custom_kernel=LProj-CAFKernel
	builder=TeamHackLG
	export ARCH=arm

	while true
	do
		# Kernel OutPut
		if [ "${kernel_build_output}" == "(OFF)" ]
		then
			kernel_build_output_enable=" -s"
		else
			kernel_build_output="(${color_green}ON${color_stock})"
			unset kernel_build_output_enable
		fi
		# Build Time
		if ! [ "${build_time}" == "" ]
		then
			if [ "${build_time_minutes}" == "" ]
			then
				menu_build_time="(Build Time | ${color_green}$((${build_time} % 60))s${color_stock})"
			else
				menu_build_time="(Build Time | ${color_green}${build_time_minutes}m$((${build_time} % 60))s${color_stock})"
			fi
		fi
		build_cpu_usage=$(($(grep -c ^processor /proc/cpuinfo) + 1))
		# Variable's
		k_version=$(cat Makefile | grep VERSION | cut -c 11- | head -1)
		k_patch_level=$(cat Makefile | grep PATCHLEVEL | cut -c 14- | head -1)
		k_sub_level=$(cat Makefile | grep SUBLEVEL | cut -c 12- | head -1)
		kernel_base="${k_version}.${k_patch_level}.${k_sub_level}"
		release=$(date +%d""%m""%Y)
		if ! [ -f .version ]
		then
			echo "0" > .version
		fi
		build=$(cat .version)
		export zipfile="${custom_kernel}-${device_name}-${release}-${build}.zip"
		# Check ZIP
		if [ -f zip-creator/${zipfile} ]
		then
			menu_zipfile="(Saved on | ${color_green}zip-creator/${zipfile}${color_stock})"
		else
			unset menu_zipfile
		fi
		# Menu
		clear
		echo "  | Simple Linux Kernel ${kernel_base} Build Script ($(date +%d"/"%m"/"%Y))"
		echo "  | ${builder} ${custom_kernel} Release $(date +%d"/"%m"/"%Y) Build #${build}"
		echo
		echo "  | ${color_red}Clean Menu${color_stock}"
		echo "1 | Clean Zip Folder"
		echo "2 | Clean Kernel"
		echo "  | ${color_green}Choice Menu${color_stock}"
		echo "3 | Set Device Defconfig ${color_green}${device_name}${color_stock}"
		echo "4 | Set Toolchain        ${color_green}${ToolchainCompile}${color_stock}"
		echo "5 | Toggle Build Output  ${kernel_build_output}"
		echo "  | ${color_yellow}Build Menu${color_stock}"
		echo "6 | Build Kernel         ${menu_build_time}"
		echo "7 | Build Zip Package    ${menu_zipfile}"
		echo "8 | Copy Zip to '/data/media/0' of Device"
		echo "e | Exit"
		echo
		read -n 1 -p "  | Choice | " -s x
		case ${x} in
			1) rm -rf zip-creator/*.zip;;
			2)
				echo "${x} | Cleaning Kernel"
				make -j${build_cpu_usage}${kernel_build_output_enable} clean mrproper
				unset device_name device_defconfig build_time;;
			3) device_choice;;
			4) toolchain_choice;;
			5)
				if [ "${kernel_build_output}" == "(OFF)" ]
				then
					unset kernel_build_output
				else
					kernel_build_output="(OFF)"
				fi;;
			6) kernel_build;;
			7) zip_packer;;
			8) zip_copy_adb;;
			q|e) echo "${x} | Ok, Bye!"; break;;
			*) wrong_choice;;
		esac
	done
else
	echo
	echo "  | Ensure you run this file from the SAME folder as where it was,"
	echo "  | otherwise the script will have problems running the commands."
	echo "  | After you 'cd' to the correct folder, start the build script"
	echo "  | with the . build.sh command, NOT with any other command!"
	echo
fi

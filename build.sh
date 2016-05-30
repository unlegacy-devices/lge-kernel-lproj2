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
echo "-${color_green}Device choice${color_stock}-"
echo
_name=${name}
_variant=${variant}
_defconfig=${defconfig}
unset name variant defconfig
echo "0) ${color_yellow}LG L1 II${color_stock} | Single/Dual | E410 E411 E415 E420"
echo "1) ${color_blue}LG L3 II${color_stock} | Single/Dual | E425 E430 E431 E435"
echo "2) ${color_red}LG L5${color_stock}    | NFC         | E610"
echo "3) ${color_red}LG L5${color_stock}    | NoNFC       | E612 E617"
echo "4) ${color_green}LG L7${color_stock}    | NFC         | P700"
echo "5) ${color_green}LG L7${color_stock}    | NoNFC       | P705"
echo "6) ${color_green}LG L7${color_stock}    | NFC - 8m    | P708"
echo
echo "*) Any other key to Exit"
echo
read -p "Choice: " -n 1 -s x
case "${x}" in
	0) defconfig="cyanogenmod_v1_defconfig"; name="L1II"; variant="All";;
	1) defconfig="cyanogenmod_vee3_defconfig"; name="L3II"; variant="All";;
	2) defconfig="cyanogenmod_m4_defconfig"; name="L5"; variant="NFC";;
	3) defconfig="cyanogenmod_m4_nonfc_defconfig"; name="L5"; variant="NoNFC";;
	4) defconfig="cyanogenmod_u0_defconfig"; name="L7"; variant="NFC";;
	5) defconfig="cyanogenmod_u0_nonfc_defconfig"; name="L7"; variant="NoNFC";;
	6) defconfig="cyanogenmod_u0_8m_defconfig"; name="L7"; variant="NFC-8m";;
esac
if [ "${defconfig}" == "" ]
then
	name=${_name}
	variant=${_variant}
	defconfig=${_defconfig}
	unset _name _variant _defconfig
else
	if ! [ $(make ${defconfig}) ]
	then
		defconfig="${common_message_error}"
	fi | echo "${name} ${variant} setting..."
	unset kernel_build_check zip_packer_check defconfig_check
fi
}

# Toolchain Choice
toolchain_choice() {
clear
echo "-Toolchain choice-"
echo
if [ -f ../android_prebuilt_toolchains/aptess.sh ]
then
	. ../android_prebuilt_toolchains/aptess.sh
else
	if [ -d ../android_prebuilt_toolchains ]
	then
		echo "You not have APTESS Script in Android Prebuilt Toolchain folder"
		echo "Check the folder"
		echo "We will use Manual Method now"
	else
		echo "-You don't have TeamVee Prebuilt Toolchains-"
	fi
	echo
	echo "Please specify a location"
	echo "and the prefix of the chosen toolchain at the end"
	echo "GCC 4.6 ex. ../arm-eabi-4.6/bin/arm-eabi-"
	echo
	echo "Stay blank if you want to exit"
	echo
	read -p "Place: " CROSS_COMPILE
	if ! [ "${CROSS_COMPILE}" == "" ]
	then
		ToolchainCompile="${CROSS_COMPILE}"
	fi
fi
if ! [ "${CROSS_COMPILE}" == "" ]
then
	unset kernel_build_check zip_packer_check
fi
}

# Kernel Build Process
kernel_build() {
if [ -f .config ]
then
	echo "${x} - Building ${customkernel}"

	if [ -f arch/${ARCH}/boot/zImage ]
	then
		rm -rf arch/${ARCH}/boot/zImage
	fi

	build_cpu_usage=$(($(grep -c ^processor /proc/cpuinfo) + 1))
	echo "${color_blue}Building ${customkernel} with ${build_cpu_usage} jobs at once${color_stock}"

	start_build_time=$(date +"%s")
	if [ "${kernel_build_output}" == "OFF" ]
	then
		if ! [ $(make -j${build_cpu_usage}) ]
		then
			loop_fail_check="true"
		fi | kernel_build_LOOP
	else
		make -j${build_cpu_usage}
	fi
	build_time=$(($(date +"%s") - ${start_build_time}))
	build_time_minutes=$((${build_time} / 60))

	if [ -f arch/${ARCH}/boot/zImage ]
	then
		kernel_build_check="${common_message_done}"
	else
		kernel_build_check="${common_message_error}"
	fi
else
	wrong_choice
fi
}

# Kernel Build LOOP function
kernel_build_LOOP() {
while true
do
	loop_build_time=$(($(date +"%s") - ${start_build_time}))
	loop_minutes=$((${loop_build_time} / 60))
	echo -ne "\r\033[K"
	if [ "$loop_minutes" == "" ]
	then
		echo -ne "${color_green}Build Time: $((${loop_build_time} % 60)) seconds.${color_stock}"
	else
		echo -ne "${color_green}Build Time: ${loop_minutes} minutes and $((${loop_build_time} % 60)) seconds.${color_stock}"
	fi
	if [[ -f arch/${ARCH}/boot/zImage || "$loop_fail_check" == "${common_message_error}" ]]
	then
		unset loop_fail_check
		break
	fi
	sleep 1
done
}

# Zip Packer Process
zip_packer() {
if ! [ "${defconfig}" == "" ]
then
	if [ -f arch/$ARCH/boot/zImage ]
	then
		echo "${x} - Ziping ${customkernel}"

		zip_out="zip-creator-out"
		rm -rf ${zip_out}
		mkdir ${zip_out}

		cp -r zip-creator/base/* ${zip_out}/
		cp arch/${ARCH}/boot/zImage ${zip_out}/

		echo "${customkernel}" >> ${zip_out}/device.prop
		echo "${name}" >> ${zip_out}/device.prop
		echo "${variant}" >> ${zip_out}/device.prop
		echo "${release}" >> ${zip_out}/device.prop

		mkdir ${zip_out}/modules
		find . -name *.ko | xargs cp -a --target-directory=${zip_out}/modules/ &> /dev/null
		${CROSS_COMPILE}strip --strip-unneeded ${zip_out}/modules/*.ko

		cd ${zip_out}
		zip -r ${zipfile} * -x .gitignore &> /dev/null
		cd ..

		cp ${zip_out}/${zipfile} zip-creator/
		rm -rf ${zip_out}

		zip_packer_check="${common_message_done}"
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
	if [ $(cat arch/${ARCH}/configs/${defconfig} | grep "Automatically" | wc -l) == "0" ]
	then
		defconfig_format="Default Linux Kernel format  | Small"
	else
		defconfig_format="Usual copy of .config format | Complete"
	fi
	clear
	echo "-${color_green}Updating defconfig${color_stock}-"
	echo
	echo "The actual defconfig is:"
	echo "--${defconfig_format}--"
	echo
	echo "Update defconfig to:"
	echo "1) Default Linux Kernel format  | Small"
	echo "2) Usual copy of .config format | Complete"
	echo
	echo "*) Any other key to Exit"
	echo
	read -p "Choice: " -n 1 -s x
	case "${x}" in
		1) echo "Building..."; make savedefconfig &>/dev/null; mv defconfig arch/${ARCH}/configs/${defconfig};;
		2) cp .config arch/${ARCH}/configs/${defconfig};;
	esac
else
	wrong_choice
fi
}

# Copy zip's via ADB
zip_copy_adb() {
if [ -f zip-creator/${zipfile} ]; then
	clear
	echo "-Coping ${customkernel}-"
	echo
	echo "You want to copy:"
	echo
	echo "1) For Internal Card (sdcard0)"
	echo "2) For External Card (sdcard1)"
	echo
	echo "*) Any other key for exit"
	echo
	read -p "Choice: " -n 1 -s x
	case "$x" in
		1) echo "Coping to Internal Card..."; _ac="sdcard0" ;;
		2) echo "Coping to External Card..."; _ac="sdcard1" ;;
	esac
	if ! [ ${_ac} == "" ]
	then
		adb shell rm -rf /storage/${_ac}/${zipfile} &> /dev/null
		adb push zip-creator/${zipfile} /storage/${_ac}/${zipfile} &> /dev/null
		unset _ac
	fi
else
	wrong_choice
fi
}

# Wrong choice
wrong_choice() {
echo "${x} - This option is not valid"; sleep 1
}

if [ ! "${BASH_VERSION}" ]
then
	echo "Please do not use sh to run this script, just use '. build.sh'"
elif [ -e build.sh ]
then
	# Stock Color
	color_stock=$(tput sgr0)
	# Bold Colors
	color_red=$(tput bold)$(tput setaf 1)
	color_green=$(tput bold)$(tput setaf 2)
	color_yellow=$(tput bold)$(tput setaf 3)
	color_blue=$(tput bold)$(tput setaf 4)
	color_magenta=$(tput bold)$(tput setaf 5)
	color_cyan=$(tput bold)$(tput setaf 6)
	color_white=$(tput bold)$(tput setaf 7)
	# Common Messages
	common_message_start="Ready to do!"
	common_message_done="Already Done!"
	common_message_error="Something goes wrong!"
	# Main Variables
	customkernel=LProj-CAFKernel
	export ARCH=arm

	while true
	do
		if [ "${kernel_build_output}" == "" ]
		then
			kernel_build_output="${color_magenta}ON${color_stock}"
		fi
		if [ "${zip_packer_check}" == "${common_message_done}" ]
		then
			zip_copy_adb_check="${common_message_start}"
		else
			zip_copy_adb_check="Use 8 first"
		fi
		if [ "${kernel_build_check}" == "" ]
		then
			kernel_build_check="${common_message_start}"
			zip_packer_check="Use 7 first"
		fi
		if [ "${kernel_build_check}" == "${common_message_done}" ]
		then
			if ! [ "$zip_packer_check" == "${common_message_done}" ]
			then
				zip_packer_check="${common_message_start}"
			fi
		fi
		if [ "${CROSS_COMPILE}" == "" ]
		then
			kernel_build_check="Use 5 first"
		fi
		if [ "${defconfig}" == "" ]
		then
			kernel_build_check="Use 3 first"
			defconfig_check="Use 3 first"
		else
			defconfig_check="${common_message_start}"
		fi
		if [ "$(ls zip-creator/* | grep 'zip' | wc -l)" == "0" ]
		then
			zip_clean_check="${common_message_done}"
		else
			unset zip_clean_check
		fi
		if [ -f .config ]
		then
			unset kernel_clean_check
		else
			kernel_clean_check="${common_message_done}"
		fi
		if ! [ -f .version ]
		then
			echo "0" > .version
		fi
		if ! [ "${build_time}" == "" ]
		then
			if [ "${build_time_minutes}" == "" ]
			then
				menu_build_time="${color_cyan}$((${build_time} % 60))s${color_stock}"
			else
				menu_build_time="${color_cyan}${build_time_minutes}m$((${build_time} % 60))s${color_stock}"
			fi
		fi
		k_version=$(cat Makefile | grep VERSION | cut -c 11- | head -1)
		k_patch_level=$(cat Makefile | grep PATCHLEVEL | cut -c 14- | head -1)
		k_sub_level=$(cat Makefile | grep SUBLEVEL | cut -c 12- | head -1)
		kernel_base="${k_version}.${k_patch_level}.${k_sub_level}"
		release=$(date +%d""%m""%Y)
		build=$(cat .version)
		export zipfile="${customkernel}-${name}-${variant}-${release}-${build}.zip"

		# Check ZIP
		if [ -f zip-creator/${zipfile} ]
		then
			menu_zipfile="${color_cyan}zip-creator/${zipfile}${color_stock}"
		fi

		clear
		echo "Simple Linux Kernel ${kernel_base} Build Script ($(date +%d"/"%m"/"%Y))"
		echo "${customkernel} Release $(date +%d"/"%m"/"%Y) Build #${build}"
		echo " ${color_red}Clean Menu${color_stock}"
		echo "1 | Zip Package's      | ${color_red}${zip_clean_check}${color_stock}"
		echo "2 | Kernel             | ${color_red}${kernel_clean_check}${color_stock}"
		echo " ${color_green}Choice Menu${color_stock}"
		echo "3 | Device             | ${color_green}${name} ${variant}${color_stock}"
		echo "4 | Update Defconfig   | ${color_green}${defconfig_check}${color_stock}"
		echo "5 | Toolchain          | ${color_green}${ToolchainCompile}${color_stock}"
		echo "6 | Build Output       | ${color_green}${kernel_build_output}${color_stock}"
		echo " ${color_yellow}Build Menu${color_stock}"
		echo "7 | Kernel             | ${color_yellow}${kernel_build_check}${color_stock}"
		echo "    Build Time         | ${color_yellow}${menu_build_time}${color_stock}"
		echo "8 | Zip Package        | ${color_yellow}${zip_packer_check}${color_stock}"
		echo "    Zip Saved          | ${color_yellow}${menu_zipfile}${color_stock}"
		echo "9 | Copy Zip to device | ${color_yellow}${zip_copy_adb_check}${color_stock}"
		echo
		echo "(e/q) Exit"
		echo
		read -n 1 -p "$(tput bold)Choice: ${color_stock}" -s x
		case ${x} in
			1) echo "${x} - Cleaning Zips"; rm -rf zip-creator/*.zip; unset zip_packer_check menu_zipfile;;
			2) echo "${x} - Cleaning Kernel"; make clean mrproper &> /dev/null; unset kernel_build_check name variant defconfig build_time;;
			3) device_choice;;
			4) defconfig_updater;;
			5) toolchain_choice;;
			6) if [ "${kernel_build_output}" == "OFF" ]; then unset kernel_build_output; else kernel_build_output="OFF"; fi;;
			7) kernel_build;;
			8) zip_packer;;
			9) zip_copy_adb;;
			q|e) echo "${x} - Ok, Bye!"; break;;
			*) wrong_choice;;
		esac
	done
else
	echo
	echo "Ensure you run this file from the SAME folder as where it was,"
	echo "otherwise the script will have problems running the commands."
	echo "After you 'cd' to the correct folder, start the build script"
	echo "with the . build.sh command, NOT with any other command!"
	echo
fi

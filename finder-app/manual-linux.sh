#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=qemu
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
BUSYBOX= github.com/mirror/busybox

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi


if [ ! -d ${OUTDIR} ]; then
	echo "Creating for the first time outdir : ${OUTDIR}"
	mkdir -p ${OUTDIR}
fi

cd ${OUTDIR} 

if [ ! -d "linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    # TODO: Add your kernel build steps here
    make -j"$(nproc)" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make -j"$(nproc)" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j"$(nproc)" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image modules dtbs
    cd ..
fi
cd ..

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
	#After reruning scripts, being sure I can remove rootfs because of the chown 
	sudo chown -R $(whoami):$(whoami) ${OUTDIR}/rootfs
	rm -rf ${OUTDIR}/rootfs

fi

# TODO: Create necessary base directories
# Very bulky but clearer for me
mkdir -p ${OUTDIR}/rootfs/bin \
        ${OUTDIR}/rootfs/dev \
        ${OUTDIR}/rootfs/etc \
        ${OUTDIR}/rootfs/home \
        ${OUTDIR}/rootfs/lib \
        ${OUTDIR}/rootfs/lib64 \
        ${OUTDIR}/rootfs/proc \
        ${OUTDIR}/rootfs/sbin \
        ${OUTDIR}/rootfs/sys \
        ${OUTDIR}/rootfs/tmp \
        ${OUTDIR}/rootfs/usr \
        ${OUTDIR}/rootfs/var \
        ${OUTDIR}/rootfs/usr/bin \
        ${OUTDIR}/rootfs/usr/lib \
        ${OUTDIR}/rootfs/usr/sbin \
        ${OUTDIR}/rootfs/var/log \
        ${OUTDIR}/rootfs/home/conf 
        
if [ ! -d "${OUTDIR}/busybox" ]
then
    echo "Cloning busybox with source : ${BUSYBOX}"
    git clone ${BUSYBOX}
    cd ${OUTDIR}/busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make menuconfig
else
    cd ${OUTDIR}/busybox
    make menuconfig
fi

# TODO: Make and install busybox
echo "Making and Installing Busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j"$(nproc)"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="../rootfs" install
cd "../rootfs/"


echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

#Leaving rootfs
cd ..

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
echo "Copying files to rootfs / lib"
cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 rootfs/lib/
cp -a ${SYSROOT}/lib64/libm.so.6 rootfs/lib64/
cp -a ${SYSROOT}/lib64/libresolv.so.2 rootfs/lib64/
cp -a ${SYSROOT}/lib64/libc.so.6 rootfs/lib64/

# TODO: Make device nodes
if [ ! -e "rootfs/dev/null" ]
then
        echo "Creating node null"
        sudo mknod -m 666 rootfs/dev/null c 1 3
else
        echo "Node Null already exists"
fi
if [ ! -e "rootfs/dev/console" ]

then
        echo "Creating node console"
        sudo mknod -m 622 rootfs/dev/console c 5 1
else
        echo "No Console already exists"
fi



# TODO: Clean and build the writer utility
echo "Cleaning and building write app"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} ARCH=${ARCH}
cd ..

echo "Copying files write app"
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying files to rootfs/home/ and watching was has been copied"
cp -v ${FINDER_APP_DIR}/*.sh ${OUTDIR}/rootfs/home/
cp -v ${FINDER_APP_DIR}/*.c ${OUTDIR}/rootfs/home/
sudo cp -v ${FINDER_APP_DIR}/Makefile ${OUTDIR}/rootfs/home/
cp -v ${FINDER_APP_DIR}/conf/*.txt ${OUTDIR}/rootfs/home/conf/


# TODO: Chown the root directory
echo "Applying chown to rootfs"
sudo chown -R root:root ${OUTDIR}/rootfs


# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root | gzip -9 > ../initramfs.cpio.gz
cd -
echo "End of script"





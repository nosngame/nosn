#!/bin/sh

#  Automatic build script for libcurl
#  for iPhoneOS and iPhoneSimulator
#
#  Created by Rockee on 2012-7-26.
#  Copyright 2012 Rockee. All rights reserved.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################
#  Change values here                                                     #
#                                                                         #
VERSION="7.65.0"                                                          #
SDKVERSION=`xcrun -sdk iphoneos --show-sdk-version`                       #
DEVELOPER=`xcode-select -print-path`                                      #
ARCHS="i386 x86_64 armv7 armv7s arm64"                                    #
#                                                                         #
###########################################################################
#                                                                         #
# Don't change anything under this line!                                  #
#                                                                         #
###########################################################################

xcodebuild -version
#developer root path
xcode-select -print-path
#developer sdk version
xcrun -sdk iphoneos --show-sdk-version
xcrun -sdk iphonesimulator --show-sdk-version
#developer sdk path
xcrun -sdk iphoneos --show-sdk-path
xcrun -sdk iphonesimulator --show-sdk-path
#developer cmd path
xcrun --sdk iphoneos --find clang
xcrun --sdk iphoneos --find clang++

###########################################################################
#                                                                         #
#                                                                         #
# Build oppenSSL                                                          #
#                                                                         #
echo "##################################################################" #
echo "########################## Build openSSL #########################" #
./ios-build-libssl.sh                                                     #
echo "########################## Build openSSL End!#####################" #
echo "##################################################################" #
#                                                                         #
#                                                                         #
###########################################################################

echo "##################################################################"
echo "########################## Build libcurl #########################"

CURRENTPATH=$(pwd)      #获取当前目录绝对路径
echo "--OK!------ 当前目录:${CURRENTPATH}"
SSLPATH="${CURRENTPATH}/OpenSSL"
echo "--OK!------ 第三方库:${SSLPATH}"

mkdir -p "Curl"
cd "Curl"
CURRENTPATH=$(pwd)      #获取当前目录绝对路径
echo "--OK!------ 当前目录:${CURRENTPATH}"

#判断是否安装Xcode.app
if [ ! \( -d "$DEVELOPER" \) ] ;
then
	echo "--ERR:------ 编译工具:The iPhone SDK could not be found.Folder: \"$DEVELOPER\" does not  exist."
	exit 1
else
    echo "--OK!------ 编译工具:The iPhone SDK be found!!"
fi

set -e
#判断当前目录下是否有curl-VERSION.tar.gz压缩包
if [ ! -e curl-${VERSION}.tar.gz ] ;
then
	echo "--OK!------ 下载文件:curl-${VERSION}.tar.gz"

	curl -O https://curl.haxx.se/download/curl-${VERSION}.tar.gz
else
	echo "--OK!------ 使用文件:curl-${VERSION}.tar.gz"

fi


mkdir -p "${CURRENTPATH}/src"
mkdir -p "${CURRENTPATH}/bin"
mkdir -p "${CURRENTPATH}/lib"
echo "--OK!------ 创建目录:${CURRENTPATH}/src"
echo "--OK!------ 创建目录:${CURRENTPATH}/bin"
echo "--OK!------ 创建目录:${CURRENTPATH}/lib"

tar zxf curl-${VERSION}.tar.gz -C "${CURRENTPATH}/src"      #将压缩包解压缩到src下
echo "--OK!------ 解压文件:curl-${VERSION}.tar.gz到${CURRENTPATH}/src"
cd "${CURRENTPATH}/src/curl-${VERSION}"                     #进入到curl文件夹下
echo "--OK!------ 进入目录:${CURRENTPATH}/src/curl-${VERSION}"

#获取当前目录绝对路径
BUILDPATH=$(pwd)
echo "--OK!------ 当前目录:${BUILDPATH}"


for ARCH in ${ARCHS}
do
    echo "##################BUILDIT(${ARCH} , ${ARCH})##################"

    export IPHONEOS_DEPLOYMENT_TARGET="7.0"
    export CROSS_TOP="${DEVELOPER}/Platforms/${PLATFORM}.platform/Developer"
    export CROSS_SDK="${PLATFORM}${SDKVERSION}.sdk"
    export BUILD_TOOLS="${DEVELOPER}"

    if [[ "${ARCH}" == "i386" || "${ARCH}" == "x86_64" ]]; then
        echo "##################BUILDIT(${ARCH} , ${ARCH})##################"

        PLATFORM="iPhoneSimulator"
        SDKTYPE="iphonesimulator"

        export CC=$(xcrun --sdk $SDKTYPE --find gcc)
        export CXX=$(xcrun --sdk $SDKTYPE --find clang++)

        export CFLAGS="-arch ${ARCH} -pipe -Os -gdwarf-2 -isysroot $(xcrun --sdk $SDKTYPE --show-sdk-path) -mios-simulator-version-min=$IPHONEOS_DEPLOYMENT_TARGET"
        export LDFLAGS="-L${SSLPATH}/bin/${platform}${SDK}-${target}.sdk/lib"
        export PKG_CONFIG_PATH="${SSLPATH}/bin/${platform}${SDK}-${target}.sdk/lib/pkgconfig"

        mkdir -p "${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk"
        LOG="${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk/build-curl-${VERSION}.log"
        ./configure --disable-shared --enable-static --host="${ARCH}-apple-darwin" --with-ssl=$SSLPATH --prefix="${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk/" > "${LOG}" 2>&1
    else
        PLATFORM="iPhoneOS"
        SDKTYPE="iphoneos"

        export CC=$(xcrun --sdk $SDKTYPE --find gcc)
        export CXX=$(xcrun --sdk $SDKTYPE --find clang++)

        export CFLAGS="-arch ${ARCH} -pipe -Os -gdwarf-2 -isysroot $(xcrun --sdk $SDKTYPE --show-sdk-path)  -miphoneos-version-min=$IPHONEOS_DEPLOYMENT_TARGET"
        export LDFLAGS="-L${SSLPATH}/bin/${platform}${SDK}-${target}.sdk/lib"
        export PKG_CONFIG_PATH="${SSLPATH}/bin/${platform}${SDK}-${target}.sdk/lib/pkgconfig"

        mkdir -p "${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk"
        LOG="${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk/build-curl-${VERSION}.log"

        if [[ "${ARCH}" = "arm64" ]];
        then
            ./configure --disable-shared --enable-static  --enable-threaded-resolver --host="aarch64-apple-darwin" --with-ssl=$SSLPATH --prefix="${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk/" > "${LOG}" 2>&1

        else
            ./configure --disable-shared --enable-static  --enable-threaded-resolver --host="${ARCH}-apple-darwin" --with-ssl=$SSLPATH --prefix="${CURRENTPATH}/bin/${PLATFORM}${SDKVERSION}-${ARCH}.sdk/" > "${LOG}" 2>&1
        fi

    fi



    make
#make -j `sysctl -n hw.logicalcpu_max`
    cp lib/.libs/libcurl.a ../../bin/libcurl-${ARCH}.a
    make clean

    echo "--OK!------ Install bin/libcurl-${ARCH}.a OK!"

    echo "################BUILDIT(${target} , ${platform}) End!################"
    echo "#####################################################################"
done


echo "--OK!------ ############################################################"
echo "--OK!------ Build library..."

#归档公用库
#lipo -create ${CURRENTPATH}/bin/libcurl.armv7.a ${CURRENTPATH}/bin/libcurl.i386.a -output ${CURRENTPATH}/lib/libcurl.a
LIBCURL_SRC=""
for ARCH in ${ARCHS}
do
    if [[ "${ARCH}" == "i386" || "${ARCH}" == "x86_64" ]];
    then
        LIBCURL_SRC="${LIBCURL_SRC}${CURRENTPATH}/bin/libcurl-${ARCH}.a "
    else
        LIBCURL_SRC="${LIBCURL_SRC}${CURRENTPATH}/bin/libcurl-${ARCH}.a "
    fi
done
lipo -create ${LIBCURL_SRC} -output ${CURRENTPATH}/lib/libcurl.a


mkdir -p ${CURRENTPATH}/include
cp -R ${BUILDPATH}/include ${CURRENTPATH}/
echo "--OK!------ Building done."
echo "--OK!------ Cleaning up..."
rm -rf ${CURRENTPATH}/src/curl-${VERSION}
echo "--OK!------ Done."
echo "--OK!------ ############################################################"

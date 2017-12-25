#!/bin/bash
DEST=`pwd`/android && rm -rf $DEST
SOURCE=`pwd`

PLATFORM=14
if [ $1 == "64" ];then
        PLATFORM=21
fi
echo "android PLATFORM $PLATFORM"

MYTOOLCHAIN=/tmp/ffmpeg
/root/work/android-ndk-r14b/build/tools/make-standalone-toolchain.sh --platform=android-$PLATFORM --install-dir=$MYTOOLCHAIN
SYSROOT=$MYTOOLCHAIN/sysroot/

#NDK_ROOT=/root/work/android-ndk-r14b
#SYSROOT=$NDK_ROOT/platforms/android-14/arch-arm
#MYTOOLCHAIN=/root/work/android-ndk-r14b/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64


export PKG_CONFIG_PATH=/root/work/openssl-OpenSSL_1_1_0-stable/android/android-14/lib/pkgconfig

OPENSSL_HEADER_DIR=/root/work/openssl-OpenSSL_1_1_0-stable/android/android-14/include
OPENSSL_LIB_DIR=/root/work/openssl-OpenSSL_1_1_0-stable/android/android-14/lib

export PATH=$MYTOOLCHAIN/bin:$PATH:$OPENSSL_HEADER_DIR:$OPENSSL_LIB_DIR
export CC="ccache arm-linux-androideabi-gcc"
#export CC="arm-linux-androideabi-gcc"
export LD=arm-linux-androideabi-ld
export AR=arm-linux-androideabi-ar
#export TMPDIR=/tmp/tmpdir

CFLAGS="-O3 -Wall -mthumb -pipe -fpic -fasm \
  -finline-limit=300 -ffast-math \
  -fstrict-aliasing -Werror=strict-aliasing \
  -fmodulo-sched -fmodulo-sched-allow-regmoves \
  -Wno-psabi -Wa,--noexecstack \
  -nostdlib \
  -DANDROID"


FFMPEG_FLAGS="--target-os=android \
  --enable-cross-compile \
  --cross-prefix=arm-linux-androideabi- \
  --disable-symver \
  --disable-debug \
  --enable-asm \

  --disable-programs \
  --disable-ffplay \
  --disable-ffmpeg \
  --disable-ffprobe \
  --disable-ffserver \

  --disable-doc \
  --disable-htmlpages \
  --disable-manpages \
  --disable-podpages \
  --disable-txtpages \

  --disable-avdevice \
  --disable-devices \

  --enable-shared \
  --enable-static \
  --enable-pic \

  --disable-everything \
  --enable-openssl \
  --enable-avcodec \
  --enable-avformat \
  --enable-swresample \
  --enable-swscale  \
  --enable-network \
  --enable-protocols  \
  --enable-parsers \
  --enable-demuxers \
  --enable-decoders \
  --enable-bsfs \
  --enable-pthreads \
  --enable-jni \
  --enable-mediacodec \
  --enable-decoder=h264_mediacodec \
  --enable-hwaccel=h264_mediacodec \
  --disable-demuxer=sbg \
  --disable-demuxer=dts \
  --disable-parser=dca \
  --disable-decoder=dca \
"

VIDEO_DECODERS="\
   --enable-decoder=h264 "

AUDIO_DECODERS="\
    --enable-decoder=aac \
    --enable-decoder=aac_fixed \
    --enable-decoder=aac_latm \
    --enable-decoder=pcm_s16le \
    --enable-decoder=pcm_s8"

DEMUXERS="\
    --enable-demuxer=h264 \
    --enable-demuxer=m4v \
    --enable-demuxer=mpegvideo \
    --enable-demuxer=mpegps \
    --enable-demuxer=mp3 \
    --enable-demuxer=avi \
    --enable-demuxer=aac \
    --enable-demuxer=pcm-alaw \

    --enable-demuxer=pcm_s16le \
    --enable-demuxer=pcm_s8 "

AUDIO_ENCODERS="\
          --enable-encoder=pcm_s16le"

MUXERS="\
        --enable-muxer=mp4"

PARSERS="\
    --enable-parser=h264 \
    --enable-parser=mpeg4video \
    --enable-parser=mpegaudio \
    --enable-parser=mpegvideo \
    --enable-parser=aac \
    --enable-parser=aac_latm"

#FFMPEG_FLAGS="$FFMPEG_FLAGS $VIDEO_DECODERS $AUDIO_DECODERS $DEMUXERS $AUDIO_ENCODERS $MUXERS $PARSERS"

EXTRA_LIB="-ldl -latomic "

#for version in neon armv7 vfp armv6; do
#for version in armabi; do
#for version in armv7-a; do
for version in armv7-a-neon; do
#for version in arm64-v8a; do

  #cd $SOURCE

  case $version in
    arm64-v8a)
      FFMPEG_FLAGS="$FFMPEG_FLAGS --arch=aarch64 --enable-neon --disable-armv5te --disable-armv6 --disable-armv6t2"
      EXTRA_CFLAGS="-march=arm64 -mfpu=neon -mfloat-abi=softfp -mvectorize-with-neon-quad"
      EXTRA_LDFLAGS="-Wl,--fix-cortex-a8"
      ;;
    armv7-a-neon)
      FFMPEG_FLAGS="$FFMPEG_FLAGS --arch=arm --enable-neon --disable-armv5te --disable-armv6 --disable-armv6t2"
      EXTRA_CFLAGS="-march=armv7-a -mfpu=neon -mfloat-abi=softfp -mvectorize-with-neon-quad"
      EXTRA_LDFLAGS="-Wl,--fix-cortex-a8"
      ;;
    armv7-a)
      EXTRA_CFLAGS="-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp"
      EXTRA_LDFLAGS="-Wl,--fix-cortex-a8"
      ;;
    vfp)
      EXTRA_CFLAGS="-march=armv6 -mfpu=vfp -mfloat-abi=softfp"
      EXTRA_LDFLAGS=""
      ;;
    armv6)
      EXTRA_CFLAGS="-march=armv6"
      EXTRA_LDFLAGS=""
      ;;
    armabi)
      CFLAGS="$CFLAGS -D__ARM_ARCH_5__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5TE__"
      EXTRA_CFLAGS="-march=armv5"
      EXTRA_LDFLAGS=""
      ;;
    *)
      EXTRA_CFLAGS=""
      EXTRA_LDFLAGS=""
      ;;
  esac

  PREFIX="$DEST/$version" && mkdir -p $PREFIX
  FFMPEG_FLAGS="$FFMPEG_FLAGS --prefix=$PREFIX"

  CFLAGS="$CFLAGS -I$OPENSSL_HEADER_DIR -L$OPENSSL_LIB_DIR"
  EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$OPENSSL_LIB_DIR -lcrypto -lssl"

  ./configure $FFMPEG_FLAGS --extra-cflags="$CFLAGS $EXTRA_CFLAGS" --extra-ldflags="$EXTRA_LDFLAGS" --extra-libs="$EXTRA_LIB" | tee $PREFIX/configuration.txt
  cp config.* $PREFIX
  [ $PIPESTATUS == 0 ] || exit 1

  make clean
  make -j 8 || exit 1
  make install || exit 1

  #rm libavcodec/inverse.o
  $CC -lm -lz -shared --sysroot=$SYSROOT -Wl,--no-undefined -Wl,-z,noexecstack $EXTRA_LDFLAGS  libavutil/*.o libavcodec/*.o libavformat/*.o libswresample/*.o  libswscale/*.o compat/strtod.o /root/work/openssl-OpenSSL_1_1_0-stable/android
/android-14/lib/libssl.a /root/work/openssl-OpenSSL_1_1_0-stable/android/android-14/lib/libcrypto.a -o $PREFIX/libffmpeg.so
  cp $PREFIX/libffmpeg.so $PREFIX/libffmpeg.so
  #arm-linux-androideabi-strip --strip-unneeded $PREFIX/libffmpeg.so
done

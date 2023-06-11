#!/usr/bin/env bash
set -ex

if [ -z "$KERNEL_ARCH" ] || [ -z "$S3_HOST" ]; then
  exit 1
fi

# preparation
mkdir -p kernels modules dtbs
rm -f .config defconfig kernels/* modules/* dtbs/*

LOCALVERSION=$(git branch --show-current)
export LOCALVERSION
export MAKEFLAGS="-j${FDO_CI_CONCURRENT:-4}"

GIT_TAG=$(git describe --tags --always)
# FIXME: drop DEBIAN_ARCH
S3_PATH="${S3_HOST}/mesa-lava/${CI_PROJECT_PATH}/${GIT_TAG}/${DEBIAN_ARCH}"

if [ "${KERNEL_ARCH}" == "x86_64" ]; then
  DEFCONFIG="arch/x86/configs/x86_64_defconfig"
  KERNEL_IMAGE_NAME=( bzImage )
elif [ "${KERNEL_ARCH}" == "arm" ]; then
  CROSS_COMPILE=arm-linux-gnueabihf-
  DEFCONFIG="arch/arm/configs/multi_v7_defconfig"
  KERNEL_IMAGE_NAME=( zImage )
  DT_PATH="arch/arm/boot/dts"
  DT=(
    allwinner/sun8i-h3-libretech-all-h3-cc.dtb
    broadcom/bcm2837-rpi-3-b.dtb
    broadcom/bcm2711-rpi-4-b.dtb
    rockchip/rk3288-veyron-jaq.dtb
    nxp/imx/imx6q-cubox-i.dtb
    nvidia/tegra124-jetson-tk1.dtb
  )

elif [ "${KERNEL_ARCH}" == "arm64" ]; then
  CROSS_COMPILE=aarch64-linux-gnu-
  DEFCONFIG="arch/arm64/configs/defconfig"
  KERNEL_IMAGE_NAME=( Image )
  DT_PATH="arch/arm64/boot/dts"
  DT=(
    rockchip/rk3399-gru-kevin.dtb
    amlogic/meson-g12b-a311d-khadas-vim3.dtb
    amlogic/meson-gxl-s805x-libretech-ac.dtb
    amlogic/meson-gxm-khadas-vim2.dtb
    allwinner/sun50i-h6-pine-h64.dtb
    broadcom/bcm2837-rpi-3-b.dtb
    broadcom/bcm2711-rpi-4-b.dtb
    freescale/imx8mq-librem5-devkit.dtb
    freescale/imx8mq-nitrogen.dtb
    mediatek/mt8192-asurada-spherion-r0.dtb
    mediatek/mt8183-kukui-jacuzzi-juniper-sku16.dtb
    nvidia/tegra210-p3450-0000.dtb
    qcom/apq8016-sbc.dtb
    qcom/apq8096-db820c.dtb
    qcom/sc7180-trogdor-lazor-limozeen-nots-r5.dtb
    qcom/sc7180-trogdor-kingoftown.dtb
    qcom/sdm845-cheza-r3.dtb
    qcom/sm8350-hdk.dtb
  )
else
  exit 1
fi

make() {
    command make ARCH="${KERNEL_ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" "$@"
}
export ARCH="${KERNEL_ARCH}"
export CROSS_COMPILE="${CROSS_COMPILE}"

# defconfig
./scripts/kconfig/merge_config.sh ${DEFCONFIG} kernel/configs/mesa3d-ci_"${KERNEL_ARCH}".config

#build
make "${KERNEL_IMAGE_NAME[@]}"
for image in "${KERNEL_IMAGE_NAME[@]}"; do
  cp -v "arch/${KERNEL_ARCH}/boot/${image}" kernels/
done

for dtb_file in "${DT[@]}"; do
    make "$dtb_file"
    cp -v "${DT_PATH}/${dtb_file}" dtbs/
done

# workarounds and specific stuff
if [[ ${KERNEL_ARCH} = "arm64" ]]; then
  { # Google's Cheza
    make Image.lzma  # Google's Cheza
    mkimage \
        -f auto \
        -A arm \
        -O linux \
        -d arch/arm64/boot/Image.lzma \
        -C lzma\
        -b arch/arm64/boot/dts/qcom/sdm845-cheza-r3.dtb \
        kernels/cheza-kernel
    KERNEL_IMAGE_NAME+=( cheza-kernel )
  }
  { # db410c
    gzip -k kernels/Image
    KERNEL_IMAGE_NAME+=( Image.gz )
  }
fi

# modules
make modules
make INSTALL_MOD_PATH=modules modules_install
rm modules/lib/modules/*/build
tar --zstd -cvf modules.tar.zst -C modules .
rm modules -rf

# defconfig template
make savedefconfig

# upload
FILES_TO_UPLOAD=( modules.tar.zst kernels/* )
if [ "${KERNEL_ARCH}" != "x86_64" ]; then
  FILES_TO_UPLOAD+=( dtbs/* )
fi

for f in "${FILES_TO_UPLOAD[@]}"; do
  ci-fairy s3cp --token "${CI_JOB_JWT:?}" "$f" "https://${S3_PATH}/$(basename -a "$f")"
done

git clean --quiet -fdx -e 'ccache/' -e '.config' -e 'defconfig' -e 'modules.tar.zst' -e 'kernels/' -e 'dtbs/'

echo "GIT_TAG: ${GIT_TAG}"

#	PTX Loadable Kernel Module
#
# $FreeBSD: $

#.PATH:  ${.CURDIR}/../../dev/ptx
KMOD    = ptx
SRCS    = ptx.c ptx_iic.c ptx_tuner.c ptx_dma.c ptx_proc.c ptx_sysctl.c pt3_bus.c pt3_i2c.c pt3_tc.c pt3_mx.c pt3_qm.c pt3_dma.c pt3_pci.c
SRCS    += bus_if.h device_if.h pci_if.h pt3_misc.h ptx.h

KMODDIR?= /boot/modules

#DEBUG_FLAGS = -g

.include <bsd.kmod.mk>

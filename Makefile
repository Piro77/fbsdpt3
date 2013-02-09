#	PTX Loadable Kernel Module
#
# $FreeBSD: $

#.PATH:  ${.CURDIR}/../../dev/ptx
KMOD    = ptx
SRCS    = ptx.c ptx_iic.c ptx_tuner.c ptx_dma.c ptx_proc.c ptx_sysctl.c
SRCS    += bus_if.h device_if.h pci_if.h

KMODDIR?= /boot/modules

#DEBUG_FLAGS = -g

.include <bsd.kmod.mk>

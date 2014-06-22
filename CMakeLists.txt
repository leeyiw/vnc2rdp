cmake_minimum_required (VERSION 2.6)
project (vnc2rdp C)

set(VNC2RDP_SRC
	capabilities.c
	capabilities.h
	d3des.c
	d3des.h
	input.c
	input.h
	license.h
	log.c
	log.h
	mcs.c
	mcs.h
	orders.h
	packet.c
	packet.h
	rdp.c
	rdp.h
	vnc2rdp.c
	sec.c
	sec.h
	session.c
	session.h
	tpkt.c
	tpkt.h
	vnc.c
	vnc.h
	vncauth.c
	vncauth.h
	x224.c
	x224.h
	)

add_executable(vnc2rdp ${VNC2RDP_SRC})

#ifndef __MILES_H__
#define __MILES_H__

#include "globals.h"
//#include "timer.h"

#define TOPBIT 0x80000000

/*
struct IRPackage {
	enum {
		PACKAGE_EMPTY, PACKAGE_COMMAND, PACKAGE_SHOOT, PACKAGE_BROKEN
	} state;
	uchar bytes[3];

	//union не используем. байты разворачиваем в id, team, damage при приёме
	uchar id;
	uchar team;
	uchar damage;

} incoming_ir;
*/

void send_ir_fire (uchar id, uchar team, uchar damage);
void send_ir_command (uchar byte1, uchar byte2, uchar byte3);
void send_ir_package(unsigned long data, int nbits);
void init_ir_out();

#endif /* __MILES_H__ */
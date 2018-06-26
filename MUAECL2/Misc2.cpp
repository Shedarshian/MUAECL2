#include "stdafx.h"
#include "Misc2.h"

Ins::Ins(int id, const vector<Parameter*>& paras, bool E, bool N, bool H, bool L, int time)
	:id(id), paras(paras), diff{ E, N, H, L }, time(time) {}

Ins::~Ins() {
	for (auto ptr : paras)
		delete ptr;
}

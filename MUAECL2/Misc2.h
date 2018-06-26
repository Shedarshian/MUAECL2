#pragma once
#include <vector>
#include <variant>
#include <initializer_list>

using namespace std;

//语句所用参数
struct Parameter {
	virtual ~Parameter() {};
	//输出等等？
};
//字面量
struct Parameter_int : public Parameter {
	int val;
};
struct Parameter_float : public Parameter {
	float val;
};
//变量，保证在sub的variables里有
struct Parameter_variable : public Parameter {
	string var;
};
//堆栈量
struct Parameter_stack : public Parameter {
	bool isFloat;
	int id;
};
//环境变量
struct Parameter_env : public Parameter {
	int id; //99xx的整数
};
//跳转的字节数，体现为指针，指向跳转到的语句
struct Parameter_jmp : public Parameter {
	Ins* jumpPoint;
};

//语句类型
struct Ins {
	Ins(int id, const vector<Parameter*>& paras, bool E = true, bool N = true, bool H = true, bool L = true, int time = 0);
	~Ins();
	bool diff[4];
	int time;
	int id;
	vector<Parameter*> paras;
};

//后端用Sub类型
struct fSub {
	string name;
	vector<string> variables;
	vector<Ins> inses;
};
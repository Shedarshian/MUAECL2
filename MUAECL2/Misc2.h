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
	explicit Parameter_int(int val) :val(val) {};
	int val;
};
struct Parameter_float : public Parameter {
	explicit Parameter_float(float val) :val(val) {};
	float val;
};
//变量，保证在sub的variables里有
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(string var) :var(var) {};
	string var;
};
//堆栈量
struct Parameter_stack : public Parameter {
	Parameter_stack(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id;
};
//环境变量
struct Parameter_env : public Parameter {
	explicit Parameter_env(int id) :id(id) {};
	int id; //99xx的整数
};
//跳转的字节数，体现为指针，指向跳转到的语句
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(Ins* jumpPoint) :jumpPoint(jumpPoint) {};
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

struct fRoot {
	vector<fSub> subs;
	vector<string> ecli;
	vector<string> anim;
};
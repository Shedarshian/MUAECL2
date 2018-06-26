#pragma once
#include <vector>
#include <variant>
#include <initializer_list>

using namespace std;

//������ò���
struct Parameter {
	virtual ~Parameter() {};
	//����ȵȣ�
};
//������
struct Parameter_int : public Parameter {
	explicit Parameter_int(int val) :val(val) {};
	int val;
};
struct Parameter_float : public Parameter {
	explicit Parameter_float(float val) :val(val) {};
	float val;
};
//��������֤��sub��variables����
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(string var) :var(var) {};
	string var;
};
//��ջ��
struct Parameter_stack : public Parameter {
	Parameter_stack(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id;
};
//��������
struct Parameter_env : public Parameter {
	explicit Parameter_env(int id) :id(id) {};
	int id; //99xx������
};
//��ת���ֽ���������Ϊָ�룬ָ����ת�������
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(Ins* jumpPoint) :jumpPoint(jumpPoint) {};
	Ins* jumpPoint;
};

//�������
struct Ins {
	Ins(int id, const vector<Parameter*>& paras, bool E = true, bool N = true, bool H = true, bool L = true, int time = 0);
	~Ins();
	bool diff[4];
	int time;
	int id;
	vector<Parameter*> paras;
};

//�����Sub����
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
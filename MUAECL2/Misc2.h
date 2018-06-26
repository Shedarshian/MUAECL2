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
	int val;
};
struct Parameter_float : public Parameter {
	float val;
};
//��������֤��sub��variables����
struct Parameter_variable : public Parameter {
	string var;
};
//��ջ��
struct Parameter_stack : public Parameter {
	bool isFloat;
	int id;
};
//��������
struct Parameter_env : public Parameter {
	int id; //99xx������
};
//��ת���ֽ���������Ϊָ�룬ָ����ת�������
struct Parameter_jmp : public Parameter {
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
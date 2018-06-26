#pragma once
#include <vector>
#include <variant>
#include <initializer_list>

using namespace std;

struct Ins;
struct SubSerializationContext;

//语句所用参数
struct Parameter abstract {
	virtual ~Parameter() {};
	virtual bool is_stack_param() const = 0;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const = 0;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const = 0;
	//输出等等？
};
//字面量
struct Parameter_int : public Parameter {
	explicit Parameter_int(int val) :val(val) {};
	int val;
	virtual bool is_stack_param() const;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
struct Parameter_float : public Parameter {
	explicit Parameter_float(float val) :val(val) {};
	float val;
	virtual bool is_stack_param() const;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//变量，保证在sub的variables里有
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(string var) :var(var) {};
	string var;
	virtual bool is_stack_param() const;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//堆栈量
struct Parameter_stack : public Parameter {
	Parameter_stack(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id;
	virtual bool is_stack_param() const;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//环境变量
struct Parameter_env : public Parameter {
	explicit Parameter_env(int id) :id(id) {};
	int id; //99xx的整数
	virtual bool is_stack_param() const;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//跳转的字节数，体现为指针，指向跳转到的语句
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(Ins* jumpPoint) :jumpPoint(jumpPoint) {};
	Ins* jumpPoint;
	virtual bool is_stack_param() const;
	virtual int32_t get_stack_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};

//语句类型
struct Ins {
	Ins(int id, const vector<Parameter*>& paras, bool E = true, bool N = true, bool H = true, bool L = true, int time = 0);
	~Ins();
	bool diff[4];
	int time;
	int id;
	vector<Parameter*> paras;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
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
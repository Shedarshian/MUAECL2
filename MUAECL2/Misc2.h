#pragma once
#include <vector>
#include <variant>
#include <string>
#include <memory>
#include <initializer_list>

using namespace std;

struct Ins;
struct SubSerializationContext;

//������ò���
struct Parameter abstract {
	virtual ~Parameter() {};
	virtual bool is_float() const = 0;
	virtual bool is_ref_param() const = 0;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const = 0;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const = 0;
	//����ȵȣ�
};
//������
struct Parameter_int : public Parameter {
	explicit Parameter_int(int val) :val(val) {};
	int val;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
struct Parameter_float : public Parameter {
	explicit Parameter_float(float val) :val(val) {};
	float val;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//��������֤��sub��variables����
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(string var, bool isFloat) :isFloat(isFloat), var(var) {};
	bool isFloat;
	string var;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//��ջ��
struct Parameter_stack : public Parameter {
	Parameter_stack(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//��������
struct Parameter_env : public Parameter {
	explicit Parameter_env(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id; //99xx������������
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//��ת���ֽ���������Ϊָ�룬ָ����ת�������
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(Ins* jumpPoint) :jumpPoint(jumpPoint) {};
	const Ins* jumpPoint;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//�ַ���
struct Parameter_string : public Parameter {
	explicit Parameter_string(const string& str) :str(str) {};
	string str;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//���õĲ���
struct Parameter_call : public Parameter {
	/// <summary>Create a <c>Parameter_call</c> object.</summary>
	/// <param name="param">
	/// Reference to a <c>shared_ptr</c> to the actual parameter.
	/// Do NOT pass a <c>shared_ptr</c> that doesn't own an object.
	/// </param>
	/// <param name="isFromFloat">Whether the actual parameter (aka the object pointed to by <c>param</c>) is a floating point value.</param>
	/// <param name="isToFloat">Whether the formal parameter is a floating point value.</param>
	explicit Parameter_call(shared_ptr<Parameter>& param, bool isFromFloat, bool isToFloat) :param(param) {}
	shared_ptr<Parameter> param;
	bool isFromFloat;
	bool isToFloat;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};

//�������
struct Ins {
	Ins(int id, const vector<Parameter*>& paras, bool E = true, bool N = true, bool H = true, bool L = true, int time = 0);
	~Ins();
	bool diff[4];
	int time;
	int id;
	vector<Parameter*> paras;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};

//�����Sub����
struct fSub {
	string name;
	vector<string> variables;
	vector<Ins> inses;

	template<class ... Types>
	void emplaceVar(Types&& ... args) { variables.emplace(args); }
	template<class ... Types>
	void emplaceIns(Types&& ... args) { inses.emplace(args); }
};

struct fRoot {
	vector<fSub> subs;
	vector<string> ecli;
	vector<string> anim;
};
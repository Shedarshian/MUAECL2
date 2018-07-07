#pragma once
#include <vector>
#include <variant>
#include <string>
#include <memory>
#include <initializer_list>

using namespace std;

struct Ins;
struct SubSerializationContext;

//语句所用参数
struct Parameter abstract {
	virtual ~Parameter() {};
	virtual bool is_float() const = 0;
	virtual bool is_ref_param() const = 0;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const = 0;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const = 0;
	//输出等等？
};
//字面量
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
//变量，保证在sub的variables里有
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(uint32_t id_var, bool isFloat) :isFloat(isFloat), id_var(id_var) {};
	bool isFloat;
	uint32_t id_var;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//堆栈量
struct Parameter_stack : public Parameter {
	Parameter_stack(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//环境变量
struct Parameter_env : public Parameter {
	explicit Parameter_env(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	bool isFloat;
	int id; //99xx的整数（正）
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//跳转的字节数
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(uint32_t id_target) :id_target(id_target) {};
	uint32_t id_target;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//字符串
struct Parameter_string : public Parameter {
	explicit Parameter_string(const string& str) :str(str) {};
	string str;
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
};
//调用的参数
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

struct fSubDataEntry abstract {
public:
	virtual ~fSubDataEntry() = default;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const = 0;
	virtual void set_offs(SubSerializationContext& sub_ctx, size_t offs) const;
};

struct DummyIns_Target :public fSubDataEntry {
public:
	explicit DummyIns_Target(uint32_t id_target);
	virtual ~DummyIns_Target() = default;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	virtual void set_offs(SubSerializationContext& sub_ctx, size_t offs) const override;
	uint32_t id_target;
};

//语句类型
struct Ins :public fSubDataEntry {
	Ins(int id, const vector<Parameter*>& paras, bool E = true, bool N = true, bool H = true, bool L = true, int time = 0);
	virtual ~Ins();
	bool diff[4];
	int time;
	int id;
	vector<Parameter*> paras;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
};

//后端用Sub类型
struct fSub {
	fSub(const string& name, uint32_t count_var, const vector<shared_ptr<fSubDataEntry>>& data_entries);
	string name;
	uint32_t count_var;
	vector<shared_ptr<fSubDataEntry>> data_entries;
};

struct fRoot {
	fRoot(const vector<fSub>& subs, const vector<string>& ecli, const vector<string>& anim);
	vector<fSub> subs;
	vector<string> ecli;
	vector<string> anim;
};

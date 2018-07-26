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
	virtual Parameter* Duplicate() const = 0;
	virtual bool isFloat() const = 0;
	virtual bool isRefParam() const = 0;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const = 0;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const = 0;
	//输出等等？
};
//字面量
struct Parameter_int : public Parameter {
	explicit Parameter_int(int val) :val(val) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_int(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	int val;
};
struct Parameter_float : public Parameter {
	explicit Parameter_float(float val) :val(val) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_float(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	float val;
};
//变量，保证在sub的variables里有
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(uint32_t id_var, bool is_float) :is_float(is_float), id_var(id_var) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_variable(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	bool is_float;
	uint32_t id_var;
};
//堆栈量
struct Parameter_stack : public Parameter {
	Parameter_stack(int32_t ref_id, bool is_float) :is_float(is_float), ref_id(ref_id) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_stack(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	bool is_float;
	int32_t ref_id; //-x的整数（负）
};
//环境变量
struct Parameter_env : public Parameter {
	explicit Parameter_env(int env_id, bool is_float) :is_float(is_float), env_id(env_id) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_env(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	bool is_float;
	uint32_t env_id; //99xx的整数（正）
};
//跳转的字节数
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(uint32_t id_target) :id_target(id_target) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_jmp(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	uint32_t id_target;
};
//字符串
struct Parameter_string : public Parameter {
	explicit Parameter_string(const string& str) :str(str) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_string(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	string str;
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
	explicit Parameter_call(const shared_ptr<Parameter>& param, bool is_from_float, bool is_to_float) :param(param), is_from_float(is_from_float), is_to_float(is_to_float) {}
	virtual Parameter* Duplicate() const override {
		return new Parameter_call(*this);
	}
	virtual bool isFloat() const override;
	virtual bool isRefParam() const override;
	virtual int32_t getRefId(const SubSerializationContext& sub_ctx) const override;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
	shared_ptr<Parameter> param;
	bool is_from_float;
	bool is_to_float;
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
	Ins(int id, const vector<Parameter*>& paras, uint8_t difficulty_mask = 0xFF, int time = 0);
	virtual ~Ins();
	uint8_t difficulty_mask;
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

	template<class ... Types>
	void emplaceVar(Types&& ... args) { variables.emplace(args); }
	template<class ... Types>
	void emplaceIns(Types&& ... args) { inses.emplace(args); }
};

struct fRoot {
	fRoot(const vector<fSub>& subs, const vector<string>& ecli, const vector<string>& anim);
	vector<fSub> subs;
	vector<string> ecli;
	vector<string> anim;
};

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
	virtual Parameter* Duplicate() const = 0;
	virtual bool is_float() const = 0;
	virtual bool is_ref_param() const = 0;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const = 0;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const = 0;
	//����ȵȣ�
};
//������
struct Parameter_int : public Parameter {
	explicit Parameter_int(int val) :val(val) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_int(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	int val;
};
struct Parameter_float : public Parameter {
	explicit Parameter_float(float val) :val(val) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_float(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	float val;
};
//��������֤��sub��variables����
struct Parameter_variable : public Parameter {
	explicit Parameter_variable(uint32_t id_var, bool isFloat) :isFloat(isFloat), id_var(id_var) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_variable(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	bool isFloat;
	uint32_t id_var;
};
//��ջ��
struct Parameter_stack : public Parameter {
	Parameter_stack(int32_t id, bool isFloat) :isFloat(isFloat), id(id) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_stack(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	bool isFloat;
	int32_t id;
};
//��������
struct Parameter_env : public Parameter {
	explicit Parameter_env(int id, bool isFloat) :isFloat(isFloat), id(id) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_env(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	bool isFloat;
	int id; //99xx������������
};
//��ת���ֽ���
struct Parameter_jmp : public Parameter {
	explicit Parameter_jmp(uint32_t id_target) :id_target(id_target) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_jmp(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	uint32_t id_target;
};
//�ַ���
struct Parameter_string : public Parameter {
	explicit Parameter_string(const string& str) :str(str) {};
	virtual Parameter* Duplicate() const override {
		return new Parameter_string(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	string str;
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
	virtual Parameter* Duplicate() const override {
		return new Parameter_call(*this);
	}
	virtual bool is_float() const;
	virtual bool is_ref_param() const;
	virtual int32_t get_ref_id(const SubSerializationContext& sub_ctx) const;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const;
	shared_ptr<Parameter> param;
	bool isFromFloat;
	bool isToFloat;
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

//�������
struct Ins :public fSubDataEntry {
	Ins(int id, const vector<Parameter*>& paras, uint8_t difficulty_mask = 0xFF, int time = 0);
	virtual ~Ins();
	uint8_t difficulty_mask;
	int time;
	int id;
	vector<Parameter*> paras;
	virtual size_t serialize(char* ptr, size_t size_buf, const SubSerializationContext& sub_ctx) const override;
};

//�����Sub����
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

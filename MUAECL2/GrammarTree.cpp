#include "stdafx.h"
#include "GrammarTree.h"
#include <cmath>

using namespace std;

void GrammarTree::changeid(int id) { throw(ErrDesignApp("GrammarTree::changeid")); }
Op::NonTerm GrammarTree::type() const { throw(ErrDesignApp("GrammarTree::type")); }
int GrammarTree::state() const { throw(ErrDesignApp("GrammarTree::state")); }
void GrammarTree::addTree(GrammarTree* t) { throw(ErrDesignApp("GrammarTree::addTree")); }
list<GrammarTree*>* GrammarTree::extractdecl(vector<mVar>& v) { return nullptr; }
void GrammarTree::extractlabel(map<string, GrammarTree*>& labels) {}
Token* GrammarTree::getToken() const { throw(ErrDesignApp("GrammarTree::getToken")); }
mType GrammarTree::getType() const { throw(ErrDesignApp("GrammarTree::getType")); }
int GrammarTree::getLineNo() const { throw(ErrDesignApp("GrammarTree::getLineNo")); }
mVType GrammarTree::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) { throw(ErrDesignApp("GrammarTree::TypeCheck")); }
GrammarTree* GrammarTree::typeChange(Op::Rank rank) { throw(ErrDesignApp("GrammarTree::typeChange")); }
bool GrammarTree::isLabel() const { return false; }

/*tState::tState(int state) :_state(state) {}
int tState::state() const { return _state; }*/

tTerminator::tTerminator(Token* t) :t(t) {}
tTerminator::~tTerminator() { delete t; }
Token* tTerminator::getToken() const { return t; }
mType tTerminator::getType() const { return t->getType(); }
int tTerminator::getLineNo() const { return t->getlineNo(); }
GrammarTree* tTerminator::typeChange(Op::Rank rank) {
	//一定是右值
	if (rank.test(Op::Rank::ITOF)) {
		//整数转浮点
		Token* tok = new Token_Literal(t->getlineNo(), (float)*t->getInt());
		delete t;
		t = tok;
		return this;
	}
	return this;
}

tSubVars::tSubVars(mType type, string id) { vars.emplace_back(type, id); }

void tSubVars::emplaceVar(mType type, string id, int lineNo) {
	auto same = accumulate(vars.begin(), vars.end(), false,
		[&str = id](bool b, mVar& a) {
			return (a.id == str) || b; });
	if (same)
		throw(ErrVarRedeclared(lineNo, id));
	vars.emplace_back(type, id);
}

Op::NonTerm tSubVars::type() const { return Op::NonTerm::subv; }

tDeclVars::tDeclVars(string id, int lineNo) :varsi({ { id, lineNo, nullptr } }), typedecl(mType::type_error) {}
tDeclVars::tDeclVars(string id, int lineNo, GrammarTree* inif) :varsi({ { id, lineNo, inif } }), typedecl(mType::type_error) {}
Op::NonTerm tDeclVars::type() const { return Op::NonTerm::vdecl; }
void tDeclVars::addVar(string id, int lineNo) { varsi.emplace_back(id, lineNo, nullptr); }
void tDeclVars::addVar(string id, int lineNo, GrammarTree* inif) { varsi.emplace_back(id, lineNo, inif); }
void tDeclVars::setDeclType(mType type) { this->typedecl = type; }

list<GrammarTree*>* tDeclVars::extractdecl(vector<mVar>& v) {
	auto l = new list<GrammarTree*>();
	try {
		for (auto s = varsi.rbegin(); s != varsi.rend(); s++) {
			auto&[var, lineNo, tree] = *s;
			auto same = accumulate(s + 1, varsi.rend(), false,
				[&str = var](bool b, tuple<string, int, GrammarTree*>& a) {
				return (get<0>(a) == str) || b; });
			if (same)
				throw(ErrVarRedeclared(lineNo, var));
			v.emplace(v.begin(), typedecl, var);
			if (tree != nullptr)
				l->push_back(tree);
		}
		return l;
	}
	catch (...) {
		delete l; throw;
	}
}

tNoVars::~tNoVars() {
	if (id == 8) { //stmt->goto id ;
		if (dynamic_cast<tTerminator*>(branchs[0]))
			delete branchs[0];
	}
	else if (id != 10 && id != 11) //stmt->break/continue ;
		for (auto i : branchs)
			delete i;
}

tNoVars::tNoVars(int id, int lineNo) :id(id), lineNo(lineNo), opID(0) {}
void tNoVars::changeid(int id) { this->id = id; }
Op::NonTerm tNoVars::type() const { return Op::Ch::ToType(id); }
void tNoVars::addTree(GrammarTree* t) { branchs.push_back(t); }

inline void extract_and_expand(GrammarTree*& tree, vector<mVar>& v) {
	//if tree is a "type vdecl ;", then we need to expand it to "{ stmts }"
	if (auto l = tree->extractdecl(v); l != nullptr) {
		delete tree;
		tree = new tStmts(*l);
		delete l;
	}
}

list<GrammarTree*>* tNoVars::extractdecl(vector<mVar>& v) {
	if (id == 3) {
		return branchs[0]->extractdecl(v);
	}
	else if (id == 9) {
		branchs[0]->extractdecl(v);
	}
	else if (id >= 5 && id <= 7) {
		extract_and_expand(branchs[0], v);
	}
	else if (id == 4) {
		extract_and_expand(branchs[0], v);
		extract_and_expand(branchs[1], v);
	}
	else if (id == 30) {
		extract_and_expand(branchs[1], v);
	}
	return nullptr;
}

mVType tNoVars::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	switch (id) {
	case 2: { //stmt->expr ;
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		//if (!mVType::canChangeTo(_type, VTYPE(Void, r)))
		//	throw(ErrTypeChangeLoss(branchs[2]->getLineNo(), _type, VTYPE(Void, r, false)));
		//不需要左值到右值转换
		//_type.valuetype = Op::LRvalue::rvalue;
		break; }
	case 3: //stmt->types vdecl ;
		throw(ErrDesignApp("GrammarTree::TypeCheck.3"));
	case 4: { //stmt->if ( expr ) stmt else stmt
		_type = branchs[2]->TypeCheck(sub, subs, whileBlock);
		if (Op::Rank rank = mVType::canChangeTo(_type, VTYPE(Int, r)); rank.incorrect())
			throw(ErrTypeChangeLoss(branchs[2]->getLineNo(), _type, VTYPE(Int, r)));
		else
			branchs[2] = branchs[2]->typeChange(rank);
		branchs[1]->TypeCheck(sub, subs, whileBlock);
		branchs[0]->TypeCheck(sub, subs, whileBlock);
		break;
	}
	case 5:	//stmt->if ( expr ) stmt
		[[fallthrough]];
	case 6:	//stmt->while ( expr ) stmt
		[[fallthrough]];
	case 7: { //stmt->for ( exprf ) stmt
		_type = branchs[1]->TypeCheck(sub, subs, whileBlock);
		if (Op::Rank rank = mVType::canChangeTo(_type, VTYPE(Int, r)); rank.incorrect())
			throw(ErrTypeChangeLoss(branchs[1]->getLineNo(), _type, VTYPE(Int, r)));
		else
			branchs[1] = branchs[1]->typeChange(rank);
		branchs[0]->TypeCheck(sub, subs, id == 5 ? whileBlock : this);
		break; }
	case 30: { //stmt->do stmt while ( expr )
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		if (Op::Rank rank = mVType::canChangeTo(_type, VTYPE(Int, r)); rank.incorrect())
			throw(ErrTypeChangeLoss(branchs[0]->getLineNo(), _type, VTYPE(Int, r)));
		else
			branchs[0] = branchs[0]->typeChange(rank);
		branchs[1]->TypeCheck(sub, subs, this);
		break; }
	case 8: { //stmt->goto id ;
		auto str = branchs[0]->getToken()->getId();
		auto ptr = sub->checkLabel(str);
		delete branchs[0];
		branchs[0] = ptr;
		_type = VTYPE(Void, r);
		break;
	}
	case 9: //stmt->{ stmts }
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		break;
	case 10: //stmt->break ;
		//如果没有while/for块则抛出异常
		if (whileBlock == nullptr)
			throw(ErrBreakWithoutWhile(lineNo, "break"));
		branchs.push_back(whileBlock);
		_type = VTYPE(Void, r);
		break;
	case 11: //stmt->continue ;
		if (whileBlock == nullptr)
			throw(ErrBreakWithoutWhile(lineNo, "continue"));
		branchs.push_back(whileBlock);
		_type = VTYPE(Void, r);
		break;
	case 29: { //stmt->thread id ( insv ) ;
		//要访问insv内部
		auto insv = static_cast<tNoVars*>(branchs[0]);
		auto name = branchs[1]->getToken()->getId();
		//此为函数调用,same as function
		if (auto[begin_it, end_it] = subs->checkSub(name); begin_it != end_it) {
			_type = OverloadCheck<void, decltype(begin_it), vector<GrammarTree*>&>(insv->branchs, [sub, subs, whileBlock](GrammarTree* tree) { return tree->TypeCheck(sub, subs, whileBlock); },
				[](const decltype(begin_it)& it) {
					vector<mVType> vTypes;
					transform(get<1>(it->second).cbegin(), get<1>(it->second).cend(), inserter(vTypes, vTypes.begin()),
						[](const mType& t) { return mVType{ t, Op::LRvalue::rvalue, false }; });
					return make_tuple((void*)nullptr, mVType{ get<0>(it->second), Op::LRvalue::rvalue, false }, vTypes); },
				[](Op::Rank rank, vector<GrammarTree*>::iterator& it) { *it = (*it)->typeChange(rank); },
				begin_it, end_it);
			if (_type.type == Op::mType::type_error)
				throw(ErrNoOverloadFunction(lineNo, name));
			break;
		}
		throw(ErrFuncNotFound(lineNo, name));
	}
	case 31: { //stmt->__rawins { data }
		//No type check!
		_type = VTYPE(Void, r);
		break;
	}
	case 12: //inia = vector<expr>
		throw(ErrDesignApp("GrammarTree::TypeCheck.12"));
	case 17: //ini->{ inia } inia = vector<expr>
		//TODO : 类型检查时直接由id = inif访问ini内部寻找
		_type = VTYPE(inilist, r);
		for (auto i : branchs)
			i->TypeCheck(sub, subs, whileBlock);
		break;
	case 19: { //insv = vector<exprf>
		//类型检查时直接由id(insv)访问insv内部寻找
		/*bool isLiteral = true;
		for (auto i : branchs)
			isLiteral = i->TypeCheck(sub, subs, whileBlock).isLiteral && isLiteral;
		_type = VTYPE(Void, r, isLiteral);
		break;*/
		throw(ErrDesignApp("GrammarTree::TypeCheck.19"));
	}
	case 14: //inif->ini
		[[fallthrough]];
	case 16: //inif->expr
		[[fallthrough]];
	case 20: //exprf->expr
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		break;
	case 15: //inif->ini : ini : ini : ini
		[[fallthrough]];
	case 21: { //exprf->expr : expr : expr : expr
		auto[begin_it, end_it] = Op::mVType::typeChange.equal_range(Op::TokenType::Colon);
		auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it), vector<GrammarTree*>&>(branchs,
			[sub, subs, whileBlock](GrammarTree* tree) { return tree->TypeCheck(sub, subs, whileBlock); },
			[](const decltype(begin_it)& it) { return make_tuple(new int(get<3>(it->second)), get<2>(it->second), vector<Op::mVType>{ get<0>(it->second), get<1>(it->second), get<1>(it->second), get<1>(it->second) }); },
			[](Op::Rank rank, vector<GrammarTree*>::iterator& it) { *it = (*it)->typeChange(rank); },
			begin_it, end_it);
		if (typ.type == Op::mType::type_error)
			throw(ErrNoOverloadFunction(lineNo, Op::Ch::ToString(Op::TokenType::Colon)));
		_type = typ; opID = *opIDPtr; delete opIDPtr;
		if constexpr (debug)clog << Op::Ch::ToString(Op::TokenType::Colon) << " ";
		break; }
	case 22: { //expr->id
		auto tok = branchs[0]->getToken();
		if (auto var = sub->checkVar(tok->getId()); var != nullptr) {
			_type = mVType{ var->type, Op::LRvalue::lvalue };
			if constexpr (debug)clog << tok->getId() << " ";
			break;
		}
		if (auto globalVarit = ReadIns::globalVariable.find(tok->getId()); globalVarit != ReadIns::globalVariable.end()) {
			opID = globalVarit->second.first;
			_type = mVType{ Op::Ch::ToType(globalVarit->second.second), Op::LRvalue::rvalue };
			if constexpr (debug)clog << tok->getId() << " " << opID << " ";
			break;
		}
		throw(ErrVarNotFound(tok->getlineNo(), tok->getId()));
	}
	case 23: { //expr->num
		if (branchs[0]->getToken()->getInt() != nullptr) {
			_type = VTYPE(Int, r, true);
			if constexpr (debug)clog << *branchs[0]->getToken()->getInt() << " ";
		}
		else if (branchs[0]->getToken()->getFloat() != nullptr) {
			_type = VTYPE(Float, r, true);
			if constexpr (debug)clog << *branchs[0]->getToken()->getFloat() << "f ";
		}
		else if (branchs[0]->getToken()->getString() != nullptr) {
			_type = VTYPE(String, r, true);
			if constexpr (debug)clog << "\"" << *branchs[0]->getToken()->getString() << "\" ";
		}
		break;
	}
	case 24: { //expr->id ( insv )
		//要访问insv内部
		auto insv = static_cast<tNoVars*>(branchs[0]);
		auto name = branchs[1]->getToken()->getId();
		//此为内置函数
		if (auto[begin_it, end_it] = Op::mVType::internalFunction.equal_range(name); begin_it != end_it) {
			bool isLiteral = true;
			auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it), vector<GrammarTree*>&>(insv->branchs,
				[sub, subs, whileBlock, &isLiteral](GrammarTree* tree) { auto type = tree->TypeCheck(sub, subs, whileBlock);
					isLiteral = isLiteral && type.isLiteral; return type; },
				[](const decltype(begin_it)& it) { return make_tuple(new int(get<2>(it->second)), get<0>(it->second), get<1>(it->second)); },
				[](Op::Rank rank, vector<GrammarTree*>::iterator& it) { *it = (*it)->typeChange(rank); },
					begin_it, end_it);
			if (typ.type == Op::mType::type_error)
				throw(ErrNoOverloadFunction(lineNo, name));
			_type = typ; opID = *opIDPtr; delete opIDPtr;
			this->id = 36;
			if (isLiteral)
				LiteralCal();
		}
		//此为函数调用,same as function
		else if (auto[begin_it, end_it] = subs->checkSub(name); begin_it != end_it) {
			_type = OverloadCheck<void, decltype(begin_it), vector<GrammarTree*>&>(insv->branchs, [sub, subs, whileBlock](GrammarTree* tree) { return tree->TypeCheck(sub, subs, whileBlock); },
				[](const decltype(begin_it)& it) {
				vector<mVType> vTypes;
				transform(get<1>(it->second).cbegin(), get<1>(it->second).cend(), inserter(vTypes, vTypes.begin()),
					[](const mType& t) { return mVType{ t, Op::LRvalue::rvalue, false }; });
				return make_tuple((void*)nullptr, mVType{ get<0>(it->second), Op::LRvalue::rvalue, false }, vTypes); },
				[](Op::Rank rank, vector<GrammarTree*>::iterator& it) { *it = (*it)->typeChange(rank); },
				begin_it, end_it);
			if (_type.type == Op::mType::type_error)
				throw(ErrNoOverloadFunction(lineNo, name));
		}
		//此为弹幕变换
		else if (auto[begin_it, end_it] = ReadIns::mode.equal_range(name); begin_it != end_it) {
			auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it), vector<GrammarTree*>&>(insv->branchs,
				[sub, subs, whileBlock](GrammarTree* tree) { return tree->TypeCheck(sub, subs, whileBlock); },
				[](const decltype(begin_it)& it) {
					vector<mVType> vTypes;
					transform(it->second.second.cbegin(), it->second.second.cend(), inserter(vTypes, vTypes.begin()),
						[](const ReadIns::NumType& t) { return mVType{ Op::Ch::ToType(t), Op::LRvalue::rvalue, false }; });
					return make_tuple(new int(get<0>(it->second)), VTYPE(Void, r), vTypes); },
				[](Op::Rank rank, vector<GrammarTree*>::iterator& it) { *it = (*it)->typeChange(rank); },
					begin_it, end_it);
			if (typ.type == Op::mType::type_error)
				throw(ErrNoOverloadFunction(lineNo, name));
			_type = VTYPE(Void, r); opID = *opIDPtr; delete opIDPtr;
			this->id = 35;
		}
		//此为ins调用
		else if (auto[begin_it, end_it] = ReadIns::ins.equal_range(name); begin_it != end_it) {
			//if "anything" or "str call" then no typecheck
			auto declTypes = begin_it->second.second;
			if (declTypes == decltype(declTypes){ ReadIns::NumType::String, ReadIns::NumType::Call }) {
				//TODO only check "str"
			}
			else if (declTypes != decltype(declTypes){ ReadIns::NumType::Anything }) {
				auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it), vector<GrammarTree*>&>(insv->branchs,
					[sub, subs, whileBlock](GrammarTree* tree) { return tree->TypeCheck(sub, subs, whileBlock); },
					[](const decltype(begin_it)& it) {
						vector<mVType> vTypes;
						transform(it->second.second.cbegin(), it->second.second.cend(), inserter(vTypes, vTypes.begin()),
							[](const ReadIns::NumType& t) { return mVType{ Op::Ch::ToType(t), Op::LRvalue::rvalue, false }; });
						return make_tuple(new int(it->second.first), VTYPE(Void, r), vTypes); },
					[](Op::Rank rank, vector<GrammarTree*>::iterator& it) { *it = (*it)->typeChange(rank); },
					begin_it, end_it);
				if (typ.type == Op::mType::type_error)
					throw(ErrNoOverloadFunction(lineNo, name));
			}
			_type = VTYPE(Void, r);
			this->id = 34;
		}
		else
			throw(ErrFuncNotFound(lineNo, name));
		if constexpr (debug)clog << "ins " << name << " ";
		break;
	}
	case 25: { //expr->Unary_op expr
		Op::TokenType tok = branchs[1]->getToken()->type();
		auto[begin_it, end_it] = Op::mVType::typeChange.equal_range(tok);
		auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it)>(vector<GrammarTree**>{ &branchs[0] },
			[sub, subs, whileBlock](GrammarTree** tree) { return (*tree)->TypeCheck(sub, subs, whileBlock); },
			[](const decltype(begin_it)& it) { return make_tuple(new int(get<3>(it->second)), get<2>(it->second), vector<Op::mVType>{ get<1>(it->second) }); },
			[](Op::Rank rank, vector<GrammarTree**>::iterator& it) { **it = (**it)->typeChange(rank); },
			begin_it, end_it);
		if (typ.type == Op::mType::type_error)
			throw(ErrNoOverloadFunction(lineNo, Op::Ch::ToString(tok)));
		_type = typ; opID = *opIDPtr; delete opIDPtr;
		if (static_cast<tNoVars*>(branchs[0])->get_mVType().isLiteral)
			LiteralCal();
		if constexpr (debug)clog << Op::Ch::ToString(tok) << " ";
		break; }
	case 26: { //expr->expr Binary_op expr/exprf/inif
		//TODO: for point = inilist, check things in inilist
		auto tok = branchs[1]->getToken()->type();
		auto[begin_it, end_it] = Op::mVType::typeChange.equal_range(tok);
		auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it)>(vector<GrammarTree**>{ &branchs[0], &branchs[2] } ,
			[sub, subs, whileBlock](GrammarTree** tree) { return (*tree)->TypeCheck(sub, subs, whileBlock); },
			[](const decltype(begin_it)& it) { return make_tuple(new int(get<3>(it->second)), get<2>(it->second), vector<Op::mVType>{ get<1>(it->second), get<0>(it->second) }); },
			[](Op::Rank rank, vector<GrammarTree**>::iterator& it) { **it = (**it)->typeChange(rank); },
			begin_it, end_it);
		if (typ.type == Op::mType::type_error)
			throw(ErrNoOverloadFunction(lineNo, Op::Ch::ToString(tok)));
		_type = typ; opID = *opIDPtr; delete opIDPtr;
		if (static_cast<tNoVars*>(branchs[0])->get_mVType().isLiteral && static_cast<tNoVars*>(branchs[2])->get_mVType().isLiteral)
			LiteralCal();
		if constexpr (debug)clog << Op::Ch::ToString(tok) << " ";
		break; }
	case 27: { //expr->expr [ expr ]
		auto[begin_it, end_it] = Op::mVType::typeChange.equal_range(Op::TokenType::MidBra);
		auto[typ, opIDPtr] = OverloadCheck<int, decltype(begin_it)>(vector<GrammarTree**>{ &branchs[0], &branchs[2] },
			[sub, subs, whileBlock](GrammarTree** tree) { return (*tree)->TypeCheck(sub, subs, whileBlock); },
			[](const decltype(begin_it)& it) { return make_tuple(new int(get<3>(it->second)), get<2>(it->second), vector<Op::mVType>{ get<1>(it->second), get<0>(it->second) }); },
			[](Op::Rank rank, vector<GrammarTree**>::iterator& it) { **it = (**it)->typeChange(rank); },
			begin_it, end_it);
		if (typ.type == Op::mType::type_error)
			throw(ErrNoOverloadFunction(lineNo, Op::Ch::ToString(Op::TokenType::MidBra)));
		_type = typ; opID = *opIDPtr; delete opIDPtr;
		if (static_cast<tNoVars*>(branchs[0])->get_mVType().isLiteral && static_cast<tNoVars*>(branchs[2])->get_mVType().isLiteral)
			LiteralCal();
		if constexpr (debug)clog << Op::Ch::ToString(Op::TokenType::MidBra) << " ";
		break; }
		break;
	case 28: { //expr->( types ) expr
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		if constexpr (debug) clog << "typechange from " << _type.debug_out() << " ";
		_type = mVType{ branchs[1]->getType(), Op::rvalue, false };
		branchs[0]->typeChange(Op::Rank().set(Op::Rank::LTOR));
		break; }
	default:
		throw(ErrDesignApp("GrammarTree::TypeCheck.unknown"));
	}
	if constexpr (debug) clog << "tNoVars " << id << " type " << _type.debug_out() << endl;
	return _type;
}

void tNoVars::extractlabel(map<string, GrammarTree*>& labels) {
	if (id == 4) {
		branchs[0]->extractlabel(labels);
		branchs[1]->extractlabel(labels);
	}
	if (id >= 5 && id <= 7) branchs[0]->extractlabel(labels);
	if (id == 30) branchs[1]->extractlabel(labels);
	if (id == 9) branchs[0]->extractlabel(labels);
}
int tNoVars::getLineNo() const { return lineNo; }

GrammarTree* tNoVars::typeChange(Op::Rank rank) {
	if (rank.test(Op::Rank::LTOR)) {
		//左值到右值直接赋值进去
		_type.valuetype = Op::LRvalue::rvalue;
	}
	if (rank.test(Op::Rank::ITOF)) {
		//整数转浮点
		if (_type.isLiteral) {
			//如果是字面量，说明一定是expr->num
			branchs[0] = branchs[0]->typeChange(rank);
			_type.type = TYPE(Float);
			return this;
		}
		return new tNoVars(VTYPE(Float, r, _type.isLiteral), 28, -1, this, new tTerminator(new Token_KeywordType(-1, Op::mType::Float)));
	}
	//void转other，并且一定是(*ptr)，赋值进去
	if (rank.test(Op::Rank::VTOI)) {
		_type.type = TYPE(Int);
		static_cast<tNoVars*>(branchs[0])->_type.type = TYPE(Int);
		if (rank.test(Op::Rank::LTOR)) {
			//(*ptr)，赋值进去
			static_cast<tNoVars*>(branchs[0])->_type.valuetype = Op::LRvalue::rvalue;
		}
	}
	else if (rank.test(Op::Rank::VTOF)) {
		_type.type = TYPE(Float);
		static_cast<tNoVars*>(branchs[0])->_type.type = TYPE(Float);
		if (rank.test(Op::Rank::LTOR)) {
			//(*ptr)，赋值进去
			static_cast<tNoVars*>(branchs[0])->_type.valuetype = Op::LRvalue::rvalue;
		}
	}
	else if (rank.test(Op::Rank::VTOSTR)) {
		_type.type = TYPE(String);
		static_cast<tNoVars*>(branchs[0])->_type.type = TYPE(String);
		if (rank.test(Op::Rank::LTOR)) {
			//(*ptr)，赋值进去
			static_cast<tNoVars*>(branchs[0])->_type.valuetype = Op::LRvalue::rvalue;
		}
	}
	else if (rank.test(Op::Rank::VTOPOINT)) {
		_type.type = TYPE(Point);
		static_cast<tNoVars*>(branchs[0])->_type.type = TYPE(Point);
		if (rank.test(Op::Rank::LTOR)) {
			//(*ptr)，赋值进去
			static_cast<tNoVars*>(branchs[0])->_type.valuetype = Op::LRvalue::rvalue;
		}
	}
	return this;
}

mVType tNoVars::get_mVType() const { return this->_type; }

void tNoVars::LiteralCal() {
	tTerminator* tree = nullptr;
	if (id == 36) {
		auto insv = static_cast<tNoVars*>(branchs[0]);
#define GET(tree, type) static_cast<tNoVars*>(static_cast<tNoVars*>(insv->branchs[tree])->branchs[0])->branchs[0]->getToken()->get##type()
#define TERM(number) new tTerminator(new Token_Literal(insv->getLineNo(), (number)))
		switch (opID) {
		case 79:
			//sin
			tree = TERM(sin(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 80:
			//cos
			tree = TERM(cos(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2013:
			//deg
			tree = TERM(*GET(0, Float) * 180 / 3.14159265f);
			_type = VTYPE(Float, r, true);
			break;
		case 2014:
			//rad
			tree = TERM(*GET(0, Float) * 3.14159265f / 180);
			_type = VTYPE(Float, r, true);
			break;
		case 2015:
			//ln
			tree = TERM(log(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2016:
			//log
			tree = TERM(log10(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2017:
			//pow float
			tree = TERM((int)pow(*GET(1, Float), *GET(0, Int)));
			_type = VTYPE(Float, r, true);
			break;
		case 2018:
			//pow int
			tree = TERM((int)pow(*GET(1, Int), *GET(0, Int)));
			_type = VTYPE(Int, r, true);
			break;
		case 2019:
			//sgn float
			{
				float temp = *GET(0, Float);
				tree = TERM(temp > 0 ? 1 : temp == 0 ? 0 : -1);
			}
			_type = VTYPE(Float, r, true);
			break;
		case 2020:
			//sgn int
			{
				int temp = *GET(0, Int);
				tree = TERM(temp > 0 ? 1 : temp == 0 ? 0 : -1);
			}
			_type = VTYPE(Float, r, true);
			break;
		case 2021:
			//tan
			tree = TERM(tan(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2022:
			//asin
			tree = TERM(asin(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2023:
			//acos
			tree = TERM(acos(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2024:
			//atan
			tree = TERM(atan(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2026:
			//sar
			tree = TERM(int(unsigned int(*GET(1, Int)) >> *GET(0, Int)));
			_type = VTYPE(Int, r, true);
			break;
		case 2027:
			//shr
			tree = TERM(*GET(1, Int) >> *GET(0, Int));
			_type = VTYPE(Int, r, true);
			break;
		case 2028:
			//shr
			tree = TERM(*GET(1, Int) << *GET(0, Int));
			_type = VTYPE(Int, r, true);
			break;
		case 2033:
			//abs float
			tree = TERM(abs(*GET(0, Float)));
			_type = VTYPE(Float, r, true);
			break;
		case 2032:
			//abs int
			tree = TERM(abs(*GET(0, Int)));
			_type = VTYPE(Int, r, true);
			break;
		default:
			break;
		}
		id = 23;
		for (auto ptr : branchs)
			delete ptr;
		branchs.clear();
		branchs.push_back(tree);
#undef GET
#undef TERM
	}
	else {
#define GET(tree, type) static_cast<tNoVars*>(branchs[tree])->branchs[0]->getToken()->get##type()
#define TERM(number) new tTerminator(new Token_Literal(branchs[0]->getLineNo(), (int)(number)))
		int change = 0;		//0：不变 1：删除1个并改id 2：删除2个并改id 3：删除3个，赋值tree，改id
		using Op::TokenType;
		switch (opID) {
		case TokenType::Plus:
			//int & int = int
			*GET(2, Int) += *GET(0, Int);
			change = 2;
			break;
		case TokenType::Plus + OFFSET:
			//float & float = float
			*GET(2, Float) += *GET(0, Float);
			change = 2;
			break;
		case TokenType::Plus + 2 * OFFSET:
			//point & point = point
			throw(ErrDesignApp("Point literal + Point literal"));
		case TokenType::Plus + 3 * OFFSET:
			//string & string = string
			*GET(2, String) += *GET(0, String);
			change = 2;
			break;
		case TokenType::Minus:
			//int & int = int
			*GET(2, Int) -= *GET(0, Int);
			change = 2;
			break;
		case TokenType::Minus + OFFSET:
			//float & float = float
			*GET(2, Float) -= *GET(0, Float);
			change = 2;
			break;
		case TokenType::Minus + 2 * OFFSET:
			//point & point = point
			throw(ErrDesignApp("Point literal - Point literal"));
		case TokenType::Times:
			//int & int = int
			*GET(2, Int) *= *GET(0, Int);
			change = 2;
			break;
		case TokenType::Times + OFFSET:
			//float & float = float
			*GET(2, Float) *= *GET(0, Float);
			change = 2;
			break;
		case TokenType::Times + 2 * OFFSET:
			//point & float = point
			throw(ErrDesignApp("Point literal * Float literal"));
		case TokenType::Times + 3 * OFFSET:
			//float & point = point
			throw(ErrDesignApp("Float literal * Point literal"));
		case TokenType::Divide:
			*GET(2, Int) /= *GET(0, Int);
			change = 2;
			break;
		case TokenType::Divide + OFFSET:
			//float & float = float
			*GET(2, Float) /= *GET(0, Float);
			change = 2;
			break;
		case TokenType::Divide + 2 * OFFSET:
			//point & float = point
			throw(ErrDesignApp("Point literal / Float literal"));
		case TokenType::Mod:
			*GET(2, Int) %= *GET(0, Int);
			change = 2;
			break;
		case TokenType::Negative:
			//int = int
			*GET(0, Int) = -*GET(0, Int);
			change = 1;
			break;
		case TokenType::Negative + OFFSET:
			//float = float
			*GET(0, Float) = -*GET(0, Float);
			change = 1;
			break;
		case TokenType::Negative + 2 * OFFSET:
			//point = point
			throw(ErrDesignApp("(-) Point literal"));
		case TokenType::Not:
			*GET(0, Int) = !*GET(0, Int);
			change = 1;
			break;
		case TokenType::Not + OFFSET:
			//float = float
			*GET(0, Float) = !*GET(0, Float);
			change = 1;
			break;
		case TokenType::Not + 2 * OFFSET:
			//point = point
			throw(ErrDesignApp("(!) Point literal"));
		case TokenType::BitOr:
			[[fallthrough]];
		case TokenType::LogicalOr:
			[[fallthrough]];
		case TokenType::Or:
			*GET(2, Int) |= *GET(0, Int);
			change = 2;
			break;
		case TokenType::BitAnd:
			[[fallthrough]];
		case TokenType::LogicalAnd:
			[[fallthrough]];
		case TokenType::And:
			*GET(2, Int) &= *GET(0, Int);
			change = 2;
			break;
		case TokenType::BitXor:
			*GET(2, Int) ^= *GET(0, Int);
			change = 2;
			break;
		case TokenType::EqualTo:
			tree = TERM(*GET(2, Int) == *GET(0, Int));
			change = 3;
			break;
		case TokenType::NotEqual:
			tree = TERM(*GET(2, Int) != *GET(0, Int));
			change = 3;
			break;
		case TokenType::Greater:
			tree = TERM(*GET(2, Int) > *GET(0, Int));
			change = 3;
			break;
		case TokenType::GreaterEqual:
			tree = TERM(*GET(2, Int) >= *GET(0, Int));
			change = 3;
			break;
		case TokenType::Less:
			tree = TERM(*GET(2, Int) < *GET(0, Int));
			change = 3;
			break;
		case TokenType::LessEqual:
			tree = TERM(*GET(2, Int) <= *GET(0, Int));
			change = 3;
			break;
		case TokenType::EqualTo + OFFSET:
			tree = TERM(*GET(2, Float) == *GET(0, Float));
			change = 3;
			break;
		case TokenType::NotEqual + OFFSET:
			tree = TERM(*GET(2, Float) != *GET(0, Float));
			change = 3;
			break;
		case TokenType::Greater + OFFSET:
			tree = TERM(*GET(2, Float) > *GET(0, Float));
			change = 3;
			break;
		case TokenType::GreaterEqual + OFFSET:
			tree = TERM(*GET(2, Float) >= *GET(0, Float));
			change = 3;
			break;
		case TokenType::Less + OFFSET:
			tree = TERM(*GET(2, Float) < *GET(0, Float));
			change = 3;
			break;
		case TokenType::LessEqual + OFFSET:
			tree = TERM(*GET(2, Float) <= *GET(0, Float));
			change = 3;
			break;
		case TokenType::EqualTo + 2 * OFFSET:
			[[fallthrough]];
		case TokenType::NotEqual + 2 * OFFSET:
			[[fallthrough]];
		case TokenType::Greater + 2 * OFFSET:
			[[fallthrough]];
		case TokenType::GreaterEqual + 2 * OFFSET:
			[[fallthrough]];
		case TokenType::Less + 2 * OFFSET:
			[[fallthrough]];
		case TokenType::LessEqual + 2 * OFFSET:
			throw(ErrDesignApp("Point literal logical operator Point literal"));
		case TokenType::EqualTo + 3 * OFFSET:
			tree = TERM(*GET(2, String) == *GET(0, String));
			change = 3;
			break;
		case TokenType::NotEqual + 3 * OFFSET:
			tree = TERM(*GET(2, String) != *GET(0, String));
			change = 3;
			break;
		case TokenType::Greater + 3 * OFFSET:
			tree = TERM(*GET(2, String) > *GET(0, String));
			change = 3;
			break;
		case TokenType::GreaterEqual + 3 * OFFSET:
			tree = TERM(*GET(2, String) >= *GET(0, String));
			change = 3;
			break;
		case TokenType::Less + 3 * OFFSET:
			tree = TERM(*GET(2, String) < *GET(0, String));
			change = 3;
			break;
		case TokenType::LessEqual + 3 * OFFSET:
			tree = TERM(*GET(2, String) <= *GET(0, String));
			change = 3;
			break;
		case TokenType::Dot:
			throw(ErrDesignApp("LiteralCal->Dot"));
		default:
			break;
		}
		if (change == 1) {
			id = 23;
			auto tree2 = static_cast<tNoVars*>(branchs[0])->branchs[0];
			static_cast<tNoVars*>(branchs[0])->branchs[0] = nullptr;
			delete branchs[0], branchs[1];
			branchs.pop_back();
			branchs[0] = tree2;
			_type.isLiteral = true;
		}
		else if (change == 2) {
			id = 23;
			auto tree2 = static_cast<tNoVars*>(branchs[2])->branchs[0];
			static_cast<tNoVars*>(branchs[2])->branchs[0] = nullptr;
			delete branchs[0], branchs[1], branchs[2];
			branchs.pop_back(); branchs.pop_back();
			branchs[0] = tree2;
			_type.isLiteral = true;
		}
		else if (change == 3) {
			id = 23;
			delete branchs[0], branchs[1], branchs[2];
			branchs.pop_back(); branchs.pop_back();
			_type = VTYPE(Int, r, true);
			branchs[0] = tree;
		}
#undef GET
#undef TERM
	}
}

tLabel::~tLabel() {}
tLabel::tLabel(const string& name, int lineNo) :name(name), lineNo(lineNo) {}
Op::NonTerm tLabel::type() const { return Op::NonTerm::stmt; }
mVType tLabel::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	if constexpr (debug) clog << "tLabel type" << endl;
	return mVType();
}
void tLabel::extractlabel(map<string, GrammarTree*>& l) {
	l.emplace(name, this);
}
bool tLabel::isLabel() const { return true; }
string tLabel::getName() const { return this->name; }

tStmts::tStmts(list<GrammarTree*>& branchs) :branchs(move(branchs)) {}
tStmts::~tStmts() { for (auto i : branchs) delete i; }
Op::NonTerm tStmts::type() const { return Op::NonTerm::stmts; }
//void tStmts::insertlabel(string s, int lineNo, GrammarTree* t) { labels.push_back(make_tuple(s, lineNo, t)); }
void tStmts::addTree(GrammarTree* t) { branchs.push_back(t); }

list<GrammarTree*>* tStmts::extractdecl(vector<mVar>& v) {
	for (auto it = branchs.begin(); it != branchs.end();) {
		if (auto t = (*it)->extractdecl(v); t != nullptr) {
			move(t->begin(), t->end(), inserter(branchs, it));
			delete *it;
			it = branchs.erase(it);
			delete t;
		}
		else {
			++it;
		}
	}
	return nullptr;
}

mVType tStmts::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	if constexpr (debug) clog << "tStmts type" << endl;
	for (auto i : branchs)
		i->TypeCheck(sub, subs, whileBlock);
	return VTYPE(Void, r);
}

void tStmts::extractlabel(map<string, GrammarTree*>& l) {
	/*for (auto p : labels) {
		auto&[label, lineNo, tree] = p;
		auto x = l.insert(make_pair(label, tree));
		if (!x.second)
			throw(ErrLabelRedefined(lineNo, label));
	}*/
	for (auto t : branchs)
		t->extractlabel(l);
}

tSub::tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts, bool no_overload) :stmts(stmts), vardecl(subv->vars), name(name), lineNo(lineNo), no_overload(no_overload) {
	transform(subv->vars.begin(), subv->vars.end(), inserter(varpara, varpara.end()), [](const mVar& t) { return t.type; });
	delete subv;
	stmts->extractdecl(vardecl);
	typeReturn = Op::mType::Void;		//返回值为void
	stmts->extractlabel(labels);
}
tSub::~tSub() { delete stmts; }

mVType tSub::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	if constexpr (debug) clog << "tSub type" << endl;
	stmts->TypeCheck(this, subs, nullptr);
	return VTYPE(Void, r);
}

void tSub::insertDecl(multimap<string, tuple<mType, vector<mType>, bool>>& m) const {
	if (no_overload)
		if (m.find(name) != m.end())
			throw(ErrFuncRedeclared(lineNo, name));
		else
			m.insert(make_pair(name, make_tuple(typeReturn, varpara, true)));
	else {
		auto[it_begin, it_end] = m.equal_range(name);
		for (; it_begin != it_end; ++it_begin)
			if (get<2>(it_begin->second) || get<1>(it_begin->second) == varpara)
				throw(ErrFuncRedeclared(lineNo, name));
		m.insert(make_pair(name, make_tuple(typeReturn, varpara, false)));
	}
}

mVar* tSub::checkVar(const string& id) {
	auto itpara = find_if(vardecl.begin(), vardecl.end(), [&id](const mVar& a) { return a.id == id; });
	if (itpara == vardecl.end())
		return nullptr;
	return &(*itpara);
}

GrammarTree* tSub::checkLabel(const string& id) {
	auto it = labels.find(id);
	if (it == labels.end())
		return nullptr;
	return it->second;
}
int tSub::getLineNo() const { return lineNo; }

tRoot::~tRoot() { for (auto i : subs) delete i; }
Op::NonTerm tRoot::type() const { return Op::NonTerm::subs; }

void tRoot::addSub(tSub* s) {
	subs.push_back(s);
	s->insertDecl(subdecl);
}

void tRoot::addSubDecl(string name, int lineNo, tSubVars* subv, mType typeReturn, bool no_overload) {
	vector<mType> varpara;
	transform(subv->vars.begin(), subv->vars.end(), inserter(varpara, varpara.end()), [](const mVar& t) { return t.type; });
	delete subv;
	if (no_overload)
		if (subdecl.find(name) != subdecl.end())
			throw(ErrFuncRedeclared(lineNo, name));
		else
			subdecl.insert(make_pair(name, make_tuple(typeReturn, varpara, true)));
	else {
		auto[it_begin, it_end] = subdecl.equal_range(name);
		for (; it_begin != it_end; ++it_begin)
			if (get<2>(it_begin->second) || get<1>(it_begin->second) == varpara)
				throw(ErrFuncRedeclared(lineNo, name));
		subdecl.insert(make_pair(name, make_tuple(typeReturn, varpara, false)));
	}
}

decltype(declval<multimap<string, tuple<mType, vector<mType>, bool>>>().equal_range(declval<string>())) tRoot::checkSub(const string& id) {
	return subdecl.equal_range(id);
}

mVType tRoot::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	for (auto i : this->subs)
		i->TypeCheck(nullptr, this, nullptr);
	return VTYPE(Void, r);
}

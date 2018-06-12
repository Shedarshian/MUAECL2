#include "stdafx.h"
#include "GrammarTree.h"

using namespace std;

void GrammarTree::changeid(int id) { throw(ErrDesignApp("GrammarTree::changeid")); }
Op::NonTerm GrammarTree::type() { throw(ErrDesignApp("GrammarTree::type")); }
int GrammarTree::state() { throw(ErrDesignApp("GrammarTree::state")); }
void GrammarTree::addTree(GrammarTree* t) { throw(ErrDesignApp("GrammarTree::addTree")); }
list<GrammarTree*>* GrammarTree::extractdecl(vector<mVar>& v) { return nullptr; }
void GrammarTree::extractlabel(map<string, GrammarTree*>& labels) {};
Token* GrammarTree::getToken() { throw(ErrDesignApp("GrammarTree::getToken")); }
mType* GrammarTree::getType() { throw(ErrDesignApp("GrammarTree::getType")); }
int GrammarTree::getLineNo() { throw(ErrDesignApp("GrammarTree::getLineNo")); }
mVType GrammarTree::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) { throw(ErrDesignApp("GrammarTree::TypeCheck")); }
GrammarTree* GrammarTree::typeChange(int rank) { throw(ErrDesignApp("GrammarTree::typeChange")); }

tState::tState(int state) :_state(state) {}
int tState::state() { return _state; }

tTerminator::tTerminator(Token* t) :t(t) {}
tTerminator::~tTerminator() { delete t; }
Token* tTerminator::getToken() { return t; }
GrammarTree* tTerminator::typeChange(int rank) {
	//一定是右值
	if (rank == mVType::ITOF) {
		//整数转浮点
		Token* tok = new Token_Literal(t->getlineNo(), (float)*t->getInt());
		delete t;
		t = tok;
		return this;
	}
	else if (rank == mVType::ITOVP) {
		return new tNoVars(mVType{ TYPE(Void)->address(), Op::LRvalue::rvalue, true }, 28, -1, this, new tType(TYPE(Void)->address()));
	}
	else if (rank == mVType::PTOVP) {
		return new tNoVars(mVType{ TYPE(Void)->address(), Op::LRvalue::rvalue, true }, 28, -1, this, new tType(TYPE(Void)->address()));
	}
	return this;
}

tType::tType(Op::BuiltInType t) :t(Op::BuiltInTypeObj(t)) {}
tType::tType(mType* t) : t(t) {}
Op::NonTerm tType::type() { return Op::NonTerm::types; }
mType* tType::getType() { return t; }
void tType::makePointer() { t = t->address(); }

tSubVars::tSubVars(mType* type, string id) { vars.emplace_back(type, id); }

void tSubVars::emplaceVar(mType* type, string id, int lineNo) {
	auto same = accumulate(vars.begin(), vars.end(), false,
		[&str = id](bool b, mVar& a) {
			return (a.id == str) || b; });
	if (same)
		throw(ErrVarRedeclared(lineNo, id));
	vars.emplace_back(type, id);
}

Op::NonTerm tSubVars::type() { return Op::NonTerm::subv; }

tDeclVars::tDeclVars(string id, int lineNo) :varsi({ { id, lineNo, nullptr } }), typedecl(nullptr) {}
tDeclVars::tDeclVars(string id, int lineNo, GrammarTree* inif) :varsi({ { id, lineNo, inif } }), typedecl(nullptr) {}
tDeclVars::~tDeclVars() { for (auto i : varsi) delete get<2>(i); }
Op::NonTerm tDeclVars::type() { return Op::NonTerm::vdecl; }
void tDeclVars::addVar(string id, int lineNo) { varsi.emplace_back(id, lineNo, nullptr); }
void tDeclVars::addVar(string id, int lineNo, GrammarTree* inif) { varsi.emplace_back(id, lineNo, inif); }
void tDeclVars::setDeclType(mType* type) { this->typedecl = type; }

list<GrammarTree*>* tDeclVars::extractdecl(vector<mVar>& v) {
	auto l = new list<GrammarTree*>();
	try {
		for (auto s : varsi) {
			auto&[var, lineNo, tree] = s;
			auto same = accumulate(varsi.begin(), varsi.end(), false,
				[&str = var](bool b, tuple<string, int, GrammarTree*>& a) {
				return (get<0>(a) == str) || b; });
			if (same)
				throw(ErrVarRedeclared(lineNo, var));
			v.emplace_back(typedecl, var);
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

tNoVars::tNoVars(int id, int lineNo) :id(id), lineNo(lineNo) {}
void tNoVars::changeid(int id) { this->id = id; }
Op::NonTerm tNoVars::type() { return Op::Ch::ToType(id); }
void tNoVars::addTree(GrammarTree* t) { branchs.push_back(t); }

list<GrammarTree*>* tNoVars::extractdecl(vector<mVar>& v) {
	if (id == 3) {
		auto x = branchs[0]->extractdecl(v);
		delete branchs[0];
		return x;
	}
	if (id >= 5 && id <= 7 || id == 9)
		branchs[0]->extractdecl(v);
	if (id == 18) {
		branchs[0]->extractdecl(v);
		branchs[1]->extractdecl(v);
	}
	return nullptr;
}

mVType tNoVars::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	switch (id) {
	case 2: { //stmt->expr ;
		//需要可转成void右值非字面量，不允许弃值表达式。由于没有可以隐式转成void右值的实际表达式故不添加隐式转换节点
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		auto typeList = mVType::canChangeTo(_type);
		bool b = false;
		for (auto i : typeList)
			if (b = (i.first == VTYPE(Void, r, false)))
				break;
		if (!b)
			throw(ErrTypeChangeLoss(branchs[2]->getLineNo(), _type, VTYPE(Void, r, false)));
		//不需要左值到右值转换
		//_type.valuetype = Op::LRvalue::rvalue;
		break; }
	case 3: //stmt->types vdecl ;
		throw(ErrDesignApp("GrammarTree::TypeCheck.3"));
	case 4: { //stmt->if ( expr ) stmt else stmt
		_type = branchs[2]->TypeCheck(sub, subs, whileBlock);
		auto typeList = mVType::canChangeTo(_type);
		bool b = false;
		for (auto i : typeList)
			if (b = (i.first == VTYPE(Int, r))) {
				branchs[2] = branchs[2]->typeChange(i.second);
				break;
			}
		if (!b)
			throw(ErrTypeChangeLoss(branchs[2]->getLineNo(), _type, VTYPE(Int, r)));
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
		auto typeList = mVType::canChangeTo(_type);
		bool b = false;
		bool b = false;
		for (auto i : typeList)
			if (b = (i.first == VTYPE(Int, r))) {
				branchs[1] = branchs[1]->typeChange(i.second);
				break;
			}
		if (!b)
			throw ErrTypeChangeLoss(branchs[1]->getLineNo(), _type, VTYPE(Int, r));
		branchs[0]->TypeCheck(sub, subs, whileBlock);
		break; }
	case 8: { //stmt->goto id ;
		auto str = branchs[0]->getToken()->getString();
		auto ptr = sub->checkLabel(*str);
		delete branchs[0];
		branchs[0] = ptr;
		break;
	}
	case 9: //stmt->{ stmts }
		return branchs[0]->TypeCheck(sub, subs, whileBlock);
	case 10: //stmt->break ;
		//如果没有while/for块则抛出异常
		if (whileBlock == nullptr)
			throw(ErrBreakWithoutWhile(lineNo, "break"));
		branchs.push_back(whileBlock);
		break;
	case 11: //stmt->continue ;
		if (whileBlock == nullptr)
			throw(ErrBreakWithoutWhile(lineNo, "continue"));
		branchs.push_back(whileBlock);
		break;
	case 12: //inia = vector<expr>
		throw(ErrDesignApp("GrammarTree::TypeCheck.12"));
	case 17: { //ini->{ inia } inia = vector<expr>
		//类型检查时直接由id = inif访问ini内部寻找???
		//TODO
		break;
	}
	case 19: { //insv = vector<exprf>
		//类型检查时直接由id(insv)访问insv内部寻找
		for (auto i : branchs)
			i->TypeCheck(sub, subs, whileBlock);
		break;
	}
	case 14: //inif->ini
		[[fallthrough]];
	case 16: //inif->expr
		[[fallthrough]];
	case 20: //exprf->expr
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		break;
	case 15: { //inif->ini : ini : ini : ini
		//TODO
		break;
	}
	case 21: { //exprf->expr : expr : expr : expr
		//TODO
		break;
	}
	case 22: { //expr->id
		auto tok = branchs[0]->getToken();
		auto var = sub->checkVar(tok->getId());
		if (var == nullptr)
			throw(ErrVarNotFound(tok->getlineNo(), tok->getId()));
		_type = mVType{ var->type, Op::LRvalue::lvalue };
		break;
	}
	case 23: { //expr->num
		if (branchs[0]->getToken()->getInt() != nullptr)
			_type = mVType{ Op::BuiltInTypeObj(Op::BuiltInType::Int), Op::LRvalue::rvalue, true };
		else if (branchs[0]->getToken()->getFloat() != nullptr)
			_type = mVType{ Op::BuiltInTypeObj(Op::BuiltInType::Float), Op::LRvalue::rvalue, true };
		else if (branchs[0]->getToken()->getString() != nullptr)
			_type = mVType{ Op::BuiltInTypeObj(Op::BuiltInType::String), Op::LRvalue::rvalue, true };
		break;
	}
	case 24: { //expr->id ( insv )
		//要访问insv内部
		//TODO
		break;
	}
	case 25: //expr->Unary_op expr
		[[fallthrough]];
	case 26: { //expr->expr Binary_op expr/exprf/inif
		auto ltype = branchs[0]->TypeCheck(sub, subs, whileBlock);
		decltype(ltype) rtype;
		auto ltypeList = mVType::canChangeTo(ltype);
		decltype(ltypeList) rtypeList = decltype(ltypeList){ make_pair(rtype, 0) };
		bool isLiteral = ltype.isLiteral;
		//如果是一元运算符，则rtype为初始对象，rtypeList为只有一个对象的列表
		if (branchs.size() == 3) {
			rtype = branchs[2]->TypeCheck(sub, subs, whileBlock);
			isLiteral = isLiteral && rtype.isLiteral;
			rtypeList = mVType::canChangeTo(rtype);
		}
		auto typ = branchs[1]->getToken()->type();
		auto[low, up] = mVType::typeChange.equal_range(typ);
		int minID = INT_MAX;
		int lrank, rrank;
		for (; low != up; ++low) {
			pair<mVType, int> lchgtype = { mVType(), INT_MAX }, rchgtype = { mVType(), INT_MAX };
			auto &[first, second, type, opID] = low->second;
			for (auto &i : ltypeList)
				if (first(i.first, rchgtype.first) && lchgtype.second < i.second)
					lchgtype = i;
			if (lchgtype.second == INT_MAX)
				continue;
			for (auto &j : rtypeList)
				if (second(j.first, lchgtype.first) && rchgtype.second < j.second)
					rchgtype = j;
			if (rchgtype.second == INT_MAX)
				continue;
			if (minID < lchgtype.second + rchgtype.second) {
				minID = lchgtype.second + rchgtype.second;
				_type = type(lchgtype.first, rchgtype.first); this->opID = opID;
				lrank = lchgtype.second; rrank = rchgtype.second;
			}
		}
		//无法应用运算符到此类型操作数
		if (minID == INT_MAX)
			throw(ErrTypeOperate(lineNo, ltype, rtype, branchs[1]->getToken()->debug_out()));
		//依据rank值确定做了些什么变换
		branchs[0] = branchs[0]->typeChange(lrank);
		if (branchs.size() == 3)
			branchs[2] = branchs[2]->typeChange(rrank);
		//处理字面量计算优化，需要排除字面量指针的情况（在里面处理）
		//需排除如void* literal dereference, void literal be assigned等等一系列情况 TODO
		if (isLiteral)
			LiteralCal();
		break;
		/*auto typeAllowed = mVType::typeChange.find(typ)->second;
		int minID = INT_MAX;
		int lrank, rrank;
		for(auto &i : ltypeList)
			for (auto &j : rtypeList) {
				auto _return = typeAllowed(i.first, j.first);
				//若不含值则说明不允许此类型对。并选取rank之和最小
				if (_return.has_value() && minID < i.second + j.second) {
					auto&[opID, retType] = _return.value();
					_type = retType; this->opID = opID;
					lrank = i.second; rrank = j.second;
				}
			}*/
	}
	case 27: { //expr->expr [ expr ]
		//TODO
		break;
	}
	case 28: { //expr->( types ) expr
		//TODO
		break;
	}
	default:
		throw(ErrDesignApp("GrammarTree::TypeCheck.unknown"));
	}
	return _type;
}

void tNoVars::extractlabel(map<string, GrammarTree*>& labels) {
	if (id == 9) branchs[0]->extractlabel(labels);
}
int tNoVars::getLineNo() { return lineNo; }

GrammarTree* tNoVars::typeChange(int rank) {
	if (rank % 3 == mVType::LTOR) {
		//左值到右值直接赋值进去
		_type.valuetype = Op::LRvalue::rvalue;
		rank -= mVType::LTOR;
	}
	if (rank == mVType::ITOF) {
		//整数转浮点
		return new tNoVars(VTYPE(Float, r, _type.isLiteral), 28, -1, this, new tType(Op::BuiltInType::Float));
	}
	else if (rank == mVType::ITOVP) {
		return new tNoVars(mVType{ TYPE(Void)->address(), Op::LRvalue::rvalue, _type.isLiteral }, 28, -1, this, new tType(TYPE(Void)->address()));
	}
	else if (rank == mVType::PTOVP) {
		return new tNoVars(mVType{ TYPE(Void)->address(), Op::LRvalue::rvalue, _type.isLiteral }, 28, -1, this, new tType(TYPE(Void)->address()));
	}
	return this;
}

void tNoVars::LiteralCal() {
#define OFFSET 128
#define GET(tree, type) branchs[tree]->getToken()->get##type()
#define TERM(number) new tTerminator(new Token_Literal(branchs[0]->getLineNo(), number))
	GrammarTree* tree;
	int change = 0;		//0：不变 1：删除1个并改id 2：删除2个并改id 3：删除3个，赋值tree，改id
	using Op::TokenType;
	switch (opID) {
	case TokenType::Plus:
		//int & int = int
		*GET(0, Int) += *GET(2, Int);
		change = 2;
		break;
	case TokenType::Plus + OFFSET:
		//float & float = float
		*GET(0, Float) += *GET(2, Float);
		change = 2;
		break;
	case TokenType::Plus + 2 * OFFSET:
		//point & point = point
		throw(ErrDesignApp("Point literal + Point literal"));
	case TokenType::Plus + 3 * OFFSET:
		//string & string = string
		*GET(0, String) += *GET(2, String);
		change = 2;
		break;
	case TokenType::Plus + 4 * OFFSET:
		//T* & int = T*
		[[fallthrough]];
	case TokenType::Plus + 5 * OFFSET:
		//int & T* = T*
		break;
	case TokenType::Minus:
		//int & int = int
		*GET(0, Int) -= *GET(2, Int);
		change = 2;
		break;
	case TokenType::Minus + OFFSET:
		//float & float = float
		*GET(0, Float) -= *GET(2, Float);
		change = 2;
		break;
	case TokenType::Minus + 2 * OFFSET:
		//point & point = point
		throw(ErrDesignApp("Point literal - Point literal"));
	case TokenType::Minus + 4 * OFFSET:
		//T* & int = T*
		[[fallthrough]];
	case TokenType::Times:
		//int & int = int
		*GET(0, Int) *= *GET(2, Int);
		change = 2;
		break;
	case TokenType::Times + OFFSET:
		//float & float = float
		*GET(0, Float) *= *GET(2, Float);
		change = 2;
		break;
	case TokenType::Times + 2 * OFFSET:
		//point & float = point
		throw(ErrDesignApp("Point literal * Float literal"));
	case TokenType::Times + 3 * OFFSET:
		//float & point = point
		throw(ErrDesignApp("Float literal * Point literal"));
	case TokenType::Divide:
		*GET(0, Int) /= *GET(2, Int);
		change = 2;
		break;
	case TokenType::Divide + OFFSET:
		//float & float = float
		*GET(0, Float) /= *GET(2, Float);
		change = 2;
		break;
	case TokenType::Divide + 2 * OFFSET:
		//point & float = point
		throw(ErrDesignApp("Point literal / Float literal"));
	case TokenType::Mod:
		*GET(0, Int) %= *GET(2, Int);
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
		*GET(0, Int) |= *GET(2, Int);
		change = 2;
		break;
	case TokenType::BitAnd:
		[[fallthrough]];
	case TokenType::LogicalAnd:
		[[fallthrough]];
	case TokenType::And:
		*GET(0, Int) &= *GET(2, Int);
		change = 2;
		break;
	case TokenType::BitXor:
		*GET(0, Int) ^= *GET(2, Int);
		change = 2;
		break;
	case TokenType::EqualTo:
		tree = TERM(*GET(0, Int) == *GET(2, Int));
		change = 3;
		break;
	case TokenType::NotEqual:
		tree = TERM(*GET(0, Int) != *GET(2, Int));
		change = 3;
		break;
	case TokenType::Greater:
		tree = TERM(*GET(0, Int) > *GET(2, Int));
		change = 3;
		break;
	case TokenType::GreaterEqual:
		tree = TERM(*GET(0, Int) >= *GET(2, Int));
		change = 3;
		break;
	case TokenType::Less:
		tree =TERM(*GET(0, Int) < *GET(2, Int));
		change = 3;
		break;
	case TokenType::LessEqual:
		tree = TERM(*GET(0, Int) <= *GET(2, Int));
		change = 3;
		break;
	case TokenType::EqualTo + OFFSET:
		tree = TERM(*GET(0, Float) == *GET(2, Float));
		change = 3;
		break;
	case TokenType::NotEqual + OFFSET:
		tree = TERM(*GET(0, Float) != *GET(2, Float));
		change = 3;
		break;
	case TokenType::Greater + OFFSET:
		tree = TERM(*GET(0, Float) > *GET(2, Float));
		change = 3;
		break;
	case TokenType::GreaterEqual + OFFSET:
		tree = TERM(*GET(0, Float) >= *GET(2, Float));
		change = 3;
		break;
	case TokenType::Less + OFFSET:
		tree = TERM(*GET(0, Float) < *GET(2, Float));
		change = 3;
		break;
	case TokenType::LessEqual + OFFSET:
		tree = TERM(*GET(0, Float) <= *GET(2, Float));
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
		tree = TERM(*GET(0, String) == *GET(2, String));
		change = 3;
		break;
	case TokenType::NotEqual + 3 * OFFSET:
		tree = TERM(*GET(0, String) != *GET(2, String));
		change = 3;
		break;
	case TokenType::Greater + 3 * OFFSET:
		tree = TERM(*GET(0, String) > *GET(2, String));
		change = 3;
		break;
	case TokenType::GreaterEqual + 3 * OFFSET:
		tree = TERM(*GET(0, String) >= *GET(2, String));
		change = 3;
		break;
	case TokenType::Less + 3 * OFFSET:
		tree = TERM(*GET(0, String) < *GET(2, String));
		change = 3;
		break;
	case TokenType::LessEqual + 3 * OFFSET:
		tree = TERM(*GET(0, String) <= *GET(2, String));
		change = 3;
		break;
	case TokenType::Dot:
		throw(ErrDesignApp("LiteralCal->Dot"));
	default:
		break;
	}
	if (change == 1) {
		id = 28;
		delete branchs[1];
		branchs.pop_back();
	}
	else if (change == 2) {
		id = 28;
		delete branchs[1], branchs[2];
		branchs.pop_back(); branchs.pop_back();
	}
	else if (change == 3) {
		id = 28;
		delete branchs[0], branchs[1], branchs[2];
		branchs.pop_back(); branchs.pop_back();
		branchs[0] = tree;
	}
#undef OFFSET
#undef GET
#undef TERM
}

tStmts::~tStmts() { for (auto i : branchs) delete i; }
Op::NonTerm tStmts::type() { return Op::NonTerm::stmts; }
void tStmts::insertlabel(string s, int lineNo, GrammarTree* t) { labels.push_back(make_tuple(s, lineNo, t)); }
void tStmts::addTree(GrammarTree* t) { branchs.push_back(t); }

list<GrammarTree*>* tStmts::extractdecl(vector<mVar>& v) {
	for (auto it = branchs.begin(); it != branchs.end(); ++it) {
		auto t = (*it)->extractdecl(v);
		if (t != nullptr)
			move(t->begin(), t->end(), inserter(branchs, it));
		delete *it;
		branchs.erase(it);
		delete t;
	}
	return nullptr;
}

mVType tStmts::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	for (auto i : branchs)
		i->TypeCheck(sub, subs, whileBlock);
	return mVType();
}

void tStmts::extractlabel(map<string, GrammarTree*>& l) {
	for (auto p : labels) {
		auto&[label, lineNo, tree] = p;
		auto x = l.insert(make_pair(label, tree));
		if (!x.second)
			throw(ErrLabelRedefined(lineNo, label));
	}
}

tSub::tSub(string name, int lineNo, tSubVars* subv, GrammarTree* stmts) :stmts(stmts), vardecl(subv->vars), name(name), lineNo(lineNo) {
	transform(subv->vars.begin(), subv->vars.end(), varpara.begin(), [](const mVar& t) {return t.type; });
	delete subv;
	stmts->extractdecl(vardecl);
	typeReturn = Op::BuiltInTypeObj(Op::BuiltInType::Void);		//返回值为void
	stmts->extractlabel(labels);
}
tSub::~tSub() { delete stmts; }

mVType tSub::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	stmts->TypeCheck(this, subs, nullptr);
	return mVType();
}

void tSub::insertDecl(map<string, vector<mType*>>& m) const {
	auto it = m.insert(make_pair(name, varpara));
	if (!it.second)
		throw(ErrFuncRedeclared(lineNo, name));
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
int tSub::getLineNo() { return lineNo; }

tRoot::~tRoot() { for (auto i : subs) delete i; }
Op::NonTerm tRoot::type() { return Op::NonTerm::subs; }

void tRoot::addSub(tSub* s) {
	subs.push_back(s);
	s->insertDecl(subdecl);
}

mVType tRoot::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	for (auto i : this->subs)
		i->TypeCheck(nullptr, this, nullptr);
	return mVType();
}

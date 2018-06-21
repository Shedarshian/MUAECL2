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
mType GrammarTree::getType() { throw(ErrDesignApp("GrammarTree::getType")); }
int GrammarTree::getLineNo() { throw(ErrDesignApp("GrammarTree::getLineNo")); }
mVType GrammarTree::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) { throw(ErrDesignApp("GrammarTree::TypeCheck")); }
GrammarTree* GrammarTree::typeChange(int rank) { throw(ErrDesignApp("GrammarTree::typeChange")); }

tState::tState(int state) :_state(state) {}
int tState::state() { return _state; }

tTerminator::tTerminator(Token* t) :t(t) {}
tTerminator::~tTerminator() { delete t; }
Token* tTerminator::getToken() { return t; }
GrammarTree* tTerminator::typeChange(int rank) {
	//һ������ֵ
	if (rank == mVType::ITOF) {
		//����ת����
		Token* tok = new Token_Literal(t->getlineNo(), (float)*t->getInt());
		delete t;
		t = tok;
		return this;
	}
	return this;
}

tType::tType(mType t) : t(t) {}
Op::NonTerm tType::type() { return Op::NonTerm::types; }
mType tType::getType() { return t; }

tSubVars::tSubVars(mType type, string id) { vars.emplace_back(type, id); }

void tSubVars::emplaceVar(mType type, string id, int lineNo) {
	auto same = accumulate(vars.begin(), vars.end(), false,
		[&str = id](bool b, mVar& a) {
			return (a.id == str) || b; });
	if (same)
		throw(ErrVarRedeclared(lineNo, id));
	vars.emplace_back(type, id);
}

Op::NonTerm tSubVars::type() { return Op::NonTerm::subv; }

tDeclVars::tDeclVars(string id, int lineNo) :varsi({ { id, lineNo, nullptr } }), typedecl(mType::type_error) {}
tDeclVars::tDeclVars(string id, int lineNo, GrammarTree* inif) :varsi({ { id, lineNo, inif } }), typedecl(mType::type_error) {}
tDeclVars::~tDeclVars() { for (auto i : varsi) delete get<2>(i); }
Op::NonTerm tDeclVars::type() { return Op::NonTerm::vdecl; }
void tDeclVars::addVar(string id, int lineNo) { varsi.emplace_back(id, lineNo, nullptr); }
void tDeclVars::addVar(string id, int lineNo, GrammarTree* inif) { varsi.emplace_back(id, lineNo, inif); }
void tDeclVars::setDeclType(mType type) { this->typedecl = type; }

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
		//��Ҫ��ת��void��ֵ������������������ֵ���ʽ������û�п�����ʽת��void��ֵ��ʵ�ʱ��ʽ�ʲ������ʽת���ڵ�
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		if (!mVType::canChangeTo(_type, VTYPE(Void, r)))
			throw(ErrTypeChangeLoss(branchs[2]->getLineNo(), _type, VTYPE(Void, r, false)));
		//����Ҫ��ֵ����ֵת��
		//_type.valuetype = Op::LRvalue::rvalue;
		break; }
	case 3: //stmt->types vdecl ;
		throw(ErrDesignApp("GrammarTree::TypeCheck.3"));
	case 4: { //stmt->if ( expr ) stmt else stmt
		_type = branchs[2]->TypeCheck(sub, subs, whileBlock);
		if (int rank = mVType::canChangeTo(_type, VTYPE(Int, r)); rank < 0)
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
		if (int rank = mVType::canChangeTo(_type, VTYPE(Int, r)); rank < 0)
			throw(ErrTypeChangeLoss(branchs[1]->getLineNo(), _type, VTYPE(Int, r)));
		else
			branchs[1] = branchs[1]->typeChange(rank);
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
		//���û��while/for�����׳��쳣
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
	case 17: //ini->{ inia } inia = vector<expr>
		//���ͼ��ʱֱ����id = inif����ini�ڲ�Ѱ��???
		_type = VTYPE(inilist, r);
		break;
	case 19: //insv = vector<exprf>
		//���ͼ��ʱֱ����id(insv)����insv�ڲ�Ѱ��
		for (auto i : branchs)
			i->TypeCheck(sub, subs, whileBlock);
		break;
	case 14: //inif->ini
		[[fallthrough]];
	case 16: //inif->expr
		[[fallthrough]];
	case 20: //exprf->expr
		_type = branchs[0]->TypeCheck(sub, subs, whileBlock);
		break;
	case 15: //inif->ini : ini : ini : ini
		[[fallthrough]];
	case 21: //exprf->expr : expr : expr : expr
		exprTypeCheck(Op::TokenType::Colon, sub, subs, whileBlock);
		break;
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
			_type = VTYPE(Int, r, true);
		else if (branchs[0]->getToken()->getFloat() != nullptr)
			_type = VTYPE(Float, r, true);
		else if (branchs[0]->getToken()->getString() != nullptr)
			_type = VTYPE(String, r, true);
		break;
	}
	case 24: { //expr->id ( insv )
		//Ҫ����insv�ڲ�
		auto insv = static_cast<tNoVars*>(branchs[0]);
		auto p = subs->checkSub(*(branchs[1]->getToken()->getString()));
		//��Ϊ��������
		if (p != nullptr) {
			const auto&[returnType, declTypes] = *p;
			if (insv->branchs.size() != declTypes.size())
				throw(ErrFuncPara(lineNo, *(branchs[1]->getToken()->getString())));
			auto it1 = insv->branchs.begin();
			for (auto it2 = declTypes.begin(); it2 != declTypes.end(); it1++, it2++) {
				auto insvptr = static_cast<tNoVars*>(*it1);
				int rank = mVType::canChangeTo(insvptr->_type, mVType{ *it2, Op::rvalue, false });
				if (rank < 0)
					throw(ErrTypeChangeLoss((*it1)->getLineNo(), insvptr->_type, mVType{ *it2, Op::rvalue, false }));
				else
					*it1 = insvptr->typeChange(rank);
			}
			_type = mVType{ returnType, Op::rvalue, false };
		}
		//ins�� TODO
		break;
	}
	case 25: //expr->Unary_op expr
		[[fallthrough]];
	case 26: //expr->expr Binary_op expr/exprf/inif
		exprTypeCheck(branchs[1]->getToken()->type(), sub, subs, whileBlock);
		break;
	case 27: //expr->expr [ expr ]
		exprTypeCheck(Op::TokenType::MidBra, sub, subs, whileBlock);
		break;
	case 28: //expr->( types ) expr
		branchs[0]->TypeCheck(sub, subs, whileBlock);
		_type = mVType{ branchs[1]->getType(), Op::rvalue, false };
		break;
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
		//��ֵ����ֱֵ�Ӹ�ֵ��ȥ
		_type.valuetype = Op::LRvalue::rvalue;
		rank -= mVType::LTOR;
	}
	if (rank == mVType::ITOF) {
		//����ת����
		return new tNoVars(VTYPE(Float, r, _type.isLiteral), 28, -1, this, new tType(Op::mType::Float));
	}
	return this;
}

void tNoVars::LiteralCal() {
#define GET(tree, type) branchs[tree]->getToken()->get##type()
#define TERM(number) new tTerminator(new Token_Literal(branchs[0]->getLineNo(), number))
	GrammarTree* tree = nullptr;
	int change = 0;		//0������ 1��ɾ��1������id 2��ɾ��2������id 3��ɾ��3������ֵtree����id
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
#undef GET
#undef TERM
}

void tNoVars::exprTypeCheck(Op::TokenType typ, tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	auto ltype = branchs[0]->TypeCheck(sub, subs, whileBlock);
	decltype(ltype) rtype;
	bool isLiteral = ltype.isLiteral;
	//����Ƕ�Ԫ�����
	if (branchs.size() == 3) {
		rtype = branchs[2]->TypeCheck(sub, subs, whileBlock);
		isLiteral = isLiteral && rtype.isLiteral;
	}
	else if (branchs.size() == 4) {
		for (int i = 1; i < 4; i++)
			branchs[i]->TypeCheck(sub, subs, whileBlock);
		isLiteral = false;
	}
	auto[low, up] = mVType::typeChange.equal_range(typ);
	int minID = INT_MAX;
	int rank[4], ranksum;
	for (; low != up; ++low) {
		auto &[first, second, type, opID] = low->second;
		rank[0] = mVType::canChangeTo(ltype, first);
		if (rank[0] < 0) continue;
		ranksum = rank[0];
		if (branchs.size() == 3) {
			rank[1] = mVType::canChangeTo(rtype, second);
			if (rank[1] < 0) continue;
			ranksum += rank[1];
		}
		else if (branchs.size() == 4) {
			rank[1] = mVType::canChangeTo(static_cast<tNoVars*>(branchs[1])->_type, second);
			if (rank[1] < 0) continue;
			rank[2] = mVType::canChangeTo(static_cast<tNoVars*>(branchs[1])->_type, second);
			if (rank[2] < 0) continue;
			rank[3] = mVType::canChangeTo(static_cast<tNoVars*>(branchs[1])->_type, second);
			if (rank[3] < 0) continue;
			ranksum += rank[1] + rank[2] + rank[3];
		}
		if (minID < ranksum) {
			minID = ranksum;
			_type = type; this->opID = opID;
		}
	}
	//�޷�Ӧ��������������Ͳ�����
	if (minID == INT_MAX)
		throw(ErrTypeOperate(lineNo, ltype, rtype, (branchs.size() == 4) ? "colon"s : branchs[1]->getToken()->debug_out()));
	//����rankֵȷ������Щʲô�任
	branchs[0] = branchs[0]->typeChange(rank[0]);
	if (branchs.size() == 3)
		branchs[2] = branchs[2]->typeChange(rank[1]);
	else if (branchs.size() == 4)
		for (int i : {1, 2, 3})
			branchs[i] = branchs[i]->typeChange(rank[i]);
	//���������������Ż�����Ҫ�ų�������ָ�������������洦��
	if (isLiteral)
		LiteralCal();
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
	typeReturn = Op::mType::Void;		//����ֵΪvoid
	stmts->extractlabel(labels);
}
tSub::~tSub() { delete stmts; }

mVType tSub::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	stmts->TypeCheck(this, subs, nullptr);
	return mVType();
}

void tSub::insertDecl(map<string, pair<mType, vector<mType>>>& m) const {
	auto it = m.insert(make_pair(name, make_pair(typeReturn, varpara)));
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

pair<mType, vector<mType>>* tRoot::checkSub(const string& id) {
	auto list = subdecl.find(id);
	if (list == subdecl.end())
		return nullptr;
	return &(list->second);
}

mVType tRoot::TypeCheck(tSub* sub, tRoot* subs, GrammarTree* whileBlock) {
	for (auto i : this->subs)
		i->TypeCheck(nullptr, this, nullptr);
	return mVType();
}

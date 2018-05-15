#include "stdafx.h"
#include "Exprnizer.h"
#include <stack>

ExprTree::~ExprTree() {
	delete left;
	delete right;
	delete token;
}

ExprTree* Exprnizer(Tokenizer& t) {
	stack<Token*> opstack;
	stack<ExprTree*> treestack;

	opstack.push(new Token_Operator(-1, Operator::End));
	ExprTree* tree = nullptr;
	bool testempty = false;
	while (1) {
		if (t.peekToken()->isIdNum())
			treestack.push(new ExprTree(t.popToken()));
		else if (t.peekToken()->type() == Token::Type::Token_Operator) {
			Operator op = t.peekToken()->getOperator();
			if (opstack.top()->getOperator() > op) {
				Token* t_in_s = opstack.top(); opstack.pop();
				//�������ں�֮�󲻶�pop��pop���ڵģ�ֱ��С��Ϊֹ
				//����ֻ�������ŵ��������������ڵ���ʱ��⼴��
				//�ж�һԪ/��Ԫ
				switch (Op::Id(t_in_s->getOperator())) {
				case 0:
					throw(ErrDesignApp("Found ( at undesigned position."));
					break;
				case 1:
					if (treestack.empty()) {
						//throw lost identifier
					}
					auto right = treestack.top(); treestack.pop();
					treestack.push(new ExprTree(t_in_s, right));
					break;
				case 2:
					if (treestack.empty()) {
						//throw lost identifier
					}
					auto right = treestack.top(); treestack.pop();
					if (treestack.empty()) {
						//throw lost identifier
					}
					auto left = treestack.top(); treestack.pop();
					treestack.push(new ExprTree(t_in_s, left, right));
					break;
				}
			}
			//���������ж�������ڼ�������˵��ԭ���ʽa==bһ��b>c
			else if (opstack.top()->getOperator() == op) {
				//���ں�ֻ������()��$)��Ŀǰ��
				//()��pop�������ţ����䣬$)���˳�
				if (opstack.top()->getOperator() == Operator::End) {
					testempty = true;
					break;
				}
				delete t.popToken();
				delete opstack.top(); opstack.pop();
			}
			else
				opstack.push(t.popToken());
		}
		else if (t.peekToken()->isExprFollow() && opstack.top()->getOperator() == Operator::End) {
			//����$��FOLLOW(expr)�����˳�
			testempty = true;
			break;
		}
		else
			throw(ErrIllegalToken(t.debug_lineNo, t.peekToken()));
	}
	if (testempty) {
		tree = treestack.top();
		treestack.pop();
		if (!treestack.empty()) {
			//throw ���identifier
		}
	}
	delete opstack.top();
	return tree;
}

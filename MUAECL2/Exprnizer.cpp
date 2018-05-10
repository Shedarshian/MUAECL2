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
				//遇到大于号之后不断pop直到大于为止
				while (opstack.top()->getOperator() < t_in_s->getOperator()) {
					//TODO 检查左括号
					//TODO 判断一元/二元
					if (treestack.empty()) {
						//throw lost identifier
					}
					auto right = treestack.top(); treestack.pop();
					if (treestack.empty()) {
						//throw lost identifier
					}
					auto left = treestack.top(); treestack.pop();
					treestack.push(new ExprTree(t_in_s, left, right));
					t_in_s = opstack.top(); opstack.pop();
				}
				opstack.push(t_in_s);
				//TODO 把t的token重新push回去，判断是否是括号
			}
			else if (opstack.top()->getOperator() == op) {
				//等于号只可能是()或$)（目前）
				//()则pop掉左括号，不变，$)则退出
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
			//若是$接FOLLOW(expr)，则退出
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
			//throw 多个identifier
		}
	}
	delete opstack.top();
	return tree;
}

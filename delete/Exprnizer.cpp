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
				//遇到大于号之后不断pop（pop等于的）直到小于为止
				//不过只有左括号等于右括号所以在等于时检测即可
				//判断一元/二元
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
			//放在这里判断如果等于即操作，说明原表达式a==b一定b>c
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

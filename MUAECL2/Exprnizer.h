////////////////////////////////////////////////////////////////
//**************************************************************
//*  使用算符优先分析法
//*  算符优先级：
//*    ||
//*    &&
//*    or
//*    and
//*    |
//*    ^
//*    &
//*    == !=
//*    > >= < <= 
//*    + -
//*    * / %
//*    (-) ! (type) sin cos
//*    . ( )
//*
//*    ||  &&  or  and |   ^   &   ==  !=  >   >=  <   <=  +   -   *   /   %   (-) !   .   (   )   $
//* f  2   4   6   8   10  12  14  16  16  18  18  18  18  20  20  22  22  22  22  22  24  0   24  0
//* g  1   3   5   7   9   11  13  15  15  17  17  17  17  19  19  21  21  21  25  25  23  25  0   0
//*
//*
//*
//*
//**************************************************************
////////////////////////////////////////////////////////////////

#pragma once
#include "Misc.h"
#include "Tokenizer.h"

using namespace std;

struct ExprTree {
	ExprTree(Token* token) :left(nullptr), right(nullptr), token(token) {};
	ExprTree(Token* token, ExprTree* one_subtree) :left(nullptr), right(one_subtree), token(token) {};
	ExprTree(Token* token, ExprTree* left_subtree, ExprTree* right_subtree) :left(left_subtree), right(right_subtree), token(token) {};
	~ExprTree();

	ExprTree* left;
	ExprTree* right;
	Token* token;
};

ExprTree* Exprnizer(Tokenizer& t);


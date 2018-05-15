#include "stdafx.h"
#include "Parser.h"
#include <algorithm>
#define asops(num) { Term::Equal, num },{ Term::PlusEqual, num },{ Term::MinusEqual, num },{ Term::TimesEqual, num },{ Term::DividesEqual, num },{ Term::ModEqual, num },{ Term::LogicalOrEqual, num },{ Term::LogicalAndEqual, num },{ Term::BitOrEqual, num },{ Term::BitAndEqual, num },{ Term::BitXorEqual, num }
#define op2s(num) { Term::Plus, num },{ Term::Minus, num },{ Term::Times, num },{ Term::Divides, num },{ Term::Mod, num },{ Term::EqualTo, num },{ Term::NotEqual, num },{ Term::Less, num },{ Term::LessEqual, num },{ Term::Greater, num },{ Term::GreaterEqual, num },{ Term::LogicalOr, num },{ Term::LogicalAnd, num },{ Term::BitOr, num },{ Term::BitAnd, num },{ Term::BitXor, num },{ Term::Dot, num },{ Term::And, num },{ Term::Or, num },asops(num)
#define op1s(num) { Term::Negative, num },{ Term::Not, num },{ Term::Deref, num },{ Term::Address, num }
#define folwstmt(num) { Term::Identifier, num },{ Term::Bra, num },{ Term::BigBra, num },{ Term::BigKet, num },{ Term::Type, num },{ Term::If, num },{ Term::Else, num },{ Term::While, num },{ Term::For, num },{ Term::Goto, num },{ Term::Break, num },{ Term::Continue, num },op1s(num)
#define folwexpr(num) { Term::Colon, num },{ Term::Semicolon, num },{ Term::Comma, num },{ Term::Ket, num },{ Term::BigKet, num },{ Term::MidBra, num },{ Term::MidKet, num },op2s(num)
#define gotoOp2s { Term::LogicalOr, 101 },{ Term::LogicalAnd, 102 },{ Term::Or, 103 },{ Term::And, 104 },{ Term::BitOr, 105 },{ Term::BitXor, 106 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(121)
#define gotoOp1s { Term::Negative, 82 },{ Term::Not, 83 },{ Term::Deref, 85 },{ Term::Address, 86 }

using namespace std;
using Term = Op::Token;
using NonTerm = Parser::NonTerminator;
using mti = map<Term, int>;

//复制一个map并将其value加i
auto ma(mti* s, int i)->decltype(s) {
	decltype(s) t = new mti();
	for (auto it : *s)
		(*t)[it.first] = it.second + i;
	return t;
}

const map<int, mti*> makeAction() {
	map<int, mti*> a;
	a[255] = new mti({ { Term::End, -1 } });
	a[0] = new mti({ { Term::End, 12 },{ Term::Sub, 34 } });
	a[1] = new mti({ { Term::Colon, 8 },{ Term::Bra, 10 },{ Term::Ket, 1039 },{ Term::BigKet, 1039 },{ Term::MidBra, 1039 },{ Term::MidKet, 1039 },op2s(1039),asops(1039) });
	a[2] = new mti({ { Term::Identifier, 11 } });
	a[3] = new mti({ { Term::Identifier, 94 },{ Term::Number, 44 },{ Term::Bra, 84 },{ Term::Negative, 82 },{ Term::Not, 83 },{ Term::Deref, 85 },{ Term::Address, 86 } });
	a[4] = a[3];
	a[5] = a[3];
	a[6] = new mti({ folwstmt(1010) });
	a[7] = new mti({ { Term::Identifier, 1 },{ Term::Number, 44 },{ Term::Bra, 84 },{ Term::BigBra, 77 },{ Term::BigKet, 1003 },{ Term::Type, 2 },{ Term::If, 3 },{ Term::While, 4 },{ Term::For, 5 },{ Term::Goto, 6 },{ Term::Break, 96 },{ Term::Continue, 97 },gotoOp1s });
	a[8] = new mti({ { Term::Identifier, 94 },{ Term::Number, 44 },{ Term::Bra, 84 },{ Term::BigBra, 77 },{ Term::Type, 2 },{ Term::If, 3 },{ Term::While, 4 },{ Term::For, 5 },{ Term::Goto, 6 },{ Term::Break, 96 },{ Term::Continue, 97 },gotoOp1s });
	a[9] = a[7];
	a[10] = new mti({ { Term::Identifier, 94 },{ Term::Number, 44 },{ Term::Bra, 84 },{ Term::Ket, 1038 },gotoOp1s });
	a[11] = new mti({ { Term::Semicolon, 1032 },{ Term::Comma, 59 },{ Term::Equal, 53 } });
	a[12] = new mti({ { Term::Semicolon, 57 } });
	a[13] = new mti({ { Term::Ket, 58 },gotoOp2s });
	a[14] = new mti({ { Term::Ket, 73 },gotoOp2s });
	a[15] = new mti({ { Term::Colon, 36 },{ Term::Semicolon, 1030 },{ Term::Comma, 1030 },{ Term::Ket, 1030 } });
	a[16] = new mti({ { Term::Ket, 75 } });
	a[17] = new mti({ { Term::BigKet, 1001 } });
	a[18] = ma(a[17], 1);
	a[19] = new mti({ { Term::Colon, 1026 },{ Term::Semicolon, 1026 },{ Term::Comma, 1026 },gotoOp2s });
	a[20] = a[3];
	a[21] = new mti({ { Term::Colon, 28 },{ Term::Semicolon, 1024 },{ Term::Comma, 1024 } });
	a[22] = new mti({ { Term::Comma, 23 },{ Term::BigKet, 29 },gotoOp2s });
	a[23] = a[3];
	a[24] = a[22];
	a[25] = new mti({ { Term::BigKet, 1028 } });
	a[26] = new mti({ { Term::BigKet, 27 } });
	a[27] = new mti({ { Term::Colon, 1027 },{ Term::Semicolon, 1027 },{ Term::Comma, 1027 } });
	a[28] = new mti({ { Term::Identifier, 94 },{ Term::Number, 44 },{ Term::Bra, 84 },{ Term::BigBra, 20 }, });
	a[29] = new mti({ { Term::Colon, 30 } });
	a[30] = a[28];
	a[31] = new mti({ { Term::Colon, 32 } });
	a[32] = a[28];
	a[33] = new mti({ { Term::Semicolon, 1025 },{ Term::Comma, 1025 } });
	a[34] = new mti({ { Term::Identifier, 35 } });
	a[35] = new mti({ { Term::Bra, 36 } });
	a[36] = a[3];
	a[37] = new mti({ { Term::Colon, 38 },gotoOp2s });
	a[38] = a[3];
	a[39] = new mti({ { Term::Colon, 40 },gotoOp2s });
	a[40] = a[3];
	a[41] = new mti({ { Term::Semicolon, 1031 },{ Term::Comma, 1031 },{ Term::Ket, 1031 },gotoOp2s });
	a[42] = new mti({ { Term::Comma, 43 },{ Term::Ket, 1036 } });
	a[43] = a[10];
	a[44] = new mti({ folwexpr(1040) });
	a[45] = new mti({ { Term::Ket, 1037 } });
	a[46] = new mti({ { Term::Ket, 47 } });
	a[47] = new mti({ folwexpr(1063) });
	a[48] = new mti({ { Term::Semicolon, 1034 } });
	a[49] = new mti({ { Term::BigKet, 50 } });
	a[50] = new mti({ { Term::End, 1012 },{ Term::Sub, 34 } });
	a[51] = new mti({ { Term::End, 1013 } });
	a[53] = a[28];
	a[54] = a[2];
	a[55] = new mti({ { Term::Semicolon, 1035 } });
	a[56] = new mti({ { Term::BigBra, 9 } });
	a[57] = new mti({ folwstmt(1005) });
	a[58] = a[8];
	a[59] = a[2];
	a[60] = new mti({ folwstmt(1007) });
	a[61] = new mti({ { Term::Semicolon, 47 } });
	a[62] = new mti({ { Term::Identifier, 63 } });
	a[63] = new mti({ { Term::Comma, 64 },{ Term::Ket, 1016 } });
	a[64] = new mti({ { Term::Type, 65 } });
	a[65] = new mti({ { Term::Identifier, 66 } });
	a[66] = a[63];
	a[67] = new mti({ { Term::Ket, 1015 } });
	a[68] = new mti({ { Term::Ket, 1017 } });
	a[69] = new mti({ { Term::Ket, 56 } });
	a[70] = a[8];
	a[71] = new mti({ folwstmt(1018) });
	a[73] = a[8];
	a[74] = new mti({ folwstmt(1008) });
	a[75] = a[8];
	a[76] = new mti({ folwstmt(1009) });
	a[77] = a[8];
	a[78] = new mti({ { Term::BigKet, 81 } });
	a[79] = a[7];
	a[80] = new mti({ { Term::Semicolon, 1033 },{ Term::Comma, 54 } });
	a[81] = new mti({ folwstmt(1011) });
	for (int i = 82; i < 87; i++)
		a[i] = a[3];
	a[87] = new mti({ { Term::Semicolon, 95 },gotoOp2s });
	a[88] = new mti({ folwexpr(1059) }); (*a[88])[Term::Dot] = 119;
	a[89] = new mti({ folwexpr(1060) }); (*a[89])[Term::Dot] = 119;
	a[90] = new mti({ folwexpr(1064) }); (*a[90])[Term::Dot] = 119;
	a[91] = new mti({ folwexpr(1065) }); (*a[91])[Term::Dot] = 119;
	a[92] = new mti({ { Term::Ket, 93 },gotoOp1s });
	a[93] = new mti({ folwexpr(1062) });
	a[94] = new mti({ folwexpr(1039) }); (*a[94])[Term::Bra] = 10;
	a[95] = new mti({ folwstmt(1019) });
	a[96] = new mti({ folwstmt(1020) });
	for (int i = 101; i < 122; i++)
		a[i] = a[3];
	a[122] = new mti({ folwexpr(1067) });
	a[123] = new mti({ folwexpr(1066) });
	a[131] = new mti({ { Term::Colon, 1041 },{ Term::Semicolon, 1041 },{ Term::Comma, 1041 },{ Term::Ket, 1041 },{ Term::BigKet, 1041 },{ Term::MidBra, 1041 },{ Term::MidKet, 1041 },{ Term::LogicalOr, 1041 },{ Term::LogicalAnd, 102 },{ Term::Or, 103 },{ Term::And, 104 },{ Term::BitOr, 105 },{ Term::BitXor, 106 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1041) });
	a[132] = new mti({ { Term::Colon, 1042 },{ Term::Semicolon, 1042 },{ Term::Comma, 1042 },{ Term::Ket, 1042 },{ Term::BigKet, 1042 },{ Term::MidBra, 1042 },{ Term::MidKet, 1042 },{ Term::LogicalOr, 1042 },{ Term::LogicalAnd, 1042 },{ Term::Or, 103 },{ Term::And, 104 },{ Term::BitOr, 105 },{ Term::BitXor, 106 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1042) });
	a[133] = new mti({ { Term::Colon, 1043 },{ Term::Semicolon, 1043 },{ Term::Comma, 1043 },{ Term::Ket, 1043 },{ Term::BigKet, 1043 },{ Term::MidBra, 1043 },{ Term::MidKet, 1043 },{ Term::LogicalOr, 1043 },{ Term::LogicalAnd, 1043 },{ Term::Or, 1043 },{ Term::And, 104 },{ Term::BitOr, 105 },{ Term::BitXor, 106 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1043) });
	a[134] = new mti({ { Term::Colon, 1044 },{ Term::Semicolon, 1044 },{ Term::Comma, 1044 },{ Term::Ket, 1044 },{ Term::BigKet, 1044 },{ Term::MidBra, 1044 },{ Term::MidKet, 1044 },{ Term::LogicalOr, 1044 },{ Term::LogicalAnd, 1044 },{ Term::Or, 1044 },{ Term::And, 1044 },{ Term::BitOr, 105 },{ Term::BitXor, 106 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1044) });
	a[135] = new mti({ { Term::Colon, 1045 },{ Term::Semicolon, 1045 },{ Term::Comma, 1045 },{ Term::Ket, 1045 },{ Term::BigKet, 1045 },{ Term::MidBra, 1045 },{ Term::MidKet, 1045 },{ Term::LogicalOr, 1045 },{ Term::LogicalAnd, 1045 },{ Term::Or, 1045 },{ Term::And, 1045 },{ Term::BitOr, 1045 },{ Term::BitXor, 106 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1045) });
	a[136] = new mti({ { Term::Colon, 1046 },{ Term::Semicolon, 1046 },{ Term::Comma, 1046 },{ Term::Ket, 1046 },{ Term::BigKet, 1046 },{ Term::MidBra, 1046 },{ Term::MidKet, 1046 },{ Term::LogicalOr, 1046 },{ Term::LogicalAnd, 1046 },{ Term::Or, 1046 },{ Term::And, 1046 },{ Term::BitOr, 1046 },{ Term::BitXor, 1046 },{ Term::BitAnd, 107 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1046) });
	a[137] = new mti({ { Term::Colon, 1047 },{ Term::Semicolon, 1047 },{ Term::Comma, 1047 },{ Term::Ket, 1047 },{ Term::BigKet, 1047 },{ Term::MidBra, 1047 },{ Term::MidKet, 1047 },{ Term::LogicalOr, 1047 },{ Term::LogicalAnd, 1047 },{ Term::Or, 1047 },{ Term::And, 1047 },{ Term::BitOr, 1047 },{ Term::BitXor, 1047 },{ Term::BitAnd, 1047 },{ Term::EqualTo, 108 },{ Term::NotEqual, 109 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1047) });
	a[138] = new mti({ { Term::Colon, 1048 },{ Term::Semicolon, 1048 },{ Term::Comma, 1048 },{ Term::Ket, 1048 },{ Term::BigKet, 1048 },{ Term::MidBra, 1048 },{ Term::MidKet, 1048 },{ Term::LogicalOr, 1048 },{ Term::LogicalAnd, 1048 },{ Term::Or, 1048 },{ Term::And, 1048 },{ Term::BitOr, 1048 },{ Term::BitXor, 1048 },{ Term::BitAnd, 1048 },{ Term::EqualTo, 1048 },{ Term::NotEqual, 1048 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1048) });
	a[139] = new mti({ { Term::Colon, 1049 },{ Term::Semicolon, 1049 },{ Term::Comma, 1049 },{ Term::Ket, 1049 },{ Term::BigKet, 1049 },{ Term::MidBra, 1049 },{ Term::MidKet, 1049 },{ Term::LogicalOr, 1049 },{ Term::LogicalAnd, 1049 },{ Term::Or, 1049 },{ Term::And, 1049 },{ Term::BitOr, 1049 },{ Term::BitXor, 1049 },{ Term::BitAnd, 1049 },{ Term::EqualTo, 1049 },{ Term::NotEqual, 1049 },{ Term::Less, 110 },{ Term::LessEqual, 111 },{ Term::Greater, 112 },{ Term::GreaterEqual, 113 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1049) });
	a[140] = new mti({ { Term::Colon, 1050 },{ Term::Semicolon, 1050 },{ Term::Comma, 1050 },{ Term::Ket, 1050 },{ Term::BigKet, 1050 },{ Term::MidBra, 1050 },{ Term::MidKet, 1050 },{ Term::LogicalOr, 1050 },{ Term::LogicalAnd, 1050 },{ Term::Or, 1050 },{ Term::And, 1050 },{ Term::BitOr, 1050 },{ Term::BitXor, 1050 },{ Term::BitAnd, 1050 },{ Term::EqualTo, 1050 },{ Term::NotEqual, 1050 },{ Term::Less, 1050 },{ Term::LessEqual, 1050 },{ Term::Greater, 1050 },{ Term::GreaterEqual, 1050 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1050) });
	a[141] = new mti({ { Term::Colon, 1051 },{ Term::Semicolon, 1051 },{ Term::Comma, 1051 },{ Term::Ket, 1051 },{ Term::BigKet, 1051 },{ Term::MidBra, 1051 },{ Term::MidKet, 1051 },{ Term::LogicalOr, 1051 },{ Term::LogicalAnd, 1051 },{ Term::Or, 1051 },{ Term::And, 1051 },{ Term::BitOr, 1051 },{ Term::BitXor, 1051 },{ Term::BitAnd, 1051 },{ Term::EqualTo, 1051 },{ Term::NotEqual, 1051 },{ Term::Less, 1051 },{ Term::LessEqual, 1051 },{ Term::Greater, 1051 },{ Term::GreaterEqual, 1051 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1051) });
	a[142] = new mti({ { Term::Colon, 1052 },{ Term::Semicolon, 1052 },{ Term::Comma, 1052 },{ Term::Ket, 1052 },{ Term::BigKet, 1052 },{ Term::MidBra, 1052 },{ Term::MidKet, 1052 },{ Term::LogicalOr, 1052 },{ Term::LogicalAnd, 1052 },{ Term::Or, 1052 },{ Term::And, 1052 },{ Term::BitOr, 1052 },{ Term::BitXor, 1052 },{ Term::BitAnd, 1052 },{ Term::EqualTo, 1052 },{ Term::NotEqual, 1052 },{ Term::Less, 1052 },{ Term::LessEqual, 1052 },{ Term::Greater, 1052 },{ Term::GreaterEqual, 1052 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1052) });
	a[143] = new mti({ { Term::Colon, 1053 },{ Term::Semicolon, 1053 },{ Term::Comma, 1053 },{ Term::Ket, 1053 },{ Term::BigKet, 1053 },{ Term::MidBra, 1053 },{ Term::MidKet, 1053 },{ Term::LogicalOr, 1053 },{ Term::LogicalAnd, 1053 },{ Term::Or, 1053 },{ Term::And, 1053 },{ Term::BitOr, 1053 },{ Term::BitXor, 1053 },{ Term::BitAnd, 1053 },{ Term::EqualTo, 1053 },{ Term::NotEqual, 1053 },{ Term::Less, 1053 },{ Term::LessEqual, 1053 },{ Term::Greater, 1053 },{ Term::GreaterEqual, 1053 },{ Term::Plus, 114 },{ Term::Minus, 115 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1053) });
	a[144] = new mti({ { Term::Colon, 1054 },{ Term::Semicolon, 1054 },{ Term::Comma, 1054 },{ Term::Ket, 1054 },{ Term::BigKet, 1054 },{ Term::MidBra, 1054 },{ Term::MidKet, 1054 },{ Term::LogicalOr, 1054 },{ Term::LogicalAnd, 1054 },{ Term::Or, 1054 },{ Term::And, 1054 },{ Term::BitOr, 1054 },{ Term::BitXor, 1054 },{ Term::BitAnd, 1054 },{ Term::EqualTo, 1054 },{ Term::NotEqual, 1054 },{ Term::Less, 1054 },{ Term::LessEqual, 1054 },{ Term::Greater, 1054 },{ Term::GreaterEqual, 1054 },{ Term::Plus, 1054 },{ Term::Minus, 1054 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1054) });
	a[145] = new mti({ { Term::Colon, 1055 },{ Term::Semicolon, 1055 },{ Term::Comma, 1055 },{ Term::Ket, 1055 },{ Term::BigKet, 1055 },{ Term::MidBra, 1055 },{ Term::MidKet, 1055 },{ Term::LogicalOr, 1055 },{ Term::LogicalAnd, 1055 },{ Term::Or, 1055 },{ Term::And, 1055 },{ Term::BitOr, 1055 },{ Term::BitXor, 1055 },{ Term::BitAnd, 1055 },{ Term::EqualTo, 1055 },{ Term::NotEqual, 1055 },{ Term::Less, 1055 },{ Term::LessEqual, 1055 },{ Term::Greater, 1055 },{ Term::GreaterEqual, 1055 },{ Term::Plus, 1055 },{ Term::Minus, 1055 },{ Term::Times, 116 },{ Term::Divides, 117 },{ Term::Mod, 118 },{ Term::Dot, 119 },asops(1055) });
	a[146] = new mti({ { Term::Colon, 1056 },{ Term::Semicolon, 1056 },{ Term::Comma, 1056 },{ Term::Ket, 1056 },{ Term::BigKet, 1056 },{ Term::MidBra, 1056 },{ Term::MidKet, 1056 },{ Term::LogicalOr, 1056 },{ Term::LogicalAnd, 1056 },{ Term::Or, 1056 },{ Term::And, 1056 },{ Term::BitOr, 1056 },{ Term::BitXor, 1056 },{ Term::BitAnd, 1056 },{ Term::EqualTo, 1056 },{ Term::NotEqual, 1056 },{ Term::Less, 1056 },{ Term::LessEqual, 1056 },{ Term::Greater, 1056 },{ Term::GreaterEqual, 1056 },{ Term::Plus, 1056 },{ Term::Minus, 1056 },{ Term::Times, 1056 },{ Term::Divides, 1056 },{ Term::Mod, 1056 },{ Term::Dot, 119 },asops(1056) });
	a[147] = new mti({ { Term::Colon, 1057 },{ Term::Semicolon, 1057 },{ Term::Comma, 1057 },{ Term::Ket, 1057 },{ Term::BigKet, 1057 },{ Term::MidBra, 1057 },{ Term::MidKet, 1057 },{ Term::LogicalOr, 1057 },{ Term::LogicalAnd, 1057 },{ Term::Or, 1057 },{ Term::And, 1057 },{ Term::BitOr, 1057 },{ Term::BitXor, 1057 },{ Term::BitAnd, 1057 },{ Term::EqualTo, 1057 },{ Term::NotEqual, 1057 },{ Term::Less, 1057 },{ Term::LessEqual, 1057 },{ Term::Greater, 1057 },{ Term::GreaterEqual, 1057 },{ Term::Plus, 1057 },{ Term::Minus, 1057 },{ Term::Times, 1057 },{ Term::Divides, 1057 },{ Term::Mod, 1057 },{ Term::Dot, 119 },asops(1057) });
	a[148] = new mti({ { Term::Colon, 1058 },{ Term::Semicolon, 1058 },{ Term::Comma, 1058 },{ Term::Ket, 1058 },{ Term::BigKet, 1058 },{ Term::MidBra, 1058 },{ Term::MidKet, 1058 },{ Term::LogicalOr, 1058 },{ Term::LogicalAnd, 1058 },{ Term::Or, 1058 },{ Term::And, 1058 },{ Term::BitOr, 1058 },{ Term::BitXor, 1058 },{ Term::BitAnd, 1058 },{ Term::EqualTo, 1058 },{ Term::NotEqual, 1058 },{ Term::Less, 1058 },{ Term::LessEqual, 1058 },{ Term::Greater, 1058 },{ Term::GreaterEqual, 1058 },{ Term::Plus, 1058 },{ Term::Minus, 1058 },{ Term::Times, 1058 },{ Term::Divides, 1058 },{ Term::Mod, 1058 },{ Term::Dot, 119 },asops(1058) });
	a[149] = new mti({ { Term::Colon, 1061 },{ Term::Semicolon, 1061 },{ Term::Comma, 1061 },{ Term::Ket, 1061 },{ Term::BigKet, 1061 },{ Term::MidBra, 1061 },{ Term::MidKet, 1061 },{ Term::LogicalOr, 1061 },{ Term::LogicalAnd, 1061 },{ Term::Or, 1061 },{ Term::And, 1061 },{ Term::BitOr, 1061 },{ Term::BitXor, 1061 },{ Term::BitAnd, 1061 },{ Term::EqualTo, 1061 },{ Term::NotEqual, 1061 },{ Term::Less, 1061 },{ Term::LessEqual, 1061 },{ Term::Greater, 1061 },{ Term::GreaterEqual, 1061 },{ Term::Plus, 1061 },{ Term::Minus, 1061 },{ Term::Times, 1061 },{ Term::Divides, 1061 },{ Term::Mod, 1061 },{ Term::Dot, 1061 },asops(1061) });
	a[150] = new mti({ gotoOp2s });
	return a;
}

const map<int, map<Term, int>*> Action = makeAction();
const map<Parser::NonTerminator, map<int, int>> Goto = {
	{ NonTerm::stmt, map<int, int>({ { 0, 7 },{ 7, 7 },{ 8, 79 },{ 9, 7 },{ 58, 60 },{ 70, 71 },{ 73, 74 },{ 75, 76 },{ 77, 7 },{ 79, 7 } }) },
	{ NonTerm::stmts, map<int, int>({ { 7, 17 },{ 9, 49 },{ 77, 78 },{ 79, 18 } }) },
	{ NonTerm::subs, map<int, int>({ { 0, 255 },{ 50, 51 } }) },
	{ NonTerm::subv, map<int, int>({ { 61, 69 } }) },
	{ NonTerm::subva, map<int, int>({ { 63, 67 },{ 66, 68 } }) },
	{ NonTerm::vdecl, map<int, int>({ { 2, 12 },{ 54, 55 },{ 59, 48 } }) },
	{ NonTerm::insv, map<int, int>({ { 10, 46 },{ 43, 45 } }) },
	{ NonTerm::ini, map<int, int>({ { 28, 29 },{ 30, 31 },{ 32, 33 },{ 53, 21 } }) },
	{ NonTerm::inif, map<int, int>({ { 53, 80 } }) },
	{ NonTerm::inia, map<int, int>({ { 22, 26 },{ 24, 25 } }) },
	{ NonTerm::exprf, map<int, int>({ { 5, 16 },{ 10, 42 },{ 43, 42 },{ 120, 122 } }) },
	{ NonTerm::expr, map<int, int>({ { 3, 13 },{ 4, 14 },{ 5, 15 },{ 7, 87 },{ 8, 87 },{ 9, 87 },{ 10, 15 },{ 20, 22 },{ 23, 24 },{ 28, 19 },{ 30, 19 },{ 32, 19 },{ 36, 37 },{ 38, 39 },{ 40, 41 },{ 43, 15 },{ 53, 19 },{ 58, 87 },{ 70, 87 },{ 77, 87 },{ 82, 88 },{ 83, 89 },{ 84, 92 },{ 85, 90 },{ 86, 91 },{ 101, 131 },{ 102, 132 },{ 103, 133 },{ 104, 134 },{ 105, 135 },{ 106, 136 },{ 107, 137 },{ 108, 138 },{ 109, 139 },{ 110, 140 },{ 111, 141 },{ 112, 142 },{ 113, 143 },{ 114, 144 },{ 115, 145 },{ 116, 146 },{ 117, 147 },{ 118, 148 },{ 119, 149 },{ 120, 150 },{ 121, 15 } }) }
};

Parser::Parser(Tokenizer &tokenizer) :tokenizer(tokenizer) {}

Parser::~Parser() {}

int Parser::action(int s, Op::Token t) {
	auto m = Action.find(s)->second;
	auto it = m->find(t);
	if (it == m->end())
		throw("");//TODO
	return it->second;
}

int Parser::gotostat(int s, Parser::NonTerminator t) {
	return Goto.find(t)->second.find(s)->second;
}

// Starter token/type names and PA2Decode algorithms:
// (C) 2013 CPPGM Foundation www.cppgm.org. All rights reserved.
#include "preprocess/post/types.h"
#include <unordered_map>
namespace cppgm {
const char* type_name(FundamentalType type) {
 switch(type) {
case FundamentalType::FT_SIGNED_CHAR: return "signed char";
case FundamentalType::FT_SHORT_INT: return "short int";
case FundamentalType::FT_INT: return "int";
case FundamentalType::FT_LONG_INT: return "long int";
case FundamentalType::FT_LONG_LONG_INT: return "long long int";
case FundamentalType::FT_UNSIGNED_CHAR: return "unsigned char";
case FundamentalType::FT_UNSIGNED_SHORT_INT: return "unsigned short int";
case FundamentalType::FT_UNSIGNED_INT: return "unsigned int";
case FundamentalType::FT_UNSIGNED_LONG_INT: return "unsigned long int";
case FundamentalType::FT_UNSIGNED_LONG_LONG_INT: return "unsigned long long int";
case FundamentalType::FT_WCHAR_T: return "wchar_t";
case FundamentalType::FT_CHAR: return "char";
case FundamentalType::FT_CHAR16_T: return "char16_t";
case FundamentalType::FT_CHAR32_T: return "char32_t";
case FundamentalType::FT_BOOL: return "bool";
case FundamentalType::FT_FLOAT: return "float";
case FundamentalType::FT_DOUBLE: return "double";
case FundamentalType::FT_LONG_DOUBLE: return "long double";
case FundamentalType::FT_VOID: return "void";
case FundamentalType::FT_NULLPTR_T: return "nullptr_t";
} return "?";
}
const char* simple_name(SimpleKind kind) {
 switch(kind) {
case SimpleKind::KW_ALIGNAS: return "KW_ALIGNAS";
case SimpleKind::KW_ALIGNOF: return "KW_ALIGNOF";
case SimpleKind::KW_ASM: return "KW_ASM";
case SimpleKind::KW_AUTO: return "KW_AUTO";
case SimpleKind::KW_BOOL: return "KW_BOOL";
case SimpleKind::KW_BREAK: return "KW_BREAK";
case SimpleKind::KW_CASE: return "KW_CASE";
case SimpleKind::KW_CATCH: return "KW_CATCH";
case SimpleKind::KW_CHAR: return "KW_CHAR";
case SimpleKind::KW_CHAR16_T: return "KW_CHAR16_T";
case SimpleKind::KW_CHAR32_T: return "KW_CHAR32_T";
case SimpleKind::KW_CLASS: return "KW_CLASS";
case SimpleKind::KW_CONST: return "KW_CONST";
case SimpleKind::KW_CONSTEXPR: return "KW_CONSTEXPR";
case SimpleKind::KW_CONST_CAST: return "KW_CONST_CAST";
case SimpleKind::KW_CONTINUE: return "KW_CONTINUE";
case SimpleKind::KW_DECLTYPE: return "KW_DECLTYPE";
case SimpleKind::KW_DEFAULT: return "KW_DEFAULT";
case SimpleKind::KW_DELETE: return "KW_DELETE";
case SimpleKind::KW_DO: return "KW_DO";
case SimpleKind::KW_DOUBLE: return "KW_DOUBLE";
case SimpleKind::KW_DYNAMIC_CAST: return "KW_DYNAMIC_CAST";
case SimpleKind::KW_ELSE: return "KW_ELSE";
case SimpleKind::KW_ENUM: return "KW_ENUM";
case SimpleKind::KW_EXPLICIT: return "KW_EXPLICIT";
case SimpleKind::KW_EXPORT: return "KW_EXPORT";
case SimpleKind::KW_EXTERN: return "KW_EXTERN";
case SimpleKind::KW_FALSE: return "KW_FALSE";
case SimpleKind::KW_FLOAT: return "KW_FLOAT";
case SimpleKind::KW_FOR: return "KW_FOR";
case SimpleKind::KW_FRIEND: return "KW_FRIEND";
case SimpleKind::KW_GOTO: return "KW_GOTO";
case SimpleKind::KW_IF: return "KW_IF";
case SimpleKind::KW_INLINE: return "KW_INLINE";
case SimpleKind::KW_INT: return "KW_INT";
case SimpleKind::KW_LONG: return "KW_LONG";
case SimpleKind::KW_MUTABLE: return "KW_MUTABLE";
case SimpleKind::KW_NAMESPACE: return "KW_NAMESPACE";
case SimpleKind::KW_NEW: return "KW_NEW";
case SimpleKind::KW_NOEXCEPT: return "KW_NOEXCEPT";
case SimpleKind::KW_NULLPTR: return "KW_NULLPTR";
case SimpleKind::KW_OPERATOR: return "KW_OPERATOR";
case SimpleKind::KW_PRIVATE: return "KW_PRIVATE";
case SimpleKind::KW_PROTECTED: return "KW_PROTECTED";
case SimpleKind::KW_PUBLIC: return "KW_PUBLIC";
case SimpleKind::KW_REGISTER: return "KW_REGISTER";
case SimpleKind::KW_REINTERPET_CAST: return "KW_REINTERPET_CAST";
case SimpleKind::KW_RETURN: return "KW_RETURN";
case SimpleKind::KW_SHORT: return "KW_SHORT";
case SimpleKind::KW_SIGNED: return "KW_SIGNED";
case SimpleKind::KW_SIZEOF: return "KW_SIZEOF";
case SimpleKind::KW_STATIC: return "KW_STATIC";
case SimpleKind::KW_STATIC_ASSERT: return "KW_STATIC_ASSERT";
case SimpleKind::KW_STATIC_CAST: return "KW_STATIC_CAST";
case SimpleKind::KW_STRUCT: return "KW_STRUCT";
case SimpleKind::KW_SWITCH: return "KW_SWITCH";
case SimpleKind::KW_TEMPLATE: return "KW_TEMPLATE";
case SimpleKind::KW_THIS: return "KW_THIS";
case SimpleKind::KW_THREAD_LOCAL: return "KW_THREAD_LOCAL";
case SimpleKind::KW_THROW: return "KW_THROW";
case SimpleKind::KW_TRUE: return "KW_TRUE";
case SimpleKind::KW_TRY: return "KW_TRY";
case SimpleKind::KW_TYPEDEF: return "KW_TYPEDEF";
case SimpleKind::KW_TYPEID: return "KW_TYPEID";
case SimpleKind::KW_TYPENAME: return "KW_TYPENAME";
case SimpleKind::KW_UNION: return "KW_UNION";
case SimpleKind::KW_UNSIGNED: return "KW_UNSIGNED";
case SimpleKind::KW_USING: return "KW_USING";
case SimpleKind::KW_VIRTUAL: return "KW_VIRTUAL";
case SimpleKind::KW_VOID: return "KW_VOID";
case SimpleKind::KW_VOLATILE: return "KW_VOLATILE";
case SimpleKind::KW_WCHAR_T: return "KW_WCHAR_T";
case SimpleKind::KW_WHILE: return "KW_WHILE";
case SimpleKind::OP_LBRACE: return "OP_LBRACE";
case SimpleKind::OP_RBRACE: return "OP_RBRACE";
case SimpleKind::OP_LSQUARE: return "OP_LSQUARE";
case SimpleKind::OP_RSQUARE: return "OP_RSQUARE";
case SimpleKind::OP_LPAREN: return "OP_LPAREN";
case SimpleKind::OP_RPAREN: return "OP_RPAREN";
case SimpleKind::OP_BOR: return "OP_BOR";
case SimpleKind::OP_XOR: return "OP_XOR";
case SimpleKind::OP_COMPL: return "OP_COMPL";
case SimpleKind::OP_AMP: return "OP_AMP";
case SimpleKind::OP_LNOT: return "OP_LNOT";
case SimpleKind::OP_SEMICOLON: return "OP_SEMICOLON";
case SimpleKind::OP_COLON: return "OP_COLON";
case SimpleKind::OP_DOTS: return "OP_DOTS";
case SimpleKind::OP_QMARK: return "OP_QMARK";
case SimpleKind::OP_COLON2: return "OP_COLON2";
case SimpleKind::OP_DOT: return "OP_DOT";
case SimpleKind::OP_DOTSTAR: return "OP_DOTSTAR";
case SimpleKind::OP_PLUS: return "OP_PLUS";
case SimpleKind::OP_MINUS: return "OP_MINUS";
case SimpleKind::OP_STAR: return "OP_STAR";
case SimpleKind::OP_DIV: return "OP_DIV";
case SimpleKind::OP_MOD: return "OP_MOD";
case SimpleKind::OP_ASS: return "OP_ASS";
case SimpleKind::OP_LT: return "OP_LT";
case SimpleKind::OP_GT: return "OP_GT";
case SimpleKind::OP_PLUSASS: return "OP_PLUSASS";
case SimpleKind::OP_MINUSASS: return "OP_MINUSASS";
case SimpleKind::OP_STARASS: return "OP_STARASS";
case SimpleKind::OP_DIVASS: return "OP_DIVASS";
case SimpleKind::OP_MODASS: return "OP_MODASS";
case SimpleKind::OP_XORASS: return "OP_XORASS";
case SimpleKind::OP_BANDASS: return "OP_BANDASS";
case SimpleKind::OP_BORASS: return "OP_BORASS";
case SimpleKind::OP_LSHIFT: return "OP_LSHIFT";
case SimpleKind::OP_RSHIFT: return "OP_RSHIFT";
case SimpleKind::OP_RSHIFTASS: return "OP_RSHIFTASS";
case SimpleKind::OP_LSHIFTASS: return "OP_LSHIFTASS";
case SimpleKind::OP_EQ: return "OP_EQ";
case SimpleKind::OP_NE: return "OP_NE";
case SimpleKind::OP_LE: return "OP_LE";
case SimpleKind::OP_GE: return "OP_GE";
case SimpleKind::OP_LAND: return "OP_LAND";
case SimpleKind::OP_LOR: return "OP_LOR";
case SimpleKind::OP_INC: return "OP_INC";
case SimpleKind::OP_DEC: return "OP_DEC";
case SimpleKind::OP_COMMA: return "OP_COMMA";
case SimpleKind::OP_ARROWSTAR: return "OP_ARROWSTAR";
case SimpleKind::OP_ARROW: return "OP_ARROW";
} return "?";
}
bool classify_simple(const std::string& spelling, SimpleKind& kind) {
 // Single-character grammar terminals are the dominant expression/header
 // path. Classify directly without hashing; the immutable bounded inventory
 // below still handles multicharacter terminals and keyword aliases.
 if (spelling.size() == 1) {
  switch (spelling[0]) {
  case '{': kind = SimpleKind::OP_LBRACE; return true;
  case '}': kind = SimpleKind::OP_RBRACE; return true;
  case '[': kind = SimpleKind::OP_LSQUARE; return true;
  case ']': kind = SimpleKind::OP_RSQUARE; return true;
  case '(': kind = SimpleKind::OP_LPAREN; return true;
  case ')': kind = SimpleKind::OP_RPAREN; return true;
  case '|': kind = SimpleKind::OP_BOR; return true;
  case '^': kind = SimpleKind::OP_XOR; return true;
  case '~': kind = SimpleKind::OP_COMPL; return true;
  case '&': kind = SimpleKind::OP_AMP; return true;
  case '!': kind = SimpleKind::OP_LNOT; return true;
  case ';': kind = SimpleKind::OP_SEMICOLON; return true;
  case ':': kind = SimpleKind::OP_COLON; return true;
  case '?': kind = SimpleKind::OP_QMARK; return true;
  case '.': kind = SimpleKind::OP_DOT; return true;
  case '+': kind = SimpleKind::OP_PLUS; return true;
  case '-': kind = SimpleKind::OP_MINUS; return true;
  case '*': kind = SimpleKind::OP_STAR; return true;
  case '/': kind = SimpleKind::OP_DIV; return true;
  case '%': kind = SimpleKind::OP_MOD; return true;
  case '=': kind = SimpleKind::OP_ASS; return true;
  case '<': kind = SimpleKind::OP_LT; return true;
  case '>': kind = SimpleKind::OP_GT; return true;
  case ',': kind = SimpleKind::OP_COMMA; return true;
  default: return false;
  }
 }
 static const std::unordered_map<std::string, SimpleKind> table = {
{"alignas", SimpleKind::KW_ALIGNAS},
{"alignof", SimpleKind::KW_ALIGNOF},
{"asm", SimpleKind::KW_ASM},
{"auto", SimpleKind::KW_AUTO},
{"bool", SimpleKind::KW_BOOL},
{"break", SimpleKind::KW_BREAK},
{"case", SimpleKind::KW_CASE},
{"catch", SimpleKind::KW_CATCH},
{"char", SimpleKind::KW_CHAR},
{"char16_t", SimpleKind::KW_CHAR16_T},
{"char32_t", SimpleKind::KW_CHAR32_T},
{"class", SimpleKind::KW_CLASS},
{"const", SimpleKind::KW_CONST},
{"constexpr", SimpleKind::KW_CONSTEXPR},
{"const_cast", SimpleKind::KW_CONST_CAST},
{"continue", SimpleKind::KW_CONTINUE},
{"decltype", SimpleKind::KW_DECLTYPE},
{"default", SimpleKind::KW_DEFAULT},
{"delete", SimpleKind::KW_DELETE},
{"do", SimpleKind::KW_DO},
{"double", SimpleKind::KW_DOUBLE},
{"dynamic_cast", SimpleKind::KW_DYNAMIC_CAST},
{"else", SimpleKind::KW_ELSE},
{"enum", SimpleKind::KW_ENUM},
{"explicit", SimpleKind::KW_EXPLICIT},
{"export", SimpleKind::KW_EXPORT},
{"extern", SimpleKind::KW_EXTERN},
{"false", SimpleKind::KW_FALSE},
{"float", SimpleKind::KW_FLOAT},
{"for", SimpleKind::KW_FOR},
{"friend", SimpleKind::KW_FRIEND},
{"goto", SimpleKind::KW_GOTO},
{"if", SimpleKind::KW_IF},
{"inline", SimpleKind::KW_INLINE},
{"int", SimpleKind::KW_INT},
{"long", SimpleKind::KW_LONG},
{"mutable", SimpleKind::KW_MUTABLE},
{"namespace", SimpleKind::KW_NAMESPACE},
{"new", SimpleKind::KW_NEW},
{"noexcept", SimpleKind::KW_NOEXCEPT},
{"nullptr", SimpleKind::KW_NULLPTR},
{"operator", SimpleKind::KW_OPERATOR},
{"private", SimpleKind::KW_PRIVATE},
{"protected", SimpleKind::KW_PROTECTED},
{"public", SimpleKind::KW_PUBLIC},
{"register", SimpleKind::KW_REGISTER},
{"reinterpret_cast", SimpleKind::KW_REINTERPET_CAST},
{"return", SimpleKind::KW_RETURN},
{"short", SimpleKind::KW_SHORT},
{"signed", SimpleKind::KW_SIGNED},
{"sizeof", SimpleKind::KW_SIZEOF},
{"static", SimpleKind::KW_STATIC},
{"static_assert", SimpleKind::KW_STATIC_ASSERT},
{"static_cast", SimpleKind::KW_STATIC_CAST},
{"struct", SimpleKind::KW_STRUCT},
{"switch", SimpleKind::KW_SWITCH},
{"template", SimpleKind::KW_TEMPLATE},
{"this", SimpleKind::KW_THIS},
{"thread_local", SimpleKind::KW_THREAD_LOCAL},
{"throw", SimpleKind::KW_THROW},
{"true", SimpleKind::KW_TRUE},
{"try", SimpleKind::KW_TRY},
{"typedef", SimpleKind::KW_TYPEDEF},
{"typeid", SimpleKind::KW_TYPEID},
{"typename", SimpleKind::KW_TYPENAME},
{"union", SimpleKind::KW_UNION},
{"unsigned", SimpleKind::KW_UNSIGNED},
{"using", SimpleKind::KW_USING},
{"virtual", SimpleKind::KW_VIRTUAL},
{"void", SimpleKind::KW_VOID},
{"volatile", SimpleKind::KW_VOLATILE},
{"wchar_t", SimpleKind::KW_WCHAR_T},
{"while", SimpleKind::KW_WHILE},
{"{", SimpleKind::OP_LBRACE},
{"<%", SimpleKind::OP_LBRACE},
{"}", SimpleKind::OP_RBRACE},
{"%>", SimpleKind::OP_RBRACE},
{"[", SimpleKind::OP_LSQUARE},
{"<:", SimpleKind::OP_LSQUARE},
{"]", SimpleKind::OP_RSQUARE},
{":>", SimpleKind::OP_RSQUARE},
{"(", SimpleKind::OP_LPAREN},
{")", SimpleKind::OP_RPAREN},
{"|", SimpleKind::OP_BOR},
{"bitor", SimpleKind::OP_BOR},
{"^", SimpleKind::OP_XOR},
{"xor", SimpleKind::OP_XOR},
{"~", SimpleKind::OP_COMPL},
{"compl", SimpleKind::OP_COMPL},
{"&", SimpleKind::OP_AMP},
{"bitand", SimpleKind::OP_AMP},
{"!", SimpleKind::OP_LNOT},
{"not", SimpleKind::OP_LNOT},
{";", SimpleKind::OP_SEMICOLON},
{":", SimpleKind::OP_COLON},
{"...", SimpleKind::OP_DOTS},
{"?", SimpleKind::OP_QMARK},
{"::", SimpleKind::OP_COLON2},
{".", SimpleKind::OP_DOT},
{".*", SimpleKind::OP_DOTSTAR},
{"+", SimpleKind::OP_PLUS},
{"-", SimpleKind::OP_MINUS},
{"*", SimpleKind::OP_STAR},
{"/", SimpleKind::OP_DIV},
{"%", SimpleKind::OP_MOD},
{"=", SimpleKind::OP_ASS},
{"<", SimpleKind::OP_LT},
{">", SimpleKind::OP_GT},
{"+=", SimpleKind::OP_PLUSASS},
{"-=", SimpleKind::OP_MINUSASS},
{"*=", SimpleKind::OP_STARASS},
{"/=", SimpleKind::OP_DIVASS},
{"%=", SimpleKind::OP_MODASS},
{"^=", SimpleKind::OP_XORASS},
{"xor_eq", SimpleKind::OP_XORASS},
{"&=", SimpleKind::OP_BANDASS},
{"and_eq", SimpleKind::OP_BANDASS},
{"|=", SimpleKind::OP_BORASS},
{"or_eq", SimpleKind::OP_BORASS},
{"<<", SimpleKind::OP_LSHIFT},
{">>", SimpleKind::OP_RSHIFT},
{">>=", SimpleKind::OP_RSHIFTASS},
{"<<=", SimpleKind::OP_LSHIFTASS},
{"==", SimpleKind::OP_EQ},
{"!=", SimpleKind::OP_NE},
{"not_eq", SimpleKind::OP_NE},
{"<=", SimpleKind::OP_LE},
{">=", SimpleKind::OP_GE},
{"&&", SimpleKind::OP_LAND},
{"and", SimpleKind::OP_LAND},
{"||", SimpleKind::OP_LOR},
{"or", SimpleKind::OP_LOR},
{"++", SimpleKind::OP_INC},
{"--", SimpleKind::OP_DEC},
{",", SimpleKind::OP_COMMA},
{"->*", SimpleKind::OP_ARROWSTAR},
{"->", SimpleKind::OP_ARROW},
};
 auto found = table.find(spelling);
 if (found == table.end()) return false;
 kind = found->second; return true;
}
}

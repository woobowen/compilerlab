%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
digit [0-9]
letter [a-zA-Z]

%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* 第1部分：基本规则 */

 /* 1.1 保留字 （Reserved Words） */
"array" {adjust(); return Parser::ARRAY;}

 /* TODO: Put your lab2 code here */
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}

/* 1.2 标点符号（Punctuation Symbols）*/
/* 多字符运算符优先匹配 */
":=" {adjust(); return Parser::ASSIGN;}
"<>" {adjust(); return Parser::NEQ;}
"<=" {adjust(); return Parser::LE;}
">=" {adjust(); return Parser::GE;}

/* 单字符标点和运算符 */
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<" {adjust(); return Parser::LT;}
">" {adjust(); return Parser::GT;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}

/* 1.3 标识符（identifiers）*/
/* 以字母开头，由字母、数字、下划线组成 */
{letter}({letter}|{digit}|_)* {adjust(); return Parser::ID;}

/* 1.4 整数字面量（integer literals）*/
/* 一个或多个十进制数字组成的序列 */
{digit}+ {adjust(); return Parser::INT;}

/* 第2部分：注释处理 */
/* 进入注释 */
"/*" {adjust(); comment_level_ = 1; begin(StartCondition_::COMMENT);}
/* 嵌套注释 */
<COMMENT>"/*" {adjust(); comment_level_++;}
/* 层数减到0时，关闭注释 */
<COMMENT>"*/" {adjust(); comment_level_--; if(comment_level_ == 0) begin(StartCondition_::INITIAL);}
/* 注释中遇到换行时，更新行号 */
<COMMENT>\n {adjust(); errormsg_->Newline();}
/* 注释中其他字符，跳过 */
<COMMENT>. {adjust();}

/* 第3部分：字符串处理 */
/* 进入字符串 */
\" {adjust(); string_buf_.clear(); begin(StartCondition_::STR);}
/* 字符串结束：把构建的真实字符串设为 matched() 的返回值 */
<STR>\" {adjustStr(); setMatched(string_buf_); begin(StartCondition_::INITIAL); return Parser::STRING;}
/* 转义字符 */
<STR>\\n {adjustStr(); string_buf_ += '\n';}
<STR>\\t {adjustStr(); string_buf_ += '\t';}
<STR>\\\\ {adjustStr(); string_buf_ += '\\';}
<STR>\\\" {adjustStr(); string_buf_ += '"';}
/* \ddd：3位十进制数字表示的ASCII码 */
<STR>\\[0-9]{3} {adjustStr(); string_buf_ += static_cast<char>(atoi(matched().c_str() + 1));}
/* \^c：控制字符，字符c减去'@'可得对应的控制字符ASCII码 */
<STR>\\\^. {adjustStr(); string_buf_ += static_cast<char>(matched()[2] - '@');}
/* \f___f\：格式化字符，跳过空白字符 */
/* STR状态下，\后面跟空白字符（若为换行，更新行号），进入 IGNORE 状态*/
<STR>\\[ \t\n\f] {adjustStr(); if(matched()[1] == '\n') errormsg_->Newline(); begin(StartCondition_::IGNORE);}
/* IGNORE状态下，遇到\，回到STR状态 */
<IGNORE>\\ {adjustStr(); begin(StartCondition_::STR);}
/* IGNORE状态下，跳过空白字符（若为换行，更新行号） */
<IGNORE>[ \t\n\f] {adjustStr(); if(matched()[0] == '\n') errormsg_->Newline();}
/* IGNORE状态下，遇到其他字符，报错 */
<IGNORE>. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
/* STR状态下，其他普通字符直接添加到 string_buf_ */
<STR>. {adjustStr(); string_buf_ += matched();}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
 /* 兜底规则：处理所有未被前面规则匹配的字符，报错并继续扫描 */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}

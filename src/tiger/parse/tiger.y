%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
  }

%token <sym> ID
%token <sval> STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  ASSIGN
  ARRAY IF WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

/* token priority */
/* TODO: Put your lab3 code here */
/* 优先级与结合性：越靠后的优先级越高*/
/* 解决 if-then-else 的悬挂 else 冲突：
遇到 IF exp THEN exp 后的 ELSE 优先 shift 而不是 reduce，使 else 绑定到最近的 then */
%right THEN ELSE  
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
/* 一元负号优先级最高 */
%right UMINUS  

%type <exp> exp expseq ifexp whileexp callexp recordexp
%type <explist> actuals nonemptyactuals sequencing sequencing_exps
%type <var> lvalue
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
program:  exp  {absyn_tree_ = std::make_unique<absyn::AbsynTree>($1);};

/* TODO: Put your lab3 code here */
/* 语法规则右侧的动作：构造抽象语法树节点，并把构造结果赋值给 $$，供上层规则使用。*/
exp:
    INT
      {
        $$ = new absyn::IntExp(scanner_.GetTokPos(), $1);
      }
  | STRING
      {
        $$ = new absyn::StringExp(scanner_.GetTokPos(), $1);
      }
  | NIL
      {
        $$ = new absyn::NilExp(scanner_.GetTokPos());
      }
  | BREAK
      {
        $$ = new absyn::BreakExp(scanner_.GetTokPos());
      }
  | LPAREN RPAREN  /* 空的 () */
      {
        $$ = new absyn::VoidExp(scanner_.GetTokPos());
      }
  | lvalue  /* 把子规则 lvalue 构造的 Var 直接传递上来，包装成 VarExp */
      {
        $$ = new absyn::VarExp(scanner_.GetTokPos(), $1);
      }
  | lvalue ASSIGN exp  
      {
        $$ = new absyn::AssignExp(scanner_.GetTokPos(), $1, $3);
      }
  | callexp  /* 把子规则 callexp 构造的 CallExp 直接传递上来 */
      {
        $$ = $1;
      }
  | recordexp
      {
        $$ = $1;
      }
  | ifexp
      {
        $$ = $1;
      }
  | whileexp
      {
        $$ = $1;
      }
  | FOR ID ASSIGN exp TO exp DO exp
      {
        $$ = new absyn::ForExp(scanner_.GetTokPos(), $2, $4, $6, $8);
      }
  | LET decs IN expseq END
      {
        $$ = new absyn::LetExp(scanner_.GetTokPos(), $2, $4);
      }
  | lvalue OF exp  /* 数组创建：a[sz] of init */
      {
        /* 把 $1（Var*）尝试当成 SubscriptVar，表示 a[sz] 部分*/
        auto *subscript = dynamic_cast<absyn::SubscriptVar *>($1);
        /* 提取出 a 和 sz 来构造 ArrayExp:
           - a 是一个 SimpleVar，表示数组名；
           - sz 是下标表达式；
           - init 是数组初始值表达式 $3。*/
        auto *base = subscript ? dynamic_cast<absyn::SimpleVar *>(subscript->var_) : nullptr;
        $$ = new absyn::ArrayExp(scanner_.GetTokPos(), base->sym_, subscript->subscript_, $3);
        // 把 subscript 中的子指针置空（所有权已转移给 ArrayExp），避免重复释放
        subscript->var_ = nullptr;
        subscript->subscript_ = nullptr;
        // 删除临时 subscript 对象
        delete subscript;
      }
  | LPAREN sequencing RPAREN  /* 序列表达式：(a; b; c) —— sequencing 负责把 a、b、c 收集成一个 ExpList */
      {
        /* 如果只有一个表达式，就直接返回该表达式；
        如果是多个表达式，才构造 SeqExp 来包含它们。*/
        $$ = $2->GetList().size() == 1 ? $2->GetList().front()
                                       : new absyn::SeqExp(scanner_.GetTokPos(), $2);
      }
  /* 二元算术/关系/逻辑运算：统一构造 OpExp */
  | exp PLUS exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::PLUS_OP, $1, $3);
      }
  | exp MINUS exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::MINUS_OP, $1, $3);
      }
  | exp TIMES exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::TIMES_OP, $1, $3);
      }
  | exp DIVIDE exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::DIVIDE_OP, $1, $3);
      }
  | exp EQ exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::EQ_OP, $1, $3);
      }
  | exp NEQ exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::NEQ_OP, $1, $3);
      }
  | exp LT exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::LT_OP, $1, $3);
      }
  | exp LE exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::LE_OP, $1, $3);
      }
  | exp GT exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::GT_OP, $1, $3);
      }
  | exp GE exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::GE_OP, $1, $3);
      }
  | exp AND exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::AND_OP, $1, $3);
      }
  | exp OR exp
      {
        $$ = new absyn::OpExp(scanner_.GetTokPos(), absyn::OR_OP, $1, $3);
      }
  | MINUS exp %prec UMINUS  /* 一元负号：使用 %prec UMINUS 指定最高优先级 */
      {
        /* 把 -exp 表示为 0 - exp 来复用 OpExp */
        $$ = new absyn::OpExp(
            scanner_.GetTokPos(),
            absyn::MINUS_OP,
            new absyn::IntExp(scanner_.GetTokPos(), 0),
            $2);
      }
  ;

ifexp:  /* else 分支可选 */
    IF exp THEN exp
      {
        $$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, nullptr);
      }
  | IF exp THEN exp ELSE exp
      {
        $$ = new absyn::IfExp(scanner_.GetTokPos(), $2, $4, $6);
      }
  ;

whileexp: 
    WHILE exp DO exp
      {
        $$ = new absyn::WhileExp(scanner_.GetTokPos(), $2, $4);
      }
  ;

expseq:  /* LET ... IN ... END 中位于 IN 和 END 之间的表达式序列 */
    /* 空序列：返回 VoidExp */
      {
        $$ = new absyn::VoidExp(scanner_.GetTokPos());
      }
  | sequencing
      {
        $$ = new absyn::SeqExp(scanner_.GetTokPos(), $1);
      }
  ;

/* 把分号序列 (a; b; c) 收集为 ExpList（按源顺序）；上层决定是否用 SeqExp 包装 */
sequencing:
    exp sequencing_exps
      {
        $$ = $2->Prepend($1);  /* 把第一个表达式 加入 ExpList 中，得到完整的 sequencing_exps。*/
      }
  ;

sequencing_exps:  
    /* 空序列：构造一个空的 ExpList */
      {
        $$ = new absyn::ExpList();
      }
  | SEMICOLON exp sequencing_exps  /* 遇到分号，说明后面还有表达式，把 exp 加入 ExpList 中继续收集。*/
      {
        $$ = $3->Prepend($2);
      }
  ;

/* 左值 */
lvalue:  
    ID  /* 简单变量（标识符） */
      {
        $$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);
      }
  | lvalue DOT ID  /* 字段访问：a.b */
      {
        $$ = new absyn::FieldVar(scanner_.GetTokPos(), $1, $3);
      }
  | lvalue LBRACK exp RBRACK  /* 数组访问：a[i] */
      {
        $$ = new absyn::SubscriptVar(scanner_.GetTokPos(), $1, $3);
      }
  ;

/* 函数调用：ID(actuals) */
callexp:  
    ID LPAREN actuals RPAREN
      {
        $$ = new absyn::CallExp(scanner_.GetTokPos(), $1, $3);
      }
  ;

actuals:  /* actuals: a, b, c */
    /* 空：没有参数，构造一个空的 ExpList */
      {
        $$ = new absyn::ExpList();
      }
  | nonemptyactuals 
      {
        $$ = $1;  /* 把 nonemptyactuals 构造的 ExpList 直接传递上来 */
      }
  ;

nonemptyactuals:
    exp  /* 最后一个参数：构造一个只有一个表达式的 ExpList */
      {
        $$ = new absyn::ExpList($1);
      }
  | exp COMMA nonemptyactuals  /* 多个参数：把第一个参数加入 ExpList 中，继续收集剩余参数。*/
      {
        $$ = $3->Prepend($1);
      }
  ;

/* 记录创建：TypeName{rec} */
recordexp:
    ID LBRACE rec RBRACE
      {
        $$ = new absyn::RecordExp(scanner_.GetTokPos(), $1, $3);
      }
  ;

rec:  /* rec: a=exp, b=exp, c=exp */
    /* 空字段：构造一个空的 EFieldList */
      {
        $$ = new absyn::EFieldList();
      }
  | rec_nonempty
      {
        $$ = $1;  /* 把 rec_nonempty 构造的 EFieldList 直接传递上来 */
      }
  ;

rec_nonempty:
    rec_one  /* 最后一个字段：构造一个只有一个字段的 EFieldList */
      {
        $$ = new absyn::EFieldList($1);
      }
  | rec_one COMMA rec_nonempty  /* 多个字段：把第一个字段加入 EFieldList 中，继续收集剩余字段。*/
      {
        $$ = $3->Prepend($1);
      }
  ;

rec_one:
    ID EQ exp  /* 最后把字段名和对应表达式构造为一个 EField */
      {
        $$ = new absyn::EField($1, $3);
      }
  ;

/* LET ... IN ... END 中位于 LET 和 IN 之间的声明序列 */
decs:
    /* 空序列：没有声明，构造一个空的 DecList */
      {
        $$ = new absyn::DecList();
      }
  | decs_nonempty
      {
        $$ = $1;
      }
  ;

decs_nonempty:
    decs_nonempty_s  /* 最后一个声明：构造一个只有一个声明的 DecList */
      {
        $$ = new absyn::DecList($1);
      }
  | decs_nonempty_s decs_nonempty  /* 多个声明：把第一个声明加入 DecList 中，继续收集剩余声明。*/
      {
        $$ = $2->Prepend($1);
      }
  ;

/* 声明：变量声明、类型声明、函数声明 */
decs_nonempty_s:  
    vardec
      {
        $$ = $1;
      }
  | tydec
      {
        $$ = new absyn::TypeDec(scanner_.GetTokPos(), $1);
      }
  | fundec
      {
        $$ = new absyn::FunctionDec(scanner_.GetTokPos(), $1);
      }
  ;

/* 变量声明：var a := exp 或 var a: type := exp */
vardec:  
    VAR ID ASSIGN exp
      {
        $$ = new absyn::VarDec(scanner_.GetTokPos(), $2, nullptr, $4);
      }
  | VAR ID COLON ID ASSIGN exp
      {
        $$ = new absyn::VarDec(scanner_.GetTokPos(), $2, $4, $6);
      }
  ;

/* 类型声明：连续多个类型声明合并成一个 NameAndTyList */
tydec:  
    tydec_one 
      {
        $$ = new absyn::NameAndTyList($1);
      }
  | tydec_one tydec  
      {
        $$ = $2->Prepend($1);
      }
  ;

tydec_one:  /* 单个类型声明：type a = ty */
    TYPE ID EQ ty
      {
        $$ = new absyn::NameAndTy($2, $4);
      }
  ;

/* 类型：基本类型、记录类型、数组类型 */
ty:
    ID
      {
        $$ = new absyn::NameTy(scanner_.GetTokPos(), $1);
      }
  | LBRACE tyfields RBRACE
      {
        $$ = new absyn::RecordTy(scanner_.GetTokPos(), $2);
      }
  | ARRAY OF ID
      {
        $$ = new absyn::ArrayTy(scanner_.GetTokPos(), $3);
      }
  ;

tyfields:  /* tyfields: a: type, b: type, c: type */
    /* 空字段列表：构造一个空的 FieldList */
      {
        $$ = new absyn::FieldList();
      }
  | tyfields_nonempty
      {
        $$ = $1;
      }
  ;

tyfields_nonempty:
    tyfield  
      {
        $$ = new absyn::FieldList($1);
      }
  | tyfield COMMA tyfields_nonempty  
      {
        $$ = $3->Prepend($1);
      }
  ;

tyfield:  /* 单个字段：a : type */
    ID COLON ID
      {
        $$ = new absyn::Field(scanner_.GetTokPos(), $1, $3);
      }
  ;

/* 函数声明：连续多个函数声明合并成一个 FunDecList */
fundec:
    fundec_one
      {
        $$ = new absyn::FunDecList($1);
      }
  | fundec_one fundec
      {
        $$ = $2->Prepend($1);
      }
  ;

fundec_one:  /* 单个函数声明：function f(params) = exp 或 function f(params): returnType = exp */
    FUNCTION ID LPAREN tyfields RPAREN EQ exp
      {
        $$ = new absyn::FunDec(scanner_.GetTokPos(), $2, $4, nullptr, $7);
      }
  | FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp
      {
        $$ = new absyn::FunDec(scanner_.GetTokPos(), $2, $4, $7, $9);
      }
  ;

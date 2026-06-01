#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  this->root_->Traverse(env, 0);
}

// 对于变量而言，如果变量被访问的位置比其被定义的位置嵌套层数深
// 那么，我们需要将escape标志置为true
void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  auto entry=env->Look(this->sym_);
  if(entry && entry->depth_<depth)
  {
    *(entry->escape_)=true;
  }
}

// 对访问记录域的表达式而言，我们只关心记录本身是否为escape的即可
void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
}

// 对数组下标访问而言，我们关心这个数组是否为escape的
// 同时，下标作为一个表达式，也可能访问了某些需要标记为escape的变量
void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
  this->subscript_->Traverse(env,depth);
}

// 变量表达式，我们只需要关心这个变量是否为escape的即可
void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
}

// nil无需判断
void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* End for lab5 code */
}

// 整型常数无需判断
void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* End for lab5 code */
}

// 字符串无需判断
void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* End for lab5 code */
}

// 对于调用而言，我们关心其实参中是否有escape对象的访问
void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  for(auto arg:this->args_->GetList())
    arg->Traverse(env,depth);
}

// 对于二元运算符，我们要对其两侧的表达式进行判断
void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->left_->Traverse(env,depth);
  this->right_->Traverse(env,depth);
}

// 对于记录表达式，每一个域的赋值表达式我们都需要进行判断
void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  for(auto field:this->fields_->GetList())
    field->exp_->Traverse(env,depth);
}

// 对于序列表达式，我们对每个单独的表达式都需要进行判断
void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  for(auto exp:this->seq_->GetList())
    exp->Traverse(env,depth);
}

// 对于赋值语句，考虑变量和表达式
void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->var_->Traverse(env,depth);
  this->exp_->Traverse(env,depth);
}

// if语句需要考虑条件表达式和分支
void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->test_->Traverse(env,depth);
  this->then_->Traverse(env,depth);
  if(this->elsee_)
    this->elsee_->Traverse(env,depth);
}

// while和if同理
void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  this->test_->Traverse(env,depth);
  this->body_->Traverse(env,depth);
  env->EndScope();
}

// for也是一样
void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->lo_->Traverse(env,depth);
  this->hi_->Traverse(env,depth);
  env->BeginScope();
  // 在env中新建迭代变量的项
  this->escape_=false;
  env->Enter(this->var_,new esc::EscapeEntry(depth,&(this->escape_)));
  this->body_->Traverse(env,depth);
  env->EndScope();
}

// break无需判断
void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* End for lab5 code */
}

// let语句的两部分都需要进行判断
void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  env->BeginScope();
  for(auto dec:this->decs_->GetList())
    dec->Traverse(env,depth);
  this->body_->Traverse(env,depth);
  env->EndScope();
}

// 数组表达式，我们关心用来指定数组大小和初值的表达式
void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->size_->Traverse(env,depth);
  this->init_->Traverse(env,depth);
}

// 空表达式不做处理
void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* End for lab5 code */
}

// 函数定义，我们在函数体内进行分析时需要增加一层深度
void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  for(auto fundec:this->functions_->GetList())
  {
    // 首先处理形参的声明
    env->BeginScope();
    for(auto param:fundec->params_->GetList()){
      param->escape_=false;
      env->Enter(param->name_,new esc::EscapeEntry(depth+1,&(param->escape_)));
    }
    fundec->body_->Traverse(env,depth+1);
    env->EndScope();
  }
}

// 变量定义，只需要将其加入env中即可
void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  this->escape_=false;
  env->Enter(this->var_,new esc::EscapeEntry(depth,&(this->escape_)));
  this->init_->Traverse(env,depth);
}

// 类型定义，无需处理
void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  /* End for lab5 code */
}

} // namespace absyn

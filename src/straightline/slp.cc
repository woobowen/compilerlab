#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  // "stm1; stm2" - 递归调用stm1和stm2的MaxArgs，取最大值
  return std::max(stm1->MaxArgs(), stm2->MaxArgs());
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  // "stm1; stm2"：
  // 1. 用当前环境t解释stm1，得到新环境t1
  Table *t1 = stm1->Interp(t);
  // 2. 用新环境t1解释stm2，得到新环境t2
  return stm2->Interp(t1);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  // "ID := exp" - 递归调用exp的MaxArgs
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  // "ID := exp"：
  // 1. 用当前环境t解释exp，得到exp的值和新环境t1
  IntAndTable *expResult = exp->InterpExp(t);
  // 2. 把ID和exp的值更新到环境t1中，得到新环境t2
  return expResult->t->Update(id, expResult->i);
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  // "print(expList)" : 取最大值
  // 1. expList的NumExps() - 当前这个print语句的参数数量
  // 2. expList的MaxArgs() - expList中print语句的最大参数数量（因为expList中可能嵌套了print语句）
  return std::max(exps->NumExps(), exps->MaxArgs());
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  // "print(expList)" : 
  // 1. 调用exps->Interp(t) —— 会递归求值并打印出expList中的每个exp的值并更新环境
  IntAndTable *expListResult = exps->Interp(t);
  // 2. 从返回的IntAndTable中取出环境
  return expListResult->t;
}

int IdExp::MaxArgs() const {
  return 0; // should be 0 for none print stm testcase
}

IntAndTable *IdExp::InterpExp(Table *t) const {
  return new IntAndTable(t->Lookup(id), t);
}

int NumExp::MaxArgs() const {
  return 0; // should be 0 for none print stm testcase
}

IntAndTable *NumExp::InterpExp(Table *t) const {
  return new IntAndTable(num, t);
}

int OpExp::MaxArgs() const {
  int args1 = left->MaxArgs();
  int args2 = right->MaxArgs();
  // std::cout << args1 << " " << args2 << std::endl;
  return args1 > args2 ? args1 : args2;
}

IntAndTable *OpExp::InterpExp(Table *t) const {
  IntAndTable *expValue1 = left->InterpExp(t);
  IntAndTable *expValue2 = right->InterpExp(t);
  switch (oper) {
  case PLUS:
    return new IntAndTable(expValue1->i + expValue2->i, expValue2->t);
  case MINUS:
    return new IntAndTable(expValue1->i - expValue2->i, expValue2->t);
  case TIMES:
    return new IntAndTable(expValue1->i * expValue2->i, expValue2->t);
  case DIV:
    return new IntAndTable(expValue1->i / expValue2->i, expValue2->t);
  default: // add default for robustness
    break;
  }
  return nullptr;
}

int EseqExp::MaxArgs() const {
  int args1 = stm->MaxArgs();
  int args2 = exp->MaxArgs();
  return args1 > args2 ? args1 : args2;
}

IntAndTable *EseqExp::InterpExp(Table *t) const {
  return exp->InterpExp(stm->Interp(t));
}

int PairExpList::MaxArgs() const {
  int num1 = exp->MaxArgs();
  int num2 = tail->MaxArgs();
  return num1 > num2 ? num1 : num2;
}

int PairExpList::NumExps() const { return 1 + tail->NumExps(); }

IntAndTable *PairExpList::Interp(Table *t) const {
  IntAndTable *temp_table = exp->InterpExp(t);
  printf("%d ", temp_table->i);
  return tail->Interp(temp_table->t);
}

int LastExpList::MaxArgs() const { return exp->MaxArgs(); }

int LastExpList::NumExps() const { return 1; }

IntAndTable *LastExpList::Interp(Table *t) const {
  // return exp->InterpExp(t);
  IntAndTable *int_and_table = exp->InterpExp(t);
  printf("%d\n", int_and_table->i);
  return int_and_table;
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
} // namespace A
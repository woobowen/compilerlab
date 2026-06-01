#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

#include <unordered_set>

namespace {

bool IsInt(type::Ty *ty) {
  return dynamic_cast<type::IntTy *>(ty ? ty->ActualTy() : nullptr) != nullptr;
}

bool IsString(type::Ty *ty) {
  return dynamic_cast<type::StringTy *>(ty ? ty->ActualTy() : nullptr) != nullptr;
}

bool IsVoid(type::Ty *ty) {
  return dynamic_cast<type::VoidTy *>(ty ? ty->ActualTy() : nullptr) != nullptr;
}

bool IsNil(type::Ty *ty) {
  return dynamic_cast<type::NilTy *>(ty ? ty->ActualTy() : nullptr) != nullptr;
}

type::Ty *Actual(type::Ty *ty) { return ty ? ty->ActualTy() : type::IntTy::Instance(); }

void RequireInt(type::Ty *ty, int pos, err::ErrorMsg *errormsg) {
  if (!IsInt(ty))
    errormsg->Error(pos, "integer required");
}

bool SameType(type::Ty *actual, type::Ty *expected) {
  return actual && expected && actual->IsSameType(expected);
}

bool IsReadOnlySimpleVar(const absyn::Var *var, env::VEnvPtr venv) {
  auto simple = dynamic_cast<const absyn::SimpleVar *>(var);
  if (!simple)
    return false;
  auto entry = dynamic_cast<env::VarEntry *>(venv->Look(simple->sym_));
  return entry && entry->readonly_;
}

} // namespace

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto entry = dynamic_cast<env::VarEntry *>(venv->Look(sym_));
  if (!entry) {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().c_str());
    return type::IntTy::Instance();
  }
  return entry->ty_;
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto record = dynamic_cast<type::RecordTy *>(
      Actual(var_->SemAnalyze(venv, tenv, labelcount, errormsg)));
  if (!record) {
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }
  for (auto field : record->fields_->GetList()) {
    if (field->name_ == sym_)
      return field->ty_;
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  auto array = dynamic_cast<type::ArrayTy *>(
      Actual(var_->SemAnalyze(venv, tenv, labelcount, errormsg)));
  if (!array) {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
  RequireInt(subscript_->SemAnalyze(venv, tenv, labelcount, errormsg),
             subscript_->pos_, errormsg);
  return array->ty_;
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  auto fun = dynamic_cast<env::FunEntry *>(venv->Look(func_));
  if (!fun) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return type::IntTy::Instance();
  }

  size_t formal_count = fun->formals_->GetList().size();
  size_t actual_count = args_->GetList().size();
  if (actual_count < formal_count) {
    errormsg->Error(pos_, "too few params in function %s", func_->Name().c_str());
  } else if (actual_count > formal_count) {
    errormsg->Error(pos_, "too many params in function %s", func_->Name().c_str());
  }

  auto formal = fun->formals_->GetList().begin();
  auto actual = args_->GetList().begin();
  for (; formal != fun->formals_->GetList().end() &&
         actual != args_->GetList().end();
       ++formal, ++actual) {
    type::Ty *actual_ty = (*actual)->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!SameType(actual_ty, *formal))
      errormsg->Error((*actual)->pos_, "para type mismatch");
  }
  for (; actual != args_->GetList().end(); ++actual)
    (*actual)->SemAnalyze(venv, tenv, labelcount, errormsg);
  return fun->result_;
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *left = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *right = right_->SemAnalyze(venv, tenv, labelcount, errormsg);
  switch (oper_) {
  case PLUS_OP:
  case MINUS_OP:
  case TIMES_OP:
  case DIVIDE_OP:
  case AND_OP:
  case OR_OP:
    RequireInt(left, left_->pos_, errormsg);
    RequireInt(right, right_->pos_, errormsg);
    break;
  case LT_OP:
  case LE_OP:
  case GT_OP:
  case GE_OP:
    if (!((IsInt(left) && IsInt(right)) || (IsString(left) && IsString(right))))
      errormsg->Error(pos_, "same type required");
    break;
  case EQ_OP:
  case NEQ_OP:
    if (!SameType(left, right))
      errormsg->Error(pos_, "same type required");
    break;
  default:
    break;
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  auto record = dynamic_cast<type::RecordTy *>(Actual(tenv->Look(typ_)));
  if (!record) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::IntTy::Instance();
  }
  auto expected = record->fields_->GetList().begin();
  auto actual = fields_->GetList().begin();
  for (; expected != record->fields_->GetList().end() &&
         actual != fields_->GetList().end();
       ++expected, ++actual) {
    if ((*expected)->name_ != (*actual)->name_)
      errormsg->Error(pos_, "field %s doesn't exist",
                      (*actual)->name_->Name().c_str());
    type::Ty *actual_ty =
        (*actual)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!SameType(actual_ty, (*expected)->ty_))
      errormsg->Error((*actual)->exp_->pos_, "type mismatch");
  }
  if (expected != record->fields_->GetList().end() ||
      actual != fields_->GetList().end())
    errormsg->Error(pos_, "type mismatch");
  return tenv->Look(typ_);
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *result = type::VoidTy::Instance();
  for (auto exp : seq_->GetList())
    result = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  return result;
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  RequireInt(test_->SemAnalyze(venv, tenv, labelcount, errormsg), test_->pos_,
             errormsg);
  type::Ty *then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (elsee_) {
    type::Ty *else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!SameType(then_ty, else_ty))
      errormsg->Error(pos_, "then exp and else exp type mismatch");
    return then_ty;
  }
  if (!IsVoid(then_ty))
    errormsg->Error(pos_, "if-then exp's body must produce no value");
  return type::VoidTy::Instance();
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  RequireInt(test_->SemAnalyze(venv, tenv, labelcount, errormsg), test_->pos_,
             errormsg);
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!IsVoid(body_ty))
    errormsg->Error(body_->pos_, "while body must produce no value");
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  RequireInt(lo_->SemAnalyze(venv, tenv, labelcount, errormsg), lo_->pos_,
             errormsg);
  if (!IsInt(hi_->SemAnalyze(venv, tenv, labelcount, errormsg)))
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  venv->BeginScope();
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (!IsVoid(body_ty))
    errormsg->Error(body_->pos_, "while body must produce no value");
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  if (labelcount <= 0)
    errormsg->Error(pos_, "break is not inside any loop");
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  venv->BeginScope();
  tenv->BeginScope();
  for (auto dec : decs_->GetList())
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  tenv->EndScope();
  venv->EndScope();
  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  auto array = dynamic_cast<type::ArrayTy *>(Actual(tenv->Look(typ_)));
  if (!array) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::IntTy::Instance();
  }
  RequireInt(size_->SemAnalyze(venv, tenv, labelcount, errormsg), size_->pos_,
             errormsg);
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!SameType(init_ty, array->ty_))
    errormsg->Error(init_->pos_, "type mismatch");
  return tenv->Look(typ_);
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  return type::VoidTy::Instance();
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  if (IsReadOnlySimpleVar(var_, venv))
    errormsg->Error(pos_, "loop variable can't be assigned");
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!SameType(exp_ty, var_ty))
    errormsg->Error(pos_, "unmatched assign exp");
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  std::unordered_set<sym::Symbol *> names;
  for (auto fun : functions_->GetList()) {
    if (!names.insert(fun->name_).second) {
      errormsg->Error(fun->pos_, "two functions have the same name");
      continue;
    }
    type::Ty *result = fun->result_ ? tenv->Look(fun->result_)
                                    : type::VoidTy::Instance();
    if (!result) {
      errormsg->Error(fun->pos_, "undefined type %s",
                      fun->result_->Name().c_str());
      result = type::IntTy::Instance();
    }
    venv->Enter(fun->name_, new env::FunEntry(
                                fun->params_->MakeFormalTyList(tenv, errormsg),
                                result));
  }

  for (auto fun : functions_->GetList()) {
    auto entry = dynamic_cast<env::FunEntry *>(venv->Look(fun->name_));
    venv->BeginScope();
    auto formal = entry->formals_->GetList().begin();
    for (auto field : fun->params_->GetList()) {
      type::Ty *formal_ty = *formal ? *formal : type::IntTy::Instance();
      venv->Enter(field->name_, new env::VarEntry(formal_ty));
      ++formal;
    }
    type::Ty *body_ty = fun->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!SameType(body_ty, entry->result_)) {
      if (IsVoid(entry->result_))
        errormsg->Error(fun->body_->pos_, "procedure returns value");
      else
        errormsg->Error(fun->body_->pos_, "type mismatch");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!typ_) {
    if (IsNil(init_ty)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
      init_ty = type::IntTy::Instance();
    }
    venv->Enter(var_, new env::VarEntry(init_ty));
    return;
  }
  type::Ty *type_ty = tenv->Look(typ_);
  if (!type_ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    type_ty = type::IntTy::Instance();
  }
  if (!SameType(init_ty, type_ty))
    errormsg->Error(pos_, "type mismatch");
  venv->Enter(var_, new env::VarEntry(type_ty));
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  std::unordered_set<sym::Symbol *> names;
  for (auto type_dec : types_->GetList()) {
    if (!names.insert(type_dec->name_).second)
      errormsg->Error(type_dec->ty_->pos_, "two types have the same name");
    tenv->Enter(type_dec->name_, new type::NameTy(type_dec->name_, nullptr));
  }

  for (auto type_dec : types_->GetList()) {
    auto name_ty = dynamic_cast<type::NameTy *>(tenv->Look(type_dec->name_));
    name_ty->ty_ = type_dec->ty_->SemAnalyze(tenv, errormsg);
  }

  for (auto type_dec : types_->GetList()) {
    std::unordered_set<type::Ty *> seen;
    type::Ty *ty = tenv->Look(type_dec->name_);
    while (auto name_ty = dynamic_cast<type::NameTy *>(ty)) {
      if (!seen.insert(name_ty).second) {
        errormsg->Error(type_dec->ty_->pos_, "illegal type cycle");
        name_ty->ty_ = type::IntTy::Instance();
        break;
      }
      ty = name_ty->ty_;
      if (!ty)
        break;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  type::Ty *ty = tenv->Look(name_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().c_str());
    return type::IntTy::Instance();
  }
  return ty;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  return new type::RecordTy(record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  type::Ty *ty = tenv->Look(array_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().c_str());
    return new type::ArrayTy(type::IntTy::Instance());
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace sem

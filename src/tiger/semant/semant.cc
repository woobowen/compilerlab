#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return static_cast<env::VarEntry*>(entry)->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().c_str());
    return type::IntTy::Instance();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*var_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }

  type::RecordTy *record_ty = static_cast<type::RecordTy*>(var_ty);
  for (type::Field *field : record_ty->fields_->GetList()) {
    if (field->name_ == sym_) {
      return field->ty_->ActualTy();
    }
  }

  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().c_str());
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *subscript_ty = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*subscript_ty) != typeid(type::IntTy)) {
    errormsg->Error(subscript_->pos_, "integer required");
  }

  if (typeid(*var_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }

  return static_cast<type::ArrayTy*>(var_ty)->ty_->ActualTy();
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
  env::EnvEntry *entry = venv->Look(func_);
  if (!entry || typeid(*entry) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().c_str());
    return type::IntTy::Instance();
  }

  env::FunEntry *fun_entry = static_cast<env::FunEntry*>(entry);
  auto formal_it = fun_entry->formals_->GetList().begin();
  auto actual_it = args_->GetList().begin();

  while (formal_it != fun_entry->formals_->GetList().end() &&
         actual_it != args_->GetList().end()) {
    type::Ty *formal_ty = (*formal_it)->ActualTy();
    type::Ty *actual_ty = (*actual_it)->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

    if (!formal_ty->IsSameType(actual_ty)) {
      errormsg->Error((*actual_it)->pos_, "para type mismatch");
    }

    ++formal_it;
    ++actual_it;
  }

  if (formal_it != fun_entry->formals_->GetList().end()) {
    errormsg->Error(pos_, "too few params in function %s", func_->Name().c_str());
  } else if (actual_it != args_->GetList().end()) {
    errormsg->Error(pos_, "too many params in function %s", func_->Name().c_str());
  }

  return fun_entry->result_->ActualTy();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  switch (oper_) {
    case PLUS_OP:
    case MINUS_OP:
    case TIMES_OP:
    case DIVIDE_OP:
      if (typeid(*left_ty) != typeid(type::IntTy)) {
        errormsg->Error(left_->pos_, "integer required");
      }
      if (typeid(*right_ty) != typeid(type::IntTy)) {
        errormsg->Error(right_->pos_, "integer required");
      }
      return type::IntTy::Instance();

    case EQ_OP:
    case NEQ_OP:
      if (!left_ty->IsSameType(right_ty)) {
        errormsg->Error(pos_, "same type required");
      }
      return type::IntTy::Instance();

    case LT_OP:
    case LE_OP:
    case GT_OP:
    case GE_OP:
      if (typeid(*left_ty) != typeid(type::IntTy)) {
        errormsg->Error(left_->pos_, "integer required");
      }
      if (typeid(*right_ty) != typeid(type::IntTy)) {
        errormsg->Error(right_->pos_, "integer required");
      }
      return type::IntTy::Instance();

    default:
      return type::IntTy::Instance();
  }
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *ty = tenv->Look(typ_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::IntTy::Instance();
  }

  ty = ty->ActualTy();
  if (typeid(*ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }

  type::RecordTy *record_ty = static_cast<type::RecordTy*>(ty);
  auto field_it = record_ty->fields_->GetList().begin();
  auto efield_it = fields_->GetList().begin();

  while (field_it != record_ty->fields_->GetList().end() &&
         efield_it != fields_->GetList().end()) {
    type::Field *field = *field_it;
    EField *efield = *efield_it;

    if (field->name_ != efield->name_) {
      errormsg->Error(pos_, "field name mismatch");
    }

    type::Ty *field_ty = field->ty_->ActualTy();
    type::Ty *exp_ty = efield->exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

    if (!field_ty->IsSameType(exp_ty)) {
      errormsg->Error(efield->exp_->pos_, "field type mismatch");
    }

    ++field_it;
    ++efield_it;
  }

  if (field_it != record_ty->fields_->GetList().end() ||
      efield_it != fields_->GetList().end()) {
    errormsg->Error(pos_, "field count mismatch");
  }

  return ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *result = type::VoidTy::Instance();
  for (Exp *exp : seq_->GetList()) {
    result = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return result;
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*test_ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "integer required");
  }

  type::Ty *then_ty = then_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (elsee_) {
    type::Ty *else_ty = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
    if (!then_ty->IsSameType(else_ty)) {
      errormsg->Error(pos_, "then exp and else exp type mismatch");
    }
    return then_ty;
  } else {
    if (typeid(*then_ty) != typeid(type::VoidTy)) {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
    return type::VoidTy::Instance();
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *test_ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typeid(*test_ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "integer required");
  }

  type::Ty *body_ty = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg)->ActualTy();

  if (typeid(*body_ty) != typeid(type::VoidTy)) {
    errormsg->Error(body_->pos_, "while body must produce no value");
  }

  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  if (labelcount == 0) {
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  venv->BeginScope();
  tenv->BeginScope();

  for (Dec *dec : decs_->GetList()) {
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }

  type::Ty *result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);

  tenv->EndScope();
  venv->EndScope();

  return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *ty = tenv->Look(typ_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
    return type::IntTy::Instance();
  }

  ty = ty->ActualTy();
  if (typeid(*ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "not an array type");
    return type::IntTy::Instance();
  }

  type::Ty *size_ty = size_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*size_ty) != typeid(type::IntTy)) {
    errormsg->Error(size_->pos_, "integer required");
  }

  type::ArrayTy *array_ty = static_cast<type::ArrayTy*>(ty);
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (!array_ty->ty_->ActualTy()->IsSameType(init_ty)) {
    errormsg->Error(init_->pos_, "type mismatch");
  }

  return ty;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  return type::VoidTy::Instance();
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *var_ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *exp_ty = exp_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (!var_ty->IsSameType(exp_ty)) {
    errormsg->Error(pos_, "unmatched assign exp");
  }

  // Check if variable is read-only (for loop variable)
  if (typeid(*var_) == typeid(SimpleVar)) {
    SimpleVar *simple_var = static_cast<SimpleVar*>(var_);
    env::EnvEntry *entry = venv->Look(simple_var->sym_);
    if (entry && typeid(*entry) == typeid(env::VarEntry)) {
      env::VarEntry *var_entry = static_cast<env::VarEntry*>(entry);
      if (var_entry->readonly_) {
        errormsg->Error(pos_, "loop variable can't be assigned");
      }
    }
  }

  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  // First pass: add all function headers to environment
  for (FunDec *fun_dec : functions_->GetList()) {
    // Check for duplicate function names in the same batch
    for (FunDec *other : functions_->GetList()) {
      if (fun_dec != other && fun_dec->name_ == other->name_) {
        errormsg->Error(pos_, "two functions have the same name");
        break;
      }
    }

    // Build formal parameter type list
    type::TyList *formals = new type::TyList();
    for (Field *field : fun_dec->params_->GetList()) {
      type::Ty *ty = tenv->Look(field->typ_);
      if (!ty) {
        errormsg->Error(pos_, "undefined type %s", field->typ_->Name().c_str());
        ty = type::IntTy::Instance();
      }
      formals->Append(ty);
    }

    // Get result type
    type::Ty *result_ty;
    if (fun_dec->result_) {
      result_ty = tenv->Look(fun_dec->result_);
      if (!result_ty) {
        errormsg->Error(pos_, "undefined type %s", fun_dec->result_->Name().c_str());
        result_ty = type::VoidTy::Instance();
      }
    } else {
      result_ty = type::VoidTy::Instance();
    }

    venv->Enter(fun_dec->name_, new env::FunEntry(formals, result_ty));
  }

  // Second pass: check function bodies
  for (FunDec *fun_dec : functions_->GetList()) {
    venv->BeginScope();

    // Add parameters to scope
    auto param_it = fun_dec->params_->GetList().begin();
    env::FunEntry *fun_entry = static_cast<env::FunEntry*>(venv->Look(fun_dec->name_));
    auto formal_it = fun_entry->formals_->GetList().begin();

    while (param_it != fun_dec->params_->GetList().end()) {
      Field *param = *param_it;
      type::Ty *formal_ty = *formal_it;
      venv->Enter(param->name_, new env::VarEntry(formal_ty));
      ++param_it;
      ++formal_it;
    }

    // Check body
    type::Ty *body_ty = fun_dec->body_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
    type::Ty *result_ty = fun_entry->result_->ActualTy();

    if (!body_ty->IsSameType(result_ty)) {
      errormsg->Error(fun_dec->body_->pos_, "function body type mismatch");
    }

    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

  if (typ_) {
    // Variable has explicit type annotation
    type::Ty *declared_ty = tenv->Look(typ_);
    if (!declared_ty) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().c_str());
      venv->Enter(var_, new env::VarEntry(init_ty));
    } else {
      declared_ty = declared_ty->ActualTy();
      if (!init_ty->IsSameType(declared_ty)) {
        errormsg->Error(pos_, "type mismatch");
      }
      venv->Enter(var_, new env::VarEntry(declared_ty));
    }
  } else {
    // Variable has no type annotation
    if (typeid(*init_ty) == typeid(type::NilTy)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
    }
    venv->Enter(var_, new env::VarEntry(init_ty));
  }
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  // First pass: add all type names to environment
  for (NameAndTy *name_and_ty : types_->GetList()) {
    // Check for duplicate type names in the same batch
    for (NameAndTy *other : types_->GetList()) {
      if (name_and_ty != other && name_and_ty->name_ == other->name_) {
        errormsg->Error(pos_, "two types have the same name");
        break;
      }
    }
    tenv->Enter(name_and_ty->name_, new type::NameTy(name_and_ty->name_, nullptr));
  }

  // Second pass: resolve type definitions
  for (NameAndTy *name_and_ty : types_->GetList()) {
    type::Ty *ty = tenv->Look(name_and_ty->name_);
    type::NameTy *name_ty = static_cast<type::NameTy*>(ty);
    name_ty->ty_ = name_and_ty->ty_->SemAnalyze(tenv, errormsg);
  }

  // Third pass: check for illegal cycles
  for (NameAndTy *name_and_ty : types_->GetList()) {
    type::Ty *ty = tenv->Look(name_and_ty->name_);
    std::set<sym::Symbol*> visited;
    type::Ty *current = ty;

    while (current && typeid(*current) == typeid(type::NameTy)) {
      type::NameTy *name_ty = static_cast<type::NameTy*>(current);
      if (visited.count(name_ty->sym_)) {
        errormsg->Error(pos_, "illegal type cycle");
        break;
      }
      visited.insert(name_ty->sym_);
      current = name_ty->ty_;
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  type::Ty *ty = tenv->Look(name_);
  if (!ty) {
    errormsg->Error(0, "undefined type %s", name_->Name().c_str());
    return type::IntTy::Instance();
  }
  return ty;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  type::FieldList *fields = new type::FieldList();
  for (Field *field : record_->GetList()) {
    type::Ty *ty = tenv->Look(field->typ_);
    if (!ty) {
      errormsg->Error(0, "undefined type %s", field->typ_->Name().c_str());
      ty = type::IntTy::Instance();
    }
    fields->Append(new type::Field(field->name_, ty));
  }
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  type::Ty *ty = tenv->Look(array_);
  if (!ty) {
    errormsg->Error(0, "undefined type %s", array_->Name().c_str());
    ty = type::IntTy::Instance();
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

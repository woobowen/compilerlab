#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace {
frame::ProcFrag *ProcEntryExit(tr::Level *level, tr::Exp *body);
}

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access *frame_access = level->frame_->AllocLocal(escape);
  return new Access(level, frame_access);
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    tree::CjumpStm *stm = new tree::CjumpStm(
      tree::NE_OP, exp_, new tree::ConstExp(0), nullptr, nullptr);
    PatchList trues({&stm->true_label_});
    PatchList falses({&stm->false_label_});
    return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    errormsg->Error(0, "NxExp cannot be converted to Cx");
    return Cx(PatchList(), PatchList(), stm_);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();

    PatchList trues_copy = cx_.trues_;
    PatchList falses_copy = cx_.falses_;
    trues_copy.DoPatch(t);
    falses_copy.DoPatch(f);

    return new tree::EseqExp(
      new tree::SeqStm(
        cx_.stm_,
        new tree::SeqStm(
          new tree::LabelStm(t),
          new tree::SeqStm(
            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
            new tree::SeqStm(
              new tree::JumpStm(new tree::NameExp(temp::LabelFactory::NewLabel()),
                                new std::vector<temp::Label*>()),
              new tree::SeqStm(
                new tree::LabelStm(f),
                new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0))
              )
            )
          )
        )
      ),
      new tree::TempExp(r)
    );
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    temp::Label *label = temp::LabelFactory::NewLabel();
    PatchList trues_copy = cx_.trues_;
    PatchList falses_copy = cx_.falses_;
    trues_copy.DoPatch(label);
    falses_copy.DoPatch(label);
    return new tree::SeqStm(cx_.stm_, new tree::LabelStm(label));
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseTEnv();
  FillBaseVEnv();

  tr::ExpAndTy *result = absyn_tree_->Translate(
    venv_.get(), tenv_.get(), main_level_.get(), nullptr, errormsg_.get());

  if (result && result->exp_) {
    frags->PushBack(ProcEntryExit(main_level_.get(), result->exp_));
  }
}

} // namespace tr

namespace {

/**
 * Wrapper for `ProcExitEntry1`, which deals with the return value of the
 * function body
 * @param level current level
 * @param body function body
 * @return statements after `ProcExitEntry1`
 */
frame::ProcFrag *ProcEntryExit(tr::Level *level, tr::Exp *body) {
  /* TODO: Put your lab5 code here */
  tree::Stm *stm = new tree::MoveStm(
    new tree::TempExp(reg_manager->ReturnValue()),
    body->UnEx()
  );

  return new frame::ProcFrag(
    frame::ProcEntryExit1(level->frame_, stm),
    level->frame_
  );
}
} // namespace

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if (!entry || typeid(*entry) != typeid(env::VarEntry)) {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::IntTy::Instance());
  }

  env::VarEntry *var_entry = static_cast<env::VarEntry*>(entry);
  tr::Access *access = var_entry->access_;

  // Follow static links to reach the variable's level
  tree::Exp *fp = new tree::TempExp(reg_manager->FramePointer());
  tr::Level *current_level = level;

  while (current_level && current_level != access->level_) {
    // Follow static link (first formal parameter)
    fp = current_level->frame_->Formals()->front()->ToExp(fp);
    current_level = current_level->parent_;
  }

  tree::Exp *var_exp = access->access_->ToExp(fp);
  return new tr::ExpAndTy(new tr::ExExp(var_exp), var_entry->ty_->ActualTy());
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *exp_ty =
    var_->Translate(venv, tenv, level, label, errormsg);
  tr::Exp *exp = exp_ty->exp_;
  type::Ty *ty = exp_ty->ty_->ActualTy();

  if (typeid(*ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::VoidTy::Instance());
  }

  if (typeid(*exp) != typeid(tr::ExExp)) {
    errormsg->Error(pos_, "field var's exp must be an expression");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::VoidTy::Instance());
  }
  auto record_ty = static_cast<type::RecordTy *>(ty);
  type::FieldList *field_list = record_ty->fields_;
  int order = 0;
  for (auto field : field_list->GetList()) {
    if (field->name_ == sym_) {
      tree::Exp *texp = new tree::MemExp(new tree::BinopExp(
          tree::PLUS_OP, exp->UnEx(),
          new tree::ConstExp(order * reg_manager->WordSize())));
      return new tr::ExpAndTy(new tr::ExExp(texp), field->ty_->ActualTy());
    }
    order++;
  }
  errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());

  /*TODO end*/
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var_exp = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *sub_exp = subscript_->Translate(venv, tenv, level, label, errormsg);

  type::Ty *var_ty = var_exp->ty_->ActualTy();
  if (typeid(*var_ty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "array type required");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::IntTy::Instance());
  }

  type::ArrayTy *array_ty = static_cast<type::ArrayTy*>(var_ty);

  tree::Exp *addr = new tree::BinopExp(
    tree::PLUS_OP,
    var_exp->exp_->UnEx(),
    new tree::BinopExp(
      tree::MUL_OP,
      sub_exp->exp_->UnEx(),
      new tree::ConstExp(reg_manager->WordSize())
    )
  );

  return new tr::ExpAndTy(new tr::ExExp(new tree::MemExp(addr)),
                          array_ty->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *str_label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label, str_));

  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str_label)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(func_);
  if (!entry || typeid(*entry) != typeid(env::FunEntry)) {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::IntTy::Instance());
  }

  env::FunEntry *fun_entry = static_cast<env::FunEntry*>(entry);

  // Translate arguments
  tree::ExpList *arg_list = new tree::ExpList();

  // Add static link as first argument
  if (fun_entry->level_->parent_ == nullptr) {
    // External function, no static link
  } else if (fun_entry->level_->parent_ == level) {
    // Calling sibling or child function: pass current frame pointer
    arg_list->Append(new tree::TempExp(reg_manager->FramePointer()));
  } else {
    // Calling parent or outer function: follow static links
    tree::Exp *fp = new tree::TempExp(reg_manager->FramePointer());
    tr::Level *current_level = level;

    while (current_level && current_level != fun_entry->level_->parent_) {
      fp = current_level->frame_->Formals()->front()->ToExp(fp);
      current_level = current_level->parent_;
    }

    arg_list->Append(fp);
  }

  // Add actual arguments
  for (auto arg : args_->GetList()) {
    tr::ExpAndTy *arg_exp = arg->Translate(venv, tenv, level, label, errormsg);
    arg_list->Append(arg_exp->exp_->UnEx());
  }

  tree::Exp *call_exp = new tree::CallExp(
    new tree::NameExp(fun_entry->label_),
    arg_list
  );

  return new tr::ExpAndTy(new tr::ExExp(call_exp), fun_entry->result_->ActualTy());

  /* End for lab5 code */
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *left_exp = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right_exp = right_->Translate(venv, tenv, level, label, errormsg);

  tree::Exp *left_tree = left_exp->exp_->UnEx();
  tree::Exp *right_tree = right_exp->exp_->UnEx();

  switch (oper_) {
    case PLUS_OP:
      return new tr::ExpAndTy(
        new tr::ExExp(new tree::BinopExp(tree::PLUS_OP, left_tree, right_tree)),
        type::IntTy::Instance());

    case MINUS_OP:
      return new tr::ExpAndTy(
        new tr::ExExp(new tree::BinopExp(tree::MINUS_OP, left_tree, right_tree)),
        type::IntTy::Instance());

    case TIMES_OP:
      return new tr::ExpAndTy(
        new tr::ExExp(new tree::BinopExp(tree::MUL_OP, left_tree, right_tree)),
        type::IntTy::Instance());

    case DIVIDE_OP:
      return new tr::ExpAndTy(
        new tr::ExExp(new tree::BinopExp(tree::DIV_OP, left_tree, right_tree)),
        type::IntTy::Instance());

    case EQ_OP:
    case NEQ_OP:
    case LT_OP:
    case LE_OP:
    case GT_OP:
    case GE_OP: {
      // Comparison operators
      tree::CjumpStm *stm;
      tree::RelOp op;

      // Check if string comparison
      type::Ty *left_ty = left_exp->ty_->ActualTy();
      if (typeid(*left_ty) == typeid(type::StringTy)) {
        // String comparison: call runtime function
        tree::Exp *call = frame::ExternalCall(
          "stringEqual",
          new tree::ExpList({left_tree, right_tree})
        );

        if (oper_ == EQ_OP) {
          op = tree::EQ_OP;
        } else {
          op = tree::NE_OP;
        }

        stm = new tree::CjumpStm(op, call, new tree::ConstExp(1), nullptr, nullptr);
      } else {
        // Integer comparison
        switch (oper_) {
          case EQ_OP: op = tree::EQ_OP; break;
          case NEQ_OP: op = tree::NE_OP; break;
          case LT_OP: op = tree::LT_OP; break;
          case LE_OP: op = tree::LE_OP; break;
          case GT_OP: op = tree::GT_OP; break;
          case GE_OP: op = tree::GE_OP; break;
          default: op = tree::EQ_OP; break;
        }

        stm = new tree::CjumpStm(op, left_tree, right_tree, nullptr, nullptr);
      }

      tr::PatchList trues({&stm->true_label_});
      tr::PatchList falses({&stm->false_label_});

      return new tr::ExpAndTy(new tr::CxExp(trues, falses, stm),
                              type::IntTy::Instance());
    }

    default:
      return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                              type::IntTy::Instance());
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(typ_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::IntTy::Instance());
  }

  type::Ty *actual_ty = ty->ActualTy();
  if (typeid(*actual_ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::IntTy::Instance());
  }

  int field_count = fields_->GetList().size();

  // Allocate memory for record
  tree::Exp *alloc = frame::ExternalCall(
    "allocRecord",
    new tree::ExpList({new tree::ConstExp(field_count * reg_manager->WordSize())})
  );

  temp::Temp *r = temp::TempFactory::NewTemp();
  tree::Stm *stm = new tree::MoveStm(new tree::TempExp(r), alloc);

  // Initialize fields
  int offset = 0;
  for (auto field : fields_->GetList()) {
    tr::ExpAndTy *field_exp = field->exp_->Translate(venv, tenv, level, label, errormsg);

    stm = new tree::SeqStm(
      stm,
      new tree::MoveStm(
        new tree::MemExp(
          new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(r),
                             new tree::ConstExp(offset))
        ),
        field_exp->exp_->UnEx()
      )
    );

    offset += reg_manager->WordSize();
  }

  return new tr::ExpAndTy(
    new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(r))),
    actual_ty
  );
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  if (seq_->GetList().empty()) {
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::VoidTy::Instance());
  }

  tree::Stm *stm = nullptr;
  tr::ExpAndTy *last_exp = nullptr;

  auto exp_list = seq_->GetList();
  for (auto it = exp_list.begin(); it != exp_list.end(); ++it) {
    last_exp = (*it)->Translate(venv, tenv, level, label, errormsg);

    if (std::next(it) != exp_list.end()) {
      // Not the last expression
      if (stm == nullptr) {
        stm = last_exp->exp_->UnNx();
      } else {
        stm = new tree::SeqStm(stm, last_exp->exp_->UnNx());
      }
    }
  }

  if (stm == nullptr) {
    // Only one expression
    return last_exp;
  } else {
    // Multiple expressions
    return new tr::ExpAndTy(
      new tr::ExExp(new tree::EseqExp(stm, last_exp->exp_->UnEx())),
      last_exp->ty_
    );
  }
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var_exp = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp_exp = exp_->Translate(venv, tenv, level, label, errormsg);

  tree::Stm *stm = new tree::MoveStm(var_exp->exp_->UnEx(), exp_exp->exp_->UnEx());

  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test_exp = test_->Translate(venv, tenv, level, label, errormsg);
  tr::Cx test_cx = test_exp->exp_->UnCx(errormsg);

  tr::ExpAndTy *then_exp = then_->Translate(venv, tenv, level, label, errormsg);

  temp::Label *t = temp::LabelFactory::NewLabel();
  temp::Label *f = temp::LabelFactory::NewLabel();

  test_cx.trues_.DoPatch(t);
  test_cx.falses_.DoPatch(f);

  if (elsee_) {
    // if-then-else: has return value
    tr::ExpAndTy *else_exp = elsee_->Translate(venv, tenv, level, label, errormsg);

    temp::Label *join = temp::LabelFactory::NewLabel();
    temp::Temp *r = temp::TempFactory::NewTemp();

    tree::Stm *stm = new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(t),
        new tree::SeqStm(
          new tree::MoveStm(new tree::TempExp(r), then_exp->exp_->UnEx()),
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(join),
                              new std::vector<temp::Label*>({join})),
            new tree::SeqStm(
              new tree::LabelStm(f),
              new tree::SeqStm(
                new tree::MoveStm(new tree::TempExp(r), else_exp->exp_->UnEx()),
                new tree::LabelStm(join)
              )
            )
          )
        )
      )
    );

    return new tr::ExpAndTy(
      new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(r))),
      then_exp->ty_
    );
  } else {
    // if-then: no return value
    tree::Stm *stm = new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(t),
        new tree::SeqStm(
          then_exp->exp_->UnNx(),
          new tree::LabelStm(f)
        )
      )
    );

    return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();

  tr::ExpAndTy *test_exp = test_->Translate(venv, tenv, level, label, errormsg);
  tr::Cx test_cx = test_exp->exp_->UnCx(errormsg);

  test_cx.trues_.DoPatch(body_label);
  test_cx.falses_.DoPatch(done_label);

  tr::ExpAndTy *body_exp = body_->Translate(venv, tenv, level, done_label, errormsg);

  tree::Stm *stm = new tree::SeqStm(
    new tree::LabelStm(test_label),
    new tree::SeqStm(
      test_cx.stm_,
      new tree::SeqStm(
        new tree::LabelStm(body_label),
        new tree::SeqStm(
          body_exp->exp_->UnNx(),
          new tree::SeqStm(
            new tree::JumpStm(new tree::NameExp(test_label),
                              new std::vector<temp::Label*>({test_label})),
            new tree::LabelStm(done_label)
          )
        )
      )
    )
  );

  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *lo_exp = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi_exp = hi_->Translate(venv, tenv, level, label, errormsg);

  // Allocate loop variable
  tr::Access *access = tr::Access::AllocLocal(level, escape_);

  venv->BeginScope();
  venv->Enter(var_, new env::VarEntry(access, type::IntTy::Instance(), true));

  temp::Label *test_label = temp::LabelFactory::NewLabel();
  temp::Label *body_label = temp::LabelFactory::NewLabel();
  temp::Label *done_label = temp::LabelFactory::NewLabel();

  // Get loop variable address
  tree::Exp *fp = new tree::TempExp(reg_manager->FramePointer());
  tree::Exp *var_exp = access->access_->ToExp(fp);

  // Initialize loop variable
  tree::Stm *init_stm = new tree::MoveStm(var_exp, lo_exp->exp_->UnEx());

  // Store hi value in a temp
  temp::Temp *limit = temp::TempFactory::NewTemp();
  tree::Stm *limit_stm = new tree::MoveStm(new tree::TempExp(limit), hi_exp->exp_->UnEx());

  // Test condition: var <= limit
  tree::CjumpStm *test_stm = new tree::CjumpStm(
    tree::LE_OP,
    var_exp,
    new tree::TempExp(limit),
    body_label,
    done_label
  );

  // Translate body
  tr::ExpAndTy *body_exp = body_->Translate(venv, tenv, level, done_label, errormsg);

  // Increment loop variable
  tree::Stm *inc_stm = new tree::MoveStm(
    var_exp,
    new tree::BinopExp(tree::PLUS_OP, var_exp, new tree::ConstExp(1))
  );

  tree::Stm *stm = new tree::SeqStm(
    init_stm,
    new tree::SeqStm(
      limit_stm,
      new tree::SeqStm(
        new tree::LabelStm(test_label),
        new tree::SeqStm(
          test_stm,
          new tree::SeqStm(
            new tree::LabelStm(body_label),
            new tree::SeqStm(
              body_exp->exp_->UnNx(),
              new tree::SeqStm(
                inc_stm,
                new tree::SeqStm(
                  new tree::JumpStm(new tree::NameExp(test_label),
                                    new std::vector<temp::Label*>({test_label})),
                  new tree::LabelStm(done_label)
                )
              )
            )
          )
        )
      )
    )
  );

  venv->EndScope();

  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  if (!label) {
    errormsg->Error(pos_, "break is not inside any loop");
    return new tr::ExpAndTy(new tr::NxExp(new tree::ExpStm(new tree::ConstExp(0))),
                            type::VoidTy::Instance());
  }

  tree::Stm *stm = new tree::JumpStm(
    new tree::NameExp(label),
    new std::vector<temp::Label*>({label})
  );

  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tree::Stm *dec_stm = nullptr;

  for (auto dec : decs_->GetList()) {
    tr::Exp *dec_exp = dec->Translate(venv, tenv, level, label, errormsg);
    if (dec_exp) {
      tree::Stm *stm = dec_exp->UnNx();
      if (dec_stm == nullptr) {
        dec_stm = stm;
      } else {
        dec_stm = new tree::SeqStm(dec_stm, stm);
      }
    }
  }

  tr::ExpAndTy *body_exp = body_->Translate(venv, tenv, level, label, errormsg);

  if (dec_stm == nullptr) {
    return body_exp;
  } else {
    return new tr::ExpAndTy(
      new tr::ExExp(new tree::EseqExp(dec_stm, body_exp->exp_->UnEx())),
      body_exp->ty_
    );
  }
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *size_exp = size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *init_exp = init_->Translate(venv, tenv, level, label, errormsg);

  type::Ty *ty = tenv->Look(typ_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                            type::IntTy::Instance());
  }

  tree::Exp *call = frame::ExternalCall(
    "initArray",
    new tree::ExpList({size_exp->exp_->UnEx(), init_exp->exp_->UnEx()})
  );

  return new tr::ExpAndTy(new tr::ExExp(call), ty->ActualTy());
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // First pass: add function headers to environment
  for (auto func : functions_->GetList()) {
    // Collect formal parameter escape info
    std::list<bool> formals;
    for (auto param : func->params_->GetList()) {
      formals.push_back(param->escape_);
    }

    // Create new level for this function
    temp::Label *func_label = temp::LabelFactory::NamedLabel(func->name_->Name());
    tr::Level *new_level = tr::Level::NewLevel(level, func_label, formals);

    // Determine result type
    type::Ty *result_ty;
    if (func->result_) {
      result_ty = tenv->Look(func->result_);
      if (!result_ty) {
        errormsg->Error(func->pos_, "undefined result type %s",
                        func->result_->Name().data());
        result_ty = type::IntTy::Instance();
      }
    } else {
      result_ty = type::VoidTy::Instance();
    }

    // Build formal types list
    type::TyList *formal_tys = new type::TyList();
    for (auto param : func->params_->GetList()) {
      type::Ty *param_ty = tenv->Look(param->typ_);
      if (!param_ty) {
        errormsg->Error(param->pos_, "undefined type %s",
                        param->typ_->Name().data());
        param_ty = type::IntTy::Instance();
      }
      formal_tys->Append(param_ty);
    }

    // Add function to environment
    venv->Enter(func->name_,
                new env::FunEntry(new_level, func_label, formal_tys, result_ty));
  }

  // Second pass: translate function bodies
  for (auto func : functions_->GetList()) {
    env::FunEntry *fun_entry =
        static_cast<env::FunEntry*>(venv->Look(func->name_));

    venv->BeginScope();

    // Add parameters to environment
    auto formals = fun_entry->level_->Formals();
    auto formal_it = formals->begin();
    auto param_it = func->params_->GetList().begin();
    auto type_it = fun_entry->formals_->GetList().begin();

    // Skip static link (first formal)
    ++formal_it;

    while (param_it != func->params_->GetList().end()) {
      venv->Enter((*param_it)->name_,
                  new env::VarEntry(*formal_it, *type_it));
      ++formal_it;
      ++param_it;
      ++type_it;
    }

    // Translate function body
    tr::ExpAndTy *body_exp = func->body_->Translate(
        venv, tenv, fun_entry->level_, nullptr, errormsg);

    // Generate procedure fragment
    frags->PushBack(ProcEntryExit(fun_entry->level_, body_exp->exp_));

    venv->EndScope();
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *init_exp_ty =
      init_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *init_ty = init_exp_ty->ty_;

  if (typ_) {
    type::Ty *ty = tenv->Look(typ_);
    if (!ty) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    }

    if (!ty->IsSameType(init_ty)) {
      errormsg->Error(pos_, "type and init type mismatch");
    }
  } else {
    auto actual_init_ty = init_ty->ActualTy();
    if (typeid(*actual_init_ty) == typeid(type::NilTy)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
    }
  }

  tr::Access *access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, init_ty));

  return new tr::NxExp(
      new tree::MoveStm(access->access_->ToExp(new tree::TempExp(
                            reg_manager->FramePointer())),
                        init_exp_ty->exp_->UnEx()));
  /*TODO end*/
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // First pass: add all type names to environment
  for (NameAndTy *name_and_ty : types_->GetList()) {
    tenv->Enter(name_and_ty->name_, new type::NameTy(name_and_ty->name_, nullptr));
  }

  // Second pass: resolve type definitions
  for (NameAndTy *name_and_ty : types_->GetList()) {
    type::Ty *ty = tenv->Look(name_and_ty->name_);
    type::NameTy *name_ty = static_cast<type::NameTy*>(ty);
    name_ty->ty_ = name_and_ty->ty_->Translate(tenv, errormsg);
  }

  // Type declarations don't generate code
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(name_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return type::IntTy::Instance();
  }
  return ty;
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList *fields = new type::FieldList();
  for (Field *field : record_->GetList()) {
    type::Ty *ty = tenv->Look(field->typ_);
    if (!ty) {
      errormsg->Error(pos_, "undefined type %s", field->typ_->Name().data());
      ty = type::IntTy::Instance();
    }
    fields->Append(new type::Field(field->name_, ty));
  }
  return new type::RecordTy(fields);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(array_);
  if (!ty) {
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
    ty = type::IntTy::Instance();
  }
  return new type::ArrayTy(ty);
}

} // namespace absyn

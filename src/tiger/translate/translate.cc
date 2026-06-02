#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include <iostream>
#include <string>

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
  
  frame::Access *access=level->frame_->AllocLocal(escape);
  return new Access(level,access);
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
    
    return this->exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    
    temp::Label *t_branch=temp::LabelFactory::NewLabel();
    temp::Label *f_branch=temp::LabelFactory::NewLabel();
    auto jump_stmt = new tree::CjumpStm(tree::NE_OP, exp_,
    new tree::ConstExp(0), t_branch, f_branch);
    PatchList t_patch(std::list<temp::Label **>{&(jump_stmt->true_label_)});
    PatchList f_patch(std::list<temp::Label **>{&(jump_stmt->false_label_)});

    return Cx(t_patch,f_patch,jump_stmt);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    
    return new tree::EseqExp(this->stm_,new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    
    return this->stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    
    std::cerr<<"nx can't be converted to test_ exp"<<std::endl;
    assert(false);
    return Cx({}, {}, nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    
    temp::Temp *result=temp::TempFactory::NewTemp();
    temp::Label *t_branch=temp::LabelFactory::NewLabel();
    temp::Label *f_branch=temp::LabelFactory::NewLabel();

    this->cx_.trues_.DoPatch(t_branch);
    this->cx_.falses_.DoPatch(f_branch);

    tree::EseqExp *exp=
      new tree::EseqExp(
        new tree::MoveStm(
          new tree::TempExp(result),
          new tree::ConstExp(1)
        ),
        new tree::EseqExp(
          this->cx_.stm_,
          new tree::EseqExp(
            new tree::LabelStm(f_branch),
            new tree::EseqExp(
              new tree::MoveStm(
                new tree::TempExp(result),
                new tree::ConstExp(0)
              ),
              new tree::EseqExp(
                new tree::LabelStm(t_branch),
                new tree::TempExp(result)
              )
            )

          )
        )
      );
    return exp;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    
    return this->cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    
    return this->cx_;
  }
};

void ProgTr::Translate() {
  
  FillBaseTEnv();
  FillBaseVEnv();

  auto func_body = this->absyn_tree_->Translate(this->venv_.get(),this->tenv_.get(),
          this->main_level_.get(),this->main_level_->frame_->name_,
          this->errormsg_.get());

  ProcEntryExit(this->main_level_.get(),func_body->exp_);
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
  
  auto frag=new frame::ProcFrag(
    frame::ProcEntryExit1(level->frame_,
      new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),
                              body->UnEx())),
    level->frame_
    );
  frags->PushBack(frag);
  return frag;
}
} // namespace

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  
  return this->root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  
  auto _entry =venv->Look(this->sym_);
  if(_entry==nullptr)
  {
    errormsg->Error(pos_, "variable %s does not exist",
      sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::IntTy::Instance());
  }
  if(typeid(*_entry)!=typeid(env::VarEntry))
  {
    errormsg->Error(pos_, "variable %s is not a variable",
      sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
  }
  auto entry= static_cast<env::VarEntry *>(_entry);
  auto access = static_cast<env::VarEntry *>(entry)->access_->access_;
  auto access_level=entry->access_->level_;

  auto current_level=level;
  tree::Exp *frameptr=new tree::TempExp(reg_manager->FramePointer());
  // Bug to be fixed
  while(current_level != access_level){
    frameptr = level->frame_->Formals()->front()->ToExp(frameptr);
    current_level = current_level->parent_;
  }
  return new tr::ExpAndTy(new tr::ExExp(access->ToExp(frameptr)),
                          entry->ty_);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  
  auto exp_and_ty=this->var_->Translate(venv,tenv,level,label,errormsg);
  auto ty = exp_and_ty->ty_->ActualTy();
  if(typeid(*(ty))!=typeid(type::RecordTy))
  {
    errormsg->Error(pos_, "variable %s is not a record",
      sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
  }
  int offset=0;
  for(auto fieldsym : static_cast<type::RecordTy *>(ty)->fields_->GetList())
  {
    if(fieldsym->name_==this->sym_)
    {
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::MemExp(
            new tree::BinopExp(
              tree::BinOp::PLUS_OP,
              exp_and_ty->exp_->UnEx(),
              new tree::ConstExp(offset*reg_manager->WordSize())
            )
          )
        ),
        fieldsym->ty_->ActualTy()
      );
    }
    ++offset;
  }
  errormsg->Error(this->pos_,"field %s not exist",this->sym_->Name().data());
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  
  auto var_exp_and_ty=this->var_->Translate(venv,tenv,level,label,errormsg);
  auto subscript_exp_and_ty=this->subscript_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *var_ty=var_exp_and_ty->ty_;
  type::ArrayTy *ret_ty=nullptr;

  if(var_ty && typeid(*var_ty)==typeid(type::ArrayTy))
  {
    ret_ty=static_cast<type::ArrayTy *>(var_ty);
  }
  else{
    errormsg->Error(this->pos_, "array type required");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::NilTy::Instance());
  }

  if(!subscript_exp_and_ty->ty_->IsSameType(type::IntTy::Instance()))
  {
    errormsg->Error(this->pos_, "subscript is not an integer type");
  }

  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::MemExp(
        new tree::BinopExp(
          tree::BinOp::PLUS_OP,
          var_exp_and_ty->exp_->UnEx(), 
          new tree::BinopExp(
            tree::BinOp::MUL_OP,
            subscript_exp_and_ty->exp_->UnEx(),
            new tree::ConstExp(reg_manager->WordSize())
            )
          )
        )
      )
    ,
    ret_ty->ty_->ActualTy()
    );
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  
  return this->var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(this->val_)),type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  
  auto str_label=temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label,this->str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str_label)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  
  auto function_=venv->Look(this->func_);
  if(function_==nullptr)
  {
    errormsg->Error(this->pos_,"function is not defined");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
  }
  if(typeid(*function_)!=typeid(env::FunEntry))
  {
    errormsg->Error(this->pos_,"function is not a function");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
  }
  auto function=static_cast<env::FunEntry *>(function_);
  tree::ExpList *params=new tree::ExpList();
  if (function->label_)
  {
    auto current_level=level;
    auto target_level=function->level_->parent_;
    tree::Exp *static_link=new tree::TempExp(reg_manager->FramePointer());
    while(current_level!=target_level)
    {
      static_link=current_level->frame_->Formals()->front()->ToExp(static_link);
      current_level=current_level->parent_;
    }
    params->Append(static_link);
  }
  auto arg_iter=this->args_->GetList().begin();
  auto formal_iter=function->formals_->GetList().begin();

  while(arg_iter!=this->args_->GetList().end() &&
        formal_iter!=function->formals_->GetList().end()){
    auto arg_exp_and_ty=(*arg_iter)->Translate(venv,tenv,level,label,errormsg);
    auto arg_ty=arg_exp_and_ty->ty_;
    auto formal_ty=(*formal_iter)->ActualTy();

    if (!arg_ty->IsSameType(type::NilTy::Instance())&&!arg_ty->IsSameType(formal_ty)) {
      errormsg->Error(this->pos_, "para type mismatch");
      return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::NilTy::Instance());
    }
    ++arg_iter;
    ++formal_iter;

    params->Append(arg_exp_and_ty->exp_->UnEx());
  }
  level->frame_->AllocOutgoSpace(params->GetList().size()-reg_manager->ArgRegs()->GetList().size());
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::CallExp(
        new tree::NameExp(this->func_),
        params
      )
    ),
    function->result_->ActualTy()
  );
  
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  
  auto left_exp_and_ty=this->left_->Translate(venv,tenv,level,label,errormsg);
  auto right_exp_and_ty=this->right_->Translate(venv,tenv,level,label,errormsg);
  if(!(left_exp_and_ty->ty_->IsSameType(right_exp_and_ty->ty_)))
  {
    errormsg->Error(this->pos_,"operands are not the same type");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
  }

  if (this->oper_ == Oper::AND_OP || this->oper_ == Oper::OR_OP) {
    auto left_cx = left_exp_and_ty->exp_->UnCx(errormsg);
    auto right_cx = right_exp_and_ty->exp_->UnCx(errormsg);
    auto right_label = temp::LabelFactory::NewLabel();
    auto stm = new tree::SeqStm(
        left_cx.stm_,
        new tree::SeqStm(new tree::LabelStm(right_label), right_cx.stm_));

    if (this->oper_ == Oper::AND_OP) {
      left_cx.trues_.DoPatch(right_label);
      return new tr::ExpAndTy(
          new tr::CxExp(right_cx.trues_,
                        tr::PatchList::JoinPatch(left_cx.falses_,
                                                 right_cx.falses_),
                        stm),
          type::IntTy::Instance());
    }

    left_cx.falses_.DoPatch(right_label);
    return new tr::ExpAndTy(
        new tr::CxExp(
            tr::PatchList::JoinPatch(left_cx.trues_, right_cx.trues_),
            right_cx.falses_, stm),
        type::IntTy::Instance());
  }

  auto left_ex=left_exp_and_ty->exp_->UnEx();
  auto right_ex=right_exp_and_ty->exp_->UnEx();
  auto cjump=new tree::CjumpStm(tree::RelOp::REL_OPER_COUNT,left_ex,right_ex,nullptr,nullptr);
  tr::PatchList t_patch(std::list<temp::Label**>{&cjump->true_label_});
  tr::PatchList f_patch(std::list<temp::Label**>{&cjump->false_label_});
  switch (this->oper_)
  {
  case absyn::Oper::PLUS_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(
          tree::BinOp::PLUS_OP,
          left_ex,
          right_ex
        )
      ),
      left_exp_and_ty->ty_->ActualTy()
    );
    break;
  case absyn::Oper::MINUS_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(
          tree::BinOp::MINUS_OP,
          left_ex,
          right_ex
        )
      ),
      left_exp_and_ty->ty_->ActualTy()
    );
    break;
  case absyn::Oper::TIMES_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(
          tree::BinOp::MUL_OP,
          left_ex,
          right_ex
        )
      ),
      left_exp_and_ty->ty_->ActualTy()
    );
    break;
  case absyn::Oper::DIVIDE_OP:
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::BinopExp(
          tree::BinOp::DIV_OP,
          left_ex,
          right_ex
        )
      ),
      left_exp_and_ty->ty_->ActualTy()
    );
    break;
  case Oper::EQ_OP:
    if(left_exp_and_ty->ty_->IsSameType(type::StringTy::Instance())){
      auto params=new tree::ExpList();
      params->Append(left_ex);
      params->Append(right_ex);
      return new tr::ExpAndTy(new tr::ExExp(level->frame_->ExternalCall("string_equal",params)),type::IntTy::Instance());
    }
    else{
      cjump->op_=tree::RelOp::EQ_OP;
      return new tr::ExpAndTy(
        new tr::CxExp(
          t_patch,
          f_patch,
          cjump
        ),
        type::IntTy::Instance()
      );
    }
    break;
  case Oper::NEQ_OP:
    if(left_exp_and_ty->ty_->IsSameType(type::StringTy::Instance())){
      auto params=new tree::ExpList;
      params->Append(left_ex);
      params->Append(right_ex);
      return new tr::ExpAndTy(new tr::ExExp(
        new tree::BinopExp(
          tree::BinOp::MINUS_OP,
          new tree::ConstExp(1),
          level->frame_->ExternalCall("string_equal", params)
          )
        ),
        type::IntTy::Instance()
      );
    }
    else{
      cjump->op_=tree::RelOp::NE_OP;
      return new tr::ExpAndTy(
        new tr::CxExp(
          t_patch,
          f_patch,
          cjump
        ),
        type::IntTy::Instance()
      );
    }
    break;
  case Oper::GE_OP:
    cjump->op_=tree::RelOp::GE_OP;
    return new tr::ExpAndTy(
      new tr::CxExp(
        t_patch,
        f_patch,
        cjump
      ),
      type::IntTy::Instance()
    );
    break;
  case Oper::GT_OP:
    cjump->op_=tree::RelOp::GT_OP;
    return new tr::ExpAndTy(
      new tr::CxExp(
        t_patch,
        f_patch,
        cjump
      ),
      type::IntTy::Instance()
    );
    break;
  case Oper::LE_OP:
    cjump->op_=tree::RelOp::LE_OP;
    return new tr::ExpAndTy(
      new tr::CxExp(
        t_patch,
        f_patch,
        cjump
      ),
      type::IntTy::Instance()
    );
    break;
  case Oper::LT_OP:
    cjump->op_=tree::RelOp::LT_OP;
    return new tr::ExpAndTy(
      new tr::CxExp(
        t_patch,
        f_patch,
        cjump
      ),
      type::IntTy::Instance()
    );
    break;
  default:
    assert(false);
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::ConstExp(0)
      ),
      type::VoidTy::Instance()
    );
    break;
  }
  assert(false);
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::ConstExp(0)
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  
  auto type_=tenv->Look(this->typ_);
  if(!type_)
  {
    errormsg->Error(this->pos_, "undefined type");
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::ConstExp(0)
      ),
      type::VoidTy::Instance()
    );
  }
  if(typeid(*(type_->ActualTy()))!=typeid(type::RecordTy))
  {
    errormsg->Error(this->pos_,"not a record type");
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::ConstExp(0)
      ),
      type::VoidTy::Instance()
    );
  }
  auto type=static_cast<type::RecordTy *>(type_->ActualTy());
  if(type->fields_->GetList().size()!=this->fields_->GetList().size())
  {
    errormsg->Error(this->pos_, "number of fields does not match:");
    return new tr::ExpAndTy(
      new tr::ExExp(
        new tree::ConstExp(0)
      ),
      type::NilTy::Instance()
    );
  }


  auto params=new tree::ExpList();
  auto result=temp::TempFactory::NewTemp();
  params->Append(new tree::ConstExp(type->fields_->GetList().size()*reg_manager->WordSize()));
  tree::Stm *move_stms=new tree::MoveStm(
    new tree::TempExp(result),
    level->frame_->ExternalCall("alloc_record",params)
  );
  int offset=0;
  auto field_ty_iter=type->fields_->GetList().begin();
  for(auto field:this->fields_->GetList())
  {
    auto field_exp_and_ty = field->exp_->Translate(venv,tenv,level,label,errormsg);
    // auto actual_type=field_exp_and_ty->ty_->ActualTy();
    if(field->name_->Name()!=(*field_ty_iter)->name_->Name())
    {
      errormsg->Error(this->pos_, "field name does not match");
    }
    if(!field_exp_and_ty->ty_->IsSameType((*field_ty_iter)->ty_->ActualTy()))
    {
      errormsg->Error(this->pos_, "type of field does not match");
    }
    ++field_ty_iter;

    auto move_stm=new tree::MoveStm(
      new tree::MemExp(
        new tree::BinopExp(
          tree::PLUS_OP,
          new tree::TempExp(result),
          new tree::ConstExp(offset)
        )
      ),
      field_exp_and_ty->exp_->UnEx()
    );
    move_stms = new tree::SeqStm(
      move_stms,
      move_stm
    );
    offset+=reg_manager->WordSize();
  }
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        move_stms,
        new tree::TempExp(result)
      )
    ),
    type->ActualTy()
  );
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  
  tree::Exp *result=nullptr;
  tr::ExpAndTy *last=nullptr;
  for(auto seq:this->seq_->GetList())
  {
    last=seq->Translate(venv,tenv,level,label,errormsg);
    if(!result)
    {
      result=last->exp_->UnEx();
    }
    else
    {
      result=new tree::EseqExp(
        new tree::ExpStm(result),
        last->exp_->UnEx()
      );
    }
  }
  return new tr::ExpAndTy(
    new tr::ExExp(
      result
    ),
    last->ty_
  );
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  
  auto var_exp_and_ty=this->var_->Translate(venv,tenv,level,label,errormsg);
  auto exp_exp_and_ty=this->exp_->Translate(venv,tenv,level,label,errormsg);
  if(!var_exp_and_ty->ty_->IsSameType(exp_exp_and_ty->ty_)
      && ! var_exp_and_ty->ty_->IsSameType(type::NilTy::Instance())){
    errormsg->Error(this->pos_, "type of var and exp does not match");
  }
  else{
    if(typeid(*(this->var_))==typeid(absyn::SimpleVar))
    {
      auto var=static_cast<absyn::SimpleVar *>(this->var_);
      auto entry=static_cast<env::VarEntry *>(venv->Look(var->sym_));
      if(entry->readonly_){
        errormsg->Error(this->pos_, "readonly variable");
      }
    }
  }

  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::MoveStm(
        var_exp_and_ty->exp_->UnEx(),
        exp_exp_and_ty->exp_->UnEx()
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  
  auto test_exp_and_ty=this->test_->Translate(venv, tenv, level, label, errormsg);
  auto then_exp_and_ty=this->then_->Translate(venv, tenv, level, label, errormsg);
  if(errormsg->AnyErrors())
  {
    return then_exp_and_ty;
  }
  auto t_branch=temp::LabelFactory::NewLabel();
  auto f_branch=temp::LabelFactory::NewLabel();
  auto test_cx=test_exp_and_ty->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(t_branch);
  test_cx.falses_.DoPatch(f_branch);
  if(this->elsee_){
    auto elsee_exp_and_ty=this->elsee_->Translate(venv, tenv, level, label, errormsg);
    auto endif_label=temp::LabelFactory::NewLabel();
    if(!then_exp_and_ty->ty_->IsSameType(elsee_exp_and_ty->ty_))
    {
      errormsg->Error(this->pos_, "then exp and else exp type mismatch");
    }
    if(then_exp_and_ty->ty_->IsSameType(type::VoidTy::Instance()))
    {
      return new tr::ExpAndTy(
        new tr::NxExp(
          new tree::SeqStm(
            test_cx.stm_,
            new tree::SeqStm(
              new tree::SeqStm(
                new tree::LabelStm(
                  t_branch
                ),
                new tree::SeqStm(
                  then_exp_and_ty->exp_->UnNx(),
                  new tree::JumpStm(
                    new tree::NameExp(endif_label),
                    new std::vector<temp::Label *>{endif_label}
                  )
                )
              ),
              new tree::SeqStm(
                new tree::LabelStm(
                  f_branch
                ),
                new tree::SeqStm(
                  elsee_exp_and_ty->exp_->UnNx(),
                  new tree::LabelStm(endif_label)
                )
              )
            )
          )
        ),
        type::VoidTy::Instance()
      );
    }
    else
    {
      auto result=temp::TempFactory::NewTemp();
      return new tr::ExpAndTy(
        new tr::ExExp(
          new tree::EseqExp(
            new tree::SeqStm(
              test_cx.stm_,
              new tree::SeqStm(
                new tree::SeqStm(
                  new tree::LabelStm(t_branch),
                  new tree::SeqStm(
                    new tree::MoveStm(
                      new tree::TempExp(result),
                      then_exp_and_ty->exp_->UnEx()
                    ),
                    new tree::JumpStm(
                      new tree::NameExp(endif_label),
                      new std::vector<temp::Label *>{endif_label}
                    )
                  )
                ),
                new tree::SeqStm(
                  new tree::LabelStm(f_branch),
                  new tree::SeqStm(
                    new tree::MoveStm(
                      new tree::TempExp(result),
                      elsee_exp_and_ty->exp_->UnEx()
                    ),
                    new tree::LabelStm(endif_label)
                  )
                )
              )
            ),
            new tree::TempExp(result)
          )
        ),
        then_exp_and_ty->ty_->ActualTy()
      );
    }
  }
  else{
    if(!then_exp_and_ty->ty_->IsSameType(type::VoidTy::Instance()))
    {
      errormsg->Error(this->pos_, "if-then exp's body must produce no value");
    }
    return new tr::ExpAndTy(
      new tr::NxExp(
        new tree::SeqStm(
          test_cx.stm_,
          new tree::SeqStm(
            new tree::LabelStm(
              t_branch
            ),
            new tree::SeqStm(
              then_exp_and_ty->exp_->UnNx(),
              new tree::LabelStm(
                f_branch
              )
            )
          )
        )
      ),
      type::VoidTy::Instance()
    );
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  
  auto done_label=temp::LabelFactory::NewLabel();
  auto test_label=temp::LabelFactory::NewLabel();
  auto body_label=temp::LabelFactory::NewLabel();
  auto test_exp_and_ty=this->test_->Translate(venv, tenv, level, label, errormsg);
  auto body_exp_and_ty=this->body_->Translate(venv, tenv, level, done_label, errormsg);
  if(!body_exp_and_ty->ty_->IsSameType(type::VoidTy::Instance()))
  {
    errormsg->Error(pos_, "while body must produce no value");
  }
  auto test_cx=test_exp_and_ty->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(body_label);
  test_cx.falses_.DoPatch(done_label);

  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::SeqStm(
        new tree::LabelStm(test_label),
        new tree::SeqStm(
          test_cx.stm_,
          new tree::SeqStm(
            new tree::LabelStm(body_label),
            new tree::SeqStm(
              body_exp_and_ty->exp_->UnNx(),
              new tree::SeqStm(
                new tree::JumpStm(
                  new tree::NameExp(test_label),
                  new std::vector<temp::Label *>{test_label}
                ),
                new tree::LabelStm(done_label)
              )
            )
          )
        )
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  
  // var iter := lo
  auto iter_dec=new VarDec(this->pos_,this->var_,nullptr,this->lo_);
  // var limit := hi
  auto hi_bound_sym=sym::Symbol::UniqueSymbol(this->var_->Name()+"_"+std::to_string(this->pos_));
  auto hi_bound_dec=new VarDec(this->pos_,hi_bound_sym,nullptr,this->hi_);
  auto declist=new DecList();
  hi_bound_dec->escape_=false;
  iter_dec->escape_=this->escape_;
  declist->Prepend(hi_bound_dec);
  declist->Prepend(iter_dec);
  auto iter_var=new SimpleVar(this->pos_,this->var_);
  auto hi_bound_var=new SimpleVar(this->pos_,hi_bound_sym);
  // iter := iter + 1
  auto incr_exp=new AssignExp(this->pos_,new SimpleVar(this->pos_,this->var_),
    new OpExp(this->pos_,absyn::Oper::PLUS_OP,
      new VarExp(this->pos_,iter_var),new IntExp(this->pos_,1)));
  auto bodylist=new ExpList();
  bodylist->Prepend(incr_exp);
  bodylist->Prepend(this->body_);
  // while test:
  //   body
  auto while_exp=new WhileExp(this->pos_,
    new OpExp(this->pos_,absyn::Oper::LE_OP,
      new VarExp(this->pos_,iter_var),new VarExp(this->pos_,hi_bound_var)),
    new SeqExp(this->pos_,bodylist));
  auto let_exp=new LetExp(this->pos_,declist,while_exp);
  auto let_exp_and_ty = let_exp->Translate(venv, tenv, level, label, errormsg);
  // venv->Look(this->var_)->readonly_=true;
  return let_exp_and_ty;
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  
  if(!label)
  {
    errormsg->Error(this->pos_, "break must be in loop.");
  }
  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::JumpStm(
        new tree::NameExp(
          label
        ),
        new std::vector<temp::Label *>{
          label
        }
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm *decs_stm=nullptr;
  for(auto dec:this->decs_->GetList())
  {
    auto dec_exp_and_ty=dec->Translate(venv, tenv, level, label, errormsg);
    if(decs_stm==nullptr){
      decs_stm=dec_exp_and_ty->UnNx();
    }
    else{
      decs_stm=new tree::SeqStm(
        decs_stm,
        dec_exp_and_ty->UnNx()
      );
    }
  }
  auto body_exp_and_ty=this->body_->Translate(venv, tenv, level, label, errormsg);
  venv->EndScope();
  tenv->EndScope();
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        decs_stm,
        body_exp_and_ty->exp_->UnEx()
      )
    ),
    body_exp_and_ty->ty_
  );
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  
  auto type_=tenv->Look(this->typ_);
  auto result=temp::TempFactory::NewTemp();
  if(!type_)
  {
    errormsg->Error(this->pos_, "undeclared type %s", this->typ_->Name().c_str());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  if(typeid(*(type_->ActualTy()))!=typeid(type::ArrayTy))
  {
    errormsg->Error(this->pos_, "undefined type %s", this->typ_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  auto type=static_cast<type::ArrayTy *>(type_);
  auto size_exp_and_ty=this->size_->Translate(venv, tenv, level, label, errormsg);
  if(!size_exp_and_ty->ty_->IsSameType(type::IntTy::Instance()))
  {
    errormsg->Error(this->pos_, "array size must be int type");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  auto init_exp_and_ty=this->init_->Translate(venv, tenv, level, label, errormsg);
  if(!init_exp_and_ty->ty_->IsSameType(static_cast<type::ArrayTy *>(type->ActualTy())->ty_)){
    errormsg->Error(this->pos_, "type mismatch");
  }
  auto params=new tree::ExpList();
  params->Append(size_exp_and_ty->exp_->UnEx());
  params->Append(init_exp_and_ty->exp_->UnEx());
  auto alloc_exp=level->frame_->ExternalCall("init_array",params);
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::EseqExp(
        new tree::MoveStm(
          new tree::TempExp(result),
          alloc_exp
        ),
        new tree::TempExp(result)
      )
    ),
    type->ActualTy()
  );
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  

  int outer_no=0,inner_no=0;
  for(const auto &fundec: this->functions_->GetList())
  {
    inner_no=0;
    for(const auto &another: this->functions_->GetList())
    {
      if(outer_no > inner_no && fundec->name_==another->name_)
      {
        errormsg->Error(this->pos_,"two functions have the same name");
      }
      ++inner_no;
    }
    if(fundec->result_)
    {
      auto result_ty=tenv->Look(fundec->result_);
      if(!result_ty)
      {
        errormsg->Error(this->pos_,"Invalid result type, name: %s",fundec->result_->Name().c_str());
        continue;
      }
    }
    ++outer_no;

    auto params_ty_list=fundec->params_->MakeFormalTyList(tenv,errormsg);
    auto func_label=temp::LabelFactory::NamedLabel(fundec->name_->Name());
    type::Ty *result_type = type::VoidTy::Instance();
    if(fundec->result_)
    {
      result_type = tenv->Look(fundec->result_);
      if(!result_type)
      {
        errormsg->Error(this->pos_,"Invalid parameter type, name:%s",fundec->result_->Name().data());
        venv->EndScope();
        continue;
      }
    }
    std::list<bool> params_escape_list{true}; 
    for(const auto &param:fundec->params_->GetList())
      params_escape_list.push_back(param->escape_);
    auto func_level=level->NewLevel(level,func_label,params_escape_list);
    venv->Enter(fundec->name_,new env::FunEntry(func_level,func_label,params_ty_list,result_type));
  }

  for(const auto &fundec: this->functions_->GetList())
  {
    venv->BeginScope();
    auto func_entry=static_cast<env::FunEntry *>(venv->Look(fundec->name_));

    auto func_level=func_entry->level_;
    auto param_info_iter=fundec->params_->GetList().begin();
    auto param_access_iter=func_level->frame_->Formals()->begin();
    if(errormsg->AnyErrors())
    {
      continue;
    }
    for(++param_access_iter;param_access_iter!=func_level->frame_->Formals()->end() && param_info_iter!=fundec->params_->GetList().end();param_access_iter++,param_info_iter++)
    {
      auto param_field=*param_info_iter;
      auto param_name=param_field->name_;
      auto param_type=tenv->Look(param_field->typ_);
      venv->Enter(param_name,new env::VarEntry(new tr::Access(func_level,*param_access_iter),param_type));
    }
    assert(param_info_iter==fundec->params_->GetList().end() && param_access_iter==func_level->frame_->Formals()->end());
    // for (const auto &param : fundec->params_->GetList())
    // {
    //   auto param_type = tenv->Look(param->typ_);
    //   auto param_name = param->name_;
    //   venv->Enter(param_name,new env::VarEntry(tr::Access::AllocLocal(func_level,param->escape_),param_type));
    // }

    auto fundec_exp_and_ty=fundec->body_->Translate(venv,tenv,func_level,label,errormsg);
    if(!fundec_exp_and_ty->ty_->IsSameType(func_entry->result_))
    {
      if(func_entry->result_->IsSameType(type::VoidTy::Instance()))
        errormsg->Error(this->pos_, "procedure returns value");
      else
        errormsg->Error(this->pos_, "function body must be same type in function:%s.",fundec->name_->Name().data());
    }
    ProcEntryExit(func_level,fundec_exp_and_ty->exp_);
    venv->EndScope();
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  
  auto init_exp_and_ty=this->init_->Translate(venv, tenv, level, label, errormsg);

  auto space=tr::Access::AllocLocal(level,this->escape_);
  type::Ty *var_type=nullptr;

  if(this->typ_)
  {
    auto var_type=tenv->Look(this->typ_);
    if(!var_type)
    {
      errormsg->Error(this->pos_, "undeclared type %s", this->typ_->Name().c_str());
      return new tr::ExExp(new tree::ConstExp(0));
    }
    var_type=static_cast<type::Ty *>(var_type);
    // venv->Enter(this->var_,new env::VarEntry(space,type));
  }
  else
  {
    auto init_ty=init_exp_and_ty->ty_;
    if(typeid(*init_ty)==typeid(type::NilTy)
        && !(this->init_ && typeid(*(this->init_))==typeid(absyn::RecordExp)))
    {
      errormsg->Error(this->pos_,"init should not be nil without type specified");
    }
  }
  var_type=init_exp_and_ty->ty_;
  venv->Enter(this->var_,new env::VarEntry(space,var_type));
  return new tr::NxExp(
    new tree::MoveStm(
      space->access_->ToExp(new tree::TempExp(reg_manager->FramePointer())),
      init_exp_and_ty->exp_->UnEx()
    )
  );
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  
  int outer_no=0,inner_no=0;
  for(const auto &absyn_ty:this->types_->GetList())
  {
    inner_no=0;
    for(const auto &another:this->types_->GetList())
    {
      if(outer_no>inner_no && absyn_ty->name_==another->name_){
        errormsg->Error(this->pos_,"two types have the same name");
        break;
      }
      ++inner_no;
    }
    tenv->Enter(absyn_ty->name_,new type::NameTy(absyn_ty->name_,nullptr));

    ++outer_no;
  }

  for(const auto &absyn_ty:this->types_->GetList())
  {
    auto ty = static_cast<type::NameTy *>(tenv->Look(absyn_ty->name_));
    ty->ty_=absyn_ty->ty_->Translate(tenv,errormsg);

    if(!ty->ty_)
    {
      errormsg->Error(this->pos_,"Failed to analyze type %s",
                      ty->sym_->Name().data());
      continue;
    }

    auto start=ty;
    for(auto iter=start->ty_;iter && typeid(*iter)==typeid(type::NameTy);iter=static_cast<type::NameTy *>(iter)->ty_)
    {
      if(iter==start)
      {
        errormsg->Error(this->pos_,"illegal type cycle");
        break;
      }
    }
  }

  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  
  return tenv->Look(this->name_);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  
  type::RecordTy *ty=new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));
  return ty;
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  
  auto result = tenv->Look(this->array_);
  if (!result) {
    errormsg->Error(this->pos_, "undefined type %s",
                    this->array_->Name().data());
    return type::NilTy::Instance();
  }
  return static_cast<type::Ty *>(new type::ArrayTy(result));
}

} // namespace absyn

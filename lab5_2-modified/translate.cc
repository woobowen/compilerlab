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
  /* TODO: Put your lab5 code here */
  frame::Access *access=level->frame_->AllocLocal(escape);
  return new Access(level,access);
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stmt) {}
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

  ExpAndTy(tr::Exp *expr, type::Ty *ty) : exp_(expr), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return this->exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
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
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(this->stm_,new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return this->stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    std::cerr<<"nx can't be converted to test_ expr"<<std::endl;
    assert(false);
    return Cx({}, {}, nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stmt) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    // 表示该条件语句的结果
    temp::Temp *result=temp::TempFactory::NewTemp();
    // 当条件为真或假时分别需要跳转到哪里
    temp::Label *t_branch=temp::LabelFactory::NewLabel();
    temp::Label *f_branch=temp::LabelFactory::NewLabel();

    this->cx_.trues_.DoPatch(true_lbl);
    this->cx_.falses_.DoPatch(false_lbl);

    // 这个表达式在条件为真时值为1，否则为0
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
    return expr;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return this->cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) const override {
    /* TODO: Put your lab5 code here */
    return this->cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
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
  /* TODO: Put your lab5 code here */
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
  /* TODO: Put your lab5 code here */
  return this->root_->Translate(venv, tenv, level, lbl, errormsg);
}

// 对简单变量的中间代码翻译，可能需要处理静态链
tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto _entry =venv->Look(this->sym_);
  // 对原始的_entry做一些类型检查
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
  // 如果该变量定义于上一层，那么我们遍历静态链直至找到所需的栈帧
  // Bug to be fixed
  // **注意静态链的位置!!**
  while(current_level != access_level){
    // 静态链应该是函数的第一个参数
    frameptr = level->frame_->Formals()->front()->ToExp(frameptr);
    current_level = current_level->parent_;
  }
  return new tr::ExpAndTy(new tr::ExExp(access->ToExp(frameptr)),
                          entry->ty_);
}

// 对记录类型的域进行处理
// 我们约定在内存中各个域按照存入FieldList中的先后顺序来存储
tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto exp_and_ty=this->var_->Translate(venv,tenv,level,lbl,errormsg);
  auto ty = exp_and_ty->ty_->ActualTy();
  if(typeid(*(ty))!=typeid(type::RecordTy))
  {
    errormsg->Error(pos_, "variable %s is not a record",
      sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
  }
  // 查找目标字段
  // 如果找到了目标字段，我们还需要知道其相对于基地址的偏移
  int mem_offset=0;
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
    ++mem_offset;
  }
  // 没有找到，则需要报错
  errormsg->Error(this->pos_,"field %s not exist",this->sym_->Name().data());
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
          type::VoidTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_exp_and_ty=this->var_->Translate(venv,tenv,level,lbl,errormsg);
  auto subscript_exp_and_ty=this->subscript_->Translate(venv,tenv,level,lbl,errormsg);
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
          var_exp_and_ty->exp_->UnEx(), // 这里不再需要取Mem，因为数组本身的值就是地址
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
  /* TODO: Put your lab5 code here */
  return this->var_->Translate(venv, tenv, level, lbl, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(this->val_)),type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto str_label=temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(str_label,this->str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(str_label)),
                          type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // 首先检查是否存在
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
  // 参数列表首先加入静态链
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
    arg_list->Append(static_link);
  }
  auto arg_it=this->args_->GetList().begin();
  auto formal_it=function->formals_->GetList().begin();

  while(arg_it!=this->args_->GetList().end() &&
        formal_it!=function->formals_->GetList().end()){
    auto arg_exp_and_ty=(*arg_it)->Translate(venv,tenv,level,lbl,errormsg);
    auto arg_ty=arg_exp_and_ty->ty_;
    auto formal_ty=(*formal_it)->ActualTy();

    if (!arg_ty->IsSameType(type::NilTy::Instance())&&!arg_ty->IsSameType(formal_ty)) {
      errormsg->Error(this->pos_, "para type mismatch");
      return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::NilTy::Instance());
    }
    ++arg_it;
    ++formal_it;

    // 做完了类型检查后，将当前实参添加进去
    arg_list->Append(arg_exp_and_ty->exp_->UnEx());
  }
  // 更新外部参数个数
  level->frame_->AllocOutgoSpace(arg_list->GetList().size()-reg_manager->ArgRegs()->GetList().size());
  // 返回表达式和类型
  return new tr::ExpAndTy(
    new tr::ExExp(
      new tree::CallExp(
        new tree::NameExp(this->func_),
        arg_list
      )
    ),
    function->result_->ActualTy()
  );
  /* End for lab5 code */
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // 获取左右两边的操作数/字符串
  auto left_exp_and_ty=this->left_->Translate(venv,tenv,level,lbl,errormsg);
  auto right_exp_and_ty=this->right_->Translate(venv,tenv,level,lbl,errormsg);
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
                        stmt),
          type::IntTy::Instance());
    }

    left_cx.falses_.DoPatch(right_label);
    return new tr::ExpAndTy(
        new tr::CxExp(
            tr::PatchList::JoinPatch(left_cx.trues_, right_cx.trues_),
            right_cx.falses_, stmt),
        type::IntTy::Instance());
  }

  auto left_ex=left_exp_and_ty->exp_->UnEx();
  auto right_ex=right_exp_and_ty->exp_->UnEx();
  auto cjump=new tree::CjumpStm(tree::RelOp::REL_OPER_COUNT,left_ex,right_ex,nullptr,nullptr);
  tr::PatchList t_patch(std::list<temp::Label**>{&cjump->true_label_});
  tr::PatchList f_patch(std::list<temp::Label**>{&cjump->false_label_});
  // TODO:类型检查
  switch (this->oper_)
  {
  // 算术运算符
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
  // 逻辑运算符
  case Oper::EQ_OP:
    if(left_exp_and_ty->ty_->IsSameType(type::StringTy::Instance())){
      // 语法糖：如果是字符串类型，那么调用外部函数stringEqual]
      auto params=new tree::ExpList();
      arg_list->Append(left_ex);
      arg_list->Append(right_ex);
      return new tr::ExpAndTy(new tr::ExExp(level->frame_->ExternalCall("string_equal",arg_list)),type::IntTy::Instance());
    }
    else{
      // 由于不知道Cx需要向哪里跳转，因此后两个参数留空，待之后填充
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
      arg_list->Append(left_ex);
      arg_list->Append(right_ex);
      return new tr::ExpAndTy(new tr::ExExp(
        new tree::BinopExp(
          tree::BinOp::MINUS_OP,
          new tree::ConstExp(1),
          level->frame_->ExternalCall("string_equal", arg_list)
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
  /* TODO: Put your lab5 code here */
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


  // 需要调用外部函数recordAlloc来为记录分配空间
  auto params=new tree::ExpList();
  // result存储分配的记录内存空间的地址
  auto result=temp::TempFactory::NewTemp();
  params->Append(new tree::ConstExp(type->fields_->GetList().size()*reg_manager->WordSize()));
  tree::Stm *move_stms=new tree::MoveStm(
    new tree::TempExp(result),
    level->frame_->ExternalCall("alloc_record",arg_list)
  );
  int mem_offset=0;
  auto field_ty_it=type->fields_->GetList().begin();
  for(auto field:this->fields_->GetList())
  {
    auto field_exp_and_ty = field->exp_->Translate(venv,tenv,level,lbl,errormsg);
    // auto actual_type=field_exp_and_ty->ty_->ActualTy();
    if(field->name_->Name()!=(*field_ty_it)->name_->Name())
    {
      errormsg->Error(this->pos_, "field name does not match");
    }
    if(!field_exp_and_ty->ty_->IsSameType((*field_ty_it)->ty_->ActualTy()))
    {
      errormsg->Error(this->pos_, "type of field does not match");
    }
    ++field_ty_it;

    // 将等号右边的值存入对应的区域
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
    // 将move语句添加进来
    move_stms = new tree::SeqStm(
      move_stms,
      move_stmt
    );
    mem_offset+=reg_manager->WordSize();
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
  /* TODO: Put your lab5 code here */
  tree::Exp *result=nullptr;
  tr::ExpAndTy *last=nullptr;
  for(auto seq:this->seq_->GetList())
  {
    last=seq->Translate(venv,tenv,level,lbl,errormsg);
    if(!res)
    {
      res=last->exp_->UnEx();
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
      res
    ),
    last->ty_
  );
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto var_exp_and_ty=this->var_->Translate(venv,tenv,level,lbl,errormsg);
  auto exp_exp_and_ty=this->exp_->Translate(venv,tenv,level,lbl,errormsg);
  if(!var_exp_and_ty->ty_->IsSameType(exp_exp_and_ty->ty_)
      && ! var_exp_and_ty->ty_->IsSameType(type::NilTy::Instance())){
    errormsg->Error(this->pos_, "type of var and expr does not match");
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
  /* TODO: Put your lab5 code here */
  auto test_exp_and_ty=this->test_->Translate(venv, tenv, level, lbl, errormsg);
  auto then_exp_and_ty=this->then_->Translate(venv, tenv, level, lbl, errormsg);
  if(errormsg->AnyErrors())
  {
    return then_exp_and_ty;
  }
  auto t_branch=temp::LabelFactory::NewLabel();
  auto f_branch=temp::LabelFactory::NewLabel();
  auto test_cx=test_exp_and_ty->exp_->UnCx(errormsg);
  test_cx.trues_.DoPatch(true_lbl);
  test_cx.falses_.DoPatch(false_lbl);
  // 接下来分有else语句和无else语句两种情况处理
  if(this->elsee_){
    auto elsee_exp_and_ty=this->elsee_->Translate(venv, tenv, level, lbl, errormsg);
    auto endif_label=temp::LabelFactory::NewLabel();
    if(!then_exp_and_ty->ty_->IsSameType(elsee_exp_and_ty->ty_))
    {
      errormsg->Error(this->pos_, "then expr and else expr type mismatch");
    }
    // 处理then和else均可翻译为Nx的情况
    if(then_exp_and_ty->ty_->IsSameType(type::VoidTy::Instance()))
    {
      return new tr::ExpAndTy(
        new tr::NxExp(
          new tree::SeqStm(
            test_cx.stm_,
            new tree::SeqStm(
              // then分支
              new tree::SeqStm(
                new tree::LabelStm(
                  true_lbl
                ),
                new tree::SeqStm(
                  then_exp_and_ty->exp_->UnNx(),
                  new tree::JumpStm(
                    new tree::NameExp(endif_label),
                    new std::vector<temp::Label *>{endif_label}
                  )
                )
              ),
              // else分支
              new tree::SeqStm(
                new tree::LabelStm(
                  false_lbl
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
                // then分支
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
                // else分支
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
  // 无else语句时，then语句必须是Void类型的，不需要result
  else{
    if(!then_exp_and_ty->ty_->IsSameType(type::VoidTy::Instance()))
    {
      errormsg->Error(this->pos_, "if-then expr's body must produce no value");
    }
    return new tr::ExpAndTy(
      new tr::NxExp(
        new tree::SeqStm(
          test_cx.stm_,
          new tree::SeqStm(
            new tree::LabelStm(
              true_lbl
            ),
            new tree::SeqStm(
              then_exp_and_ty->exp_->UnNx(),
              new tree::LabelStm(
                false_lbl
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
  /* TODO: Put your lab5 code here */
  auto done_label=temp::LabelFactory::NewLabel();
  auto test_label=temp::LabelFactory::NewLabel();
  auto body_label=temp::LabelFactory::NewLabel();
  auto test_exp_and_ty=this->test_->Translate(venv, tenv, level, lbl, errormsg);
  // 这里将done_label传递给body进行翻译，以便确定break跳转到何处
  auto body_exp_and_ty=this->body_->Translate(venv, tenv, level, done_label, errormsg);
  if(!body_exp_and_ty->ty_->IsSameType(type::VoidTy::Instance()))
  {
    errormsg->Error(pos_, "while body must produce no value");
  }
  // test语句翻译成为一个条件跳转，如果判断成功跳转到body_label，否则跳转到done_label
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

// 根据ppt上的提示，我们重新构造一个let语句，完成for循环的功能，这样可以提高复用性
tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // var iter := lo
  auto iter_dec=new VarDec(this->pos_,this->var_,nullptr,this->lo_);
  // var limit := hi
  auto hi_bound_sym=sym::Symbol::UniqueSymbol(this->var_->Name()+"_"+std::to_string(this->pos_));
  auto hi_bound_dec=new VarDec(this->pos_,hi_bound_sym,nullptr,this->hi_);
  // 把所有的定义放在列表中
  auto declist=new DecList();
  // for循环的escape事实上就是用来标识该迭代变量是否为escape的
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
  // 构造循环体
  auto bodylist=new ExpList();
  bodylist->Prepend(incr_exp);
  bodylist->Prepend(this->body_);
  // while test:
  //   body
  auto while_exp=new WhileExp(this->pos_,
    new OpExp(this->pos_,absyn::Oper::LE_OP,
      new VarExp(this->pos_,iter_var),new VarExp(this->pos_,hi_bound_var)),
    new SeqExp(this->pos_,bodylist));
  // 用let语句将上面的所有东西结合在一起
  auto let_exp=new LetExp(this->pos_,declist,while_exp);
  auto let_exp_and_ty = let_exp->Translate(venv, tenv, level, lbl, errormsg);
  // // 将迭代变量设置为只读
  // venv->Look(this->var_)->readonly_=true;
  return let_exp_and_ty;
}

// label是done_label，即跳出循环的目的位置
tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  if(!lbl)
  {
    errormsg->Error(this->pos_, "break must be in loop.");
  }
  // 一个普通的jump语句
  return new tr::ExpAndTy(
    new tr::NxExp(
      new tree::JumpStm(
        new tree::NameExp(
          lbl
        ),
        new std::vector<temp::Label *>{
          lbl
        }
      )
    ),
    type::VoidTy::Instance()
  );
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  tree::Stm *decs_stm=nullptr;
  // 将所有的定义组合成一个SeqStm
  for(auto dec:this->decs_->GetList())
  {
    auto dec_exp_and_ty=dec->Translate(venv, tenv, level, lbl, errormsg);
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
  auto body_exp_and_ty=this->body_->Translate(venv, tenv, level, lbl, errormsg);
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
  /* TODO: Put your lab5 code here */
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
  auto size_exp_and_ty=this->size_->Translate(venv, tenv, level, lbl, errormsg);
  if(!size_exp_and_ty->ty_->IsSameType(type::IntTy::Instance()))
  {
    errormsg->Error(this->pos_, "array size must be int type");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  auto init_exp_and_ty=this->init_->Translate(venv, tenv, level, lbl, errormsg);
  if(!init_exp_and_ty->ty_->IsSameType(static_cast<type::ArrayTy *>(type->ActualTy())->ty_)){
    errormsg->Error(this->pos_, "type mismatch");
  }
  auto params=new tree::ExpList();
  arg_list->Append(size_exp_and_ty->exp_->UnEx());
  arg_list->Append(init_exp_and_ty->exp_->UnEx());
  auto alloc_exp=level->frame_->ExternalCall("init_array",arg_list);
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
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */

  // 首先注册所有的函数名
  // 如果没有声明返回类型，那么就是一个过程（或者说，一个返回值类型为Void的函数）
  int outer_no=0,inner_no=0;
  for(const auto &fundec: this->functions_->GetList())
  {
    // 检查是否有重名的函数以及函数的返回类型是否正确
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
        errormsg->Error(this->pos_,"Invalid res type, name: %s",fundec->result_->Name().c_str());
        continue;
      }
    }
    ++outer_no;

    // 注册函数
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
    std::list<bool> params_escape_list{true}; // 有一个隐含的静态链参数
    for(const auto &param:fundec->params_->GetList())
      params_escape_list.push_back(param->escape_);
    auto func_level=level->NewLevel(level,func_label,params_escape_list);
    venv->Enter(fundec->name_,new env::FunEntry(func_level,func_label,params_ty_list,result_type));
  }

  for(const auto &fundec: this->functions_->GetList())
  {
    // 处理函数体的翻译
    venv->BeginScope();
    auto func_entry=static_cast<env::FunEntry *>(venv->Look(fundec->name_));

    auto func_level=func_entry->level_;
    auto param_info_it=fundec->params_->GetList().begin();
    auto param_access_it=func_level->frame_->Formals()->begin();
    if(errormsg->AnyErrors())
    {
      continue;
    }
    for(++param_access_it;param_access_it!=func_level->frame_->Formals()->end() && param_info_it!=fundec->params_->GetList().end();param_access_it++,param_info_it++)
    {
      auto param_field=*param_info_it;
      auto param_name=param_field->name_;
      auto param_type=tenv->Look(param_field->typ_);
      venv->Enter(param_name,new env::VarEntry(new tr::Access(func_level,*param_access_it),param_type));
    }
    assert(param_info_it==fundec->params_->GetList().end() && param_access_it==func_level->frame_->Formals()->end());
    // for (const auto &param : fundec->params_->GetList())
    // {
    //   auto param_type = tenv->Look(param->typ_);
    //   auto param_name = param->name_;
    //   venv->Enter(param_name,new env::VarEntry(tr::Access::AllocLocal(func_level,param->escape_),param_type));
    // }

    auto fundec_exp_and_ty=fundec->body_->Translate(venv,tenv,func_level,lbl,errormsg);
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
  /* TODO: Put your lab5 code here */
  auto init_exp_and_ty=this->init_->Translate(venv, tenv, level, lbl, errormsg);

  // 使用AllocLocal为变量分配空间
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
  /* TODO: Put your lab5 code here */
  // 先注册
  // 与此同时检查有无重复的类型名称
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

  // 再分析具体的类型
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

    // 检查是否存在循环定义
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
  /* TODO: Put your lab5 code here */
  return tenv->Look(this->name_);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::RecordTy *ty=new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));
  return ty;
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  auto res = tenv->Look(this->array_);
  if (!res) {
    errormsg->Error(this->pos_, "undefined type %s",
                    this->array_->Name().data());
    return type::NilTy::Instance();
  }
  return static_cast<type::Ty *>(new type::ArrayTy(res));
}

} // namespace absyn

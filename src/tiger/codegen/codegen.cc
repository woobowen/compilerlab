#include "tiger/codegen/codegen.h"
// #include "tiger/frame/x64frame.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {

void CodeGen::Codegen() {
  std::string fs_=this->frame_->GetLabel() + "_framesize";
  auto list=new assem::InstrList();

  for(auto stm:this->traces_->GetStmList()->GetList())
  {
    stm->Munch(*list, fs_);
  }

  this->assem_instr_=
    std::make_unique<AssemInstr>(frame::ProcEntryExit2(list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  auto results=new temp::TempList();
  int num_args=this->exp_list_.size();
  int i=0;
  auto arg_iter=this->exp_list_.begin();
  for(auto reg:reg_manager->ArgRegs()->GetList())
  {
    if(arg_iter==this->exp_list_.end())
      break;
    auto temp=(*arg_iter)->Munch(instr_list,fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      new temp::TempList{reg},new temp::TempList{temp}));
    results->Append(temp);
    ++arg_iter;
    ++i;
  }
  auto fp_exp=new TempExp(reg_manager->FramePointer());
  while(arg_iter!=this->exp_list_.end())
  {
    auto temp=(*arg_iter)->Munch(instr_list,fs);
    std::ostringstream instr_builder;
    instr_builder<<"movq `s0, -"<<(num_args-i)*reg_manager->WordSize()<<"(`s1)";
    instr_list.Append(new assem::MoveInstr(instr_builder.str(),
      nullptr,new temp::TempList{temp,fp_exp->Munch(instr_list,fs)}));
    results->Append(temp);
    ++arg_iter;
    ++i;
  }
  return results;
}

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  instr_list.Append(new assem::LabelInstr(
      this->label_->Name(), this->label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream instr_builder;
  instr_builder << "jmp " << this->jumps_->front()->Name();
  instr_list.Append(new assem::OperInstr(instr_builder.str(), nullptr, nullptr,
                                         new assem::Targets(this->jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream instr_builder_cmp, instr_builder_jump;
  if (typeid(*(this->left_)) == typeid(tree::ConstExp)) {
    instr_builder_cmp << "cmpq `s0, $"
                      << static_cast<tree::ConstExp *>(this->left_)->consti_;
    auto right_op = this->right_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(instr_builder_cmp.str(), nullptr,
                                           new temp::TempList{right_op},
                                           nullptr));
  } else {
    instr_builder_cmp << "cmpq `s1, `s0";
    auto left_op = this->left_->Munch(instr_list, fs);
    auto right_op = this->right_->Munch(instr_list, fs);
    instr_list.Append(
        new assem::OperInstr(instr_builder_cmp.str(), nullptr,
                             new temp::TempList{left_op, right_op}, nullptr));
  }
  switch (this->op_) {
  case tree::RelOp::EQ_OP:
    instr_builder_jump << "je ";
    break;
  case tree::RelOp::NE_OP:
    instr_builder_jump << "jne ";
    break;
  case tree::RelOp::LT_OP:
    instr_builder_jump << "jl ";
    break;
  case tree::RelOp::LE_OP:
    instr_builder_jump << "jle ";
    break;
  case tree::RelOp::GT_OP:
    instr_builder_jump << "jg ";
    break;
  case tree::RelOp::GE_OP:
    instr_builder_jump << "jge ";
    break;
  default:
    assert(false);
    break;
  }
  instr_builder_jump << " `j0";
  instr_list.Append(
      new assem::OperInstr(instr_builder_jump.str(), nullptr, nullptr,
                           new assem::Targets(new std::vector<temp::Label *>{
                               this->true_label_, this->false_label_})));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream instr_builder;
  auto dst_list=new temp::TempList();
  auto src_list=new temp::TempList();
  int src_count=0;
  instr_builder << "movq ";
  if(typeid(*(this->src_)) == typeid(tree::ConstExp))
  {
    instr_builder << "$"<<static_cast<tree::ConstExp *>(this->src_)->consti_;
  }
  else
  {
    instr_builder << "`s"<<src_count++;
    src_list->Append(this->src_->Munch(instr_list, fs));
  }

  instr_builder << ", ";
  if (typeid(*(this->dst_))==typeid(tree::MemExp))
  {
    auto addr=static_cast<tree::MemExp *>(this->dst_)->exp_->Munch(instr_list, fs);
    instr_builder<<"(`s"<<src_count++<<")";
    src_list->Append(addr);
  }
  else
  {
    auto result=this->dst_->Munch(instr_list, fs);
    instr_builder<<"`d0";
    dst_list->Append(result);
  }
  instr_builder;
  instr_list.Append(new assem::MoveInstr(instr_builder.str(), dst_list, src_list));
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  this->exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream instr_builder;
  auto left = this->left_->Munch(instr_list, fs);
  auto right = this->right_->Munch(instr_list, fs);
  auto src_list = new temp::TempList();
  auto dst_list = new temp::TempList();
  auto result=temp::TempFactory::NewTemp();
  temp::Temp *rax_save=nullptr, *rdx_save=nullptr;

  if(this->op_==tree::BinOp::DIV_OP || this->op_==tree::BinOp::MUL_OP)
  {
    rax_save=temp::TempFactory::NewTemp();
    rdx_save=temp::TempFactory::NewTemp();

    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
    new temp::TempList{rax_save},new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RAX)}));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
    new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RAX)},new temp::TempList{left}));
    instr_list.Append((new assem::MoveInstr("movq `s0, `d0",
    new temp::TempList{rdx_save},new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RDX)})));
  }
  else
  {
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      new temp::TempList{result},new temp::TempList{left}));
  }

  switch (this->op_) {
  case tree::BinOp::PLUS_OP:
    instr_builder << "addq `s0, `d0";
    src_list->Append(right);
    dst_list->Append(result);
    break;
  case tree::BinOp::MINUS_OP:
    instr_builder << "subq `s0, `d0";
    src_list->Append(right);
    dst_list->Append(result);
    break;
  case tree::BinOp::MUL_OP:
    instr_builder << "imulq `s0";
    src_list->Append(right);
    break;
  case tree::BinOp::DIV_OP:
    instr_list.Append(new assem::OperInstr("cqto", nullptr, nullptr,nullptr));
    instr_builder << "idivq `s0";
    src_list->Append(right);
    break;
  default:
    assert(false);
    break;
  }
  instr_list.Append(new assem::OperInstr(instr_builder.str(), dst_list, src_list,
                                         nullptr));
  if(this->op_==tree::BinOp::DIV_OP || this->op_==tree::BinOp::MUL_OP)
  {
    dst_list->Append(result);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      dst_list,new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RAX)}));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
    new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RDX)},new temp::TempList{rdx_save}));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RAX)},new temp::TempList{rax_save}));
  }
  return result;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto result=temp::TempFactory::NewTemp();
  auto mem_addr=this->exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr("movq (`s0), `d0",
    new temp::TempList{result},new temp::TempList{mem_addr}));
  return result;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if(this->temp_==reg_manager->FramePointer()){
    auto fp=temp::TempFactory::NewTemp();
    std::ostringstream instr_builder;
    instr_builder << "leaq "<<fs<<"(`s0), `d0";
    instr_list.Append(new assem::MoveInstr(instr_builder.str(),
                                           new temp::TempList{fp},
                                           new temp::TempList{reg_manager->StackPointer()}));
    return fp;
  }
  return this->temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  this->stm_->Munch(instr_list, fs);
  return this->exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream instr_builder;
  auto tmp = temp::TempFactory::NewTemp();
  instr_builder << "leaq " << this->name_->Name() << "(%rip), `d0";
  instr_list.Append(new assem::MoveInstr(instr_builder.str(),
                                         new temp::TempList{tmp}, nullptr));
  return tmp;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream instr_builder;
  instr_builder << "movq $" << this->consti_ << ", `d0";
  auto tmp = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr(instr_builder.str(),
                                         new temp::TempList{tmp}, nullptr));
  return tmp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto param_list=this->args_->MunchArgs(instr_list, fs);
  std::ostringstream instr_builder_call;
  instr_builder_call<<"callq "<<
    static_cast<NameExp *>(this->fun_)->name_->Name();
  instr_list.Append(new assem::OperInstr(instr_builder_call.str(),nullptr,param_list,nullptr));
  auto ret_val=temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                         new temp::TempList{ret_val},
                                         new temp::TempList{reg_manager->ReturnValue()}));
  if(this->args_->GetList().size()<=reg_manager->ArgRegs()->GetList().size())
  {
    int stack_size=(this->args_->GetList().size()-reg_manager->ArgRegs()->GetList().size())
                    *reg_manager->WordSize();
    std::ostringstream instr_builder_clear;
    instr_builder_call<<"addq $"<<stack_size<<", `d0";
    instr_list.Append(new assem::OperInstr(instr_builder_clear.str(),
                                           new temp::TempList{reg_manager->StackPointer()},
                                           nullptr,nullptr));
  }
  return ret_val;
}

} // namespace tree

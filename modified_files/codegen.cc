#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

}

namespace cg {

void CodeGen::Codegen() {
  std::string framesize_label = this->frame_->GetLabel() + "_framesize";
  auto instruction_list = new assem::InstrList();

  for (auto statement : this->traces_->GetStmList()->GetList()) {
    statement->Munch(*instruction_list, framesize_label);
  }

  this->assem_instr_ =
      std::make_unique<AssemInstr>(frame::ProcEntryExit2(instruction_list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}

}

namespace tree {

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  auto temp_results = new temp::TempList();
  int total_args = this->exp_list_.size();
  int arg_index = 0;
  auto current_arg = this->exp_list_.begin();

  for (auto argument_reg : reg_manager->ArgRegs()->GetList()) {
    if (current_arg == this->exp_list_.end())
      break;
    auto tmp = (*current_arg)->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                            new temp::TempList{argument_reg},
                                            new temp::TempList{tmp}));
    temp_results->Append(tmp);
    ++current_arg;
    ++arg_index;
  }

  auto frame_ptr_exp = new TempExp(reg_manager->FramePointer());
  while (current_arg != this->exp_list_.end()) {
    auto tmp = (*current_arg)->Munch(instr_list, fs);
    std::ostringstream asm_instr;
    asm_instr << "movq `s0, -"
              << (total_args - arg_index) * reg_manager->WordSize() << "(`s1)";
    instr_list.Append(new assem::MoveInstr(
        asm_instr.str(), nullptr,
        new temp::TempList{tmp, frame_ptr_exp->Munch(instr_list, fs)}));
    temp_results->Append(tmp);
    ++current_arg;
    ++arg_index;
  }

  return temp_results;
}

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  instr_list.Append(
      new assem::LabelInstr(this->label_->Name(), this->label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream asm_instr;
  asm_instr << "jmp " << this->jumps_->front()->Name();
  instr_list.Append(new assem::OperInstr(asm_instr.str(), nullptr, nullptr,
                                         new assem::Targets(this->jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream cmp_instr, jump_instr;

  if (typeid(*(this->left_)) == typeid(tree::ConstExp)) {
    cmp_instr << "cmpq `s0, $"
              << static_cast<tree::ConstExp *>(this->left_)->consti_;
    auto right_operand = this->right_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(cmp_instr.str(), nullptr,
                                           new temp::TempList{right_operand},
                                           nullptr));
  } else {
    cmp_instr << "cmpq `s1, `s0";
    auto left_operand = this->left_->Munch(instr_list, fs);
    auto right_operand = this->right_->Munch(instr_list, fs);
    instr_list.Append(new assem::OperInstr(
        cmp_instr.str(), nullptr,
        new temp::TempList{left_operand, right_operand}, nullptr));
  }

  switch (this->op_) {
  case tree::RelOp::EQ_OP:
    jump_instr << "je ";
    break;
  case tree::RelOp::NE_OP:
    jump_instr << "jne ";
    break;
  case tree::RelOp::LT_OP:
    jump_instr << "jl ";
    break;
  case tree::RelOp::LE_OP:
    jump_instr << "jle ";
    break;
  case tree::RelOp::GT_OP:
    jump_instr << "jg ";
    break;
  case tree::RelOp::GE_OP:
    jump_instr << "jge ";
    break;
  default:
    assert(false);
    break;
  }

  jump_instr << " `j0";
  instr_list.Append(
      new assem::OperInstr(jump_instr.str(), nullptr, nullptr,
                           new assem::Targets(new std::vector<temp::Label *>{
                               this->true_label_, this->false_label_})));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream asm_instr;
  auto destination_list = new temp::TempList();
  auto source_list = new temp::TempList();
  int source_idx = 0;

  asm_instr << "movq ";

  if (typeid(*(this->src_)) == typeid(tree::ConstExp)) {
    asm_instr << "$"
              << static_cast<tree::ConstExp *>(this->src_)->consti_;
  } else {
    asm_instr << "`s" << source_idx++;
    source_list->Append(this->src_->Munch(instr_list, fs));
  }

  asm_instr << ", ";

  if (typeid(*(this->dst_)) == typeid(tree::MemExp)) {
    auto memory_addr =
        static_cast<tree::MemExp *>(this->dst_)->exp_->Munch(instr_list, fs);
    asm_instr << "(`s" << source_idx++ << ")";
    source_list->Append(memory_addr);
  } else {
    auto dst_temp = this->dst_->Munch(instr_list, fs);
    asm_instr << "`d0";
    destination_list->Append(dst_temp);
  }

  instr_list.Append(
      new assem::MoveInstr(asm_instr.str(), destination_list, source_list));
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  this->exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list,
                            std::string_view fs) {
  std::ostringstream asm_instr;
  auto left_op = this->left_->Munch(instr_list, fs);
  auto right_op = this->right_->Munch(instr_list, fs);
  auto source_list = new temp::TempList();
  auto destination_list = new temp::TempList();
  auto output = temp::TempFactory::NewTemp();
  temp::Temp *saved_rax = nullptr, *saved_rdx = nullptr;

  if (this->op_ == tree::BinOp::DIV_OP || this->op_ == tree::BinOp::MUL_OP) {
    saved_rax = temp::TempFactory::NewTemp();
    saved_rdx = temp::TempFactory::NewTemp();

    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList{saved_rax},
        new temp::TempList{reg_manager->GetRegister(
            frame::X64RegManager::Reg::RAX)}));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList{reg_manager->GetRegister(
            frame::X64RegManager::Reg::RAX)},
        new temp::TempList{left_op}));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList{saved_rdx},
        new temp::TempList{reg_manager->GetRegister(
            frame::X64RegManager::Reg::RDX)}));
  } else {
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList{output},
        new temp::TempList{left_op}));
  }

  switch (this->op_) {
  case tree::BinOp::PLUS_OP:
    asm_instr << "addq `s0, `d0";
    source_list->Append(right_op);
    destination_list->Append(output);
    break;
  case tree::BinOp::MINUS_OP:
    asm_instr << "subq `s0, `d0";
    source_list->Append(right_op);
    destination_list->Append(output);
    break;
  case tree::BinOp::MUL_OP:
    asm_instr << "imulq `s0";
    source_list->Append(right_op);
    break;
  case tree::BinOp::DIV_OP:
    instr_list.Append(
        new assem::OperInstr("cqto", nullptr, nullptr, nullptr));
    asm_instr << "idivq `s0";
    source_list->Append(right_op);
    break;
  default:
    assert(false);
    break;
  }

  instr_list.Append(new assem::OperInstr(asm_instr.str(), destination_list,
                                         source_list, nullptr));

  if (this->op_ == tree::BinOp::DIV_OP || this->op_ == tree::BinOp::MUL_OP) {
    destination_list->Append(output);
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", destination_list,
        new temp::TempList{reg_manager->GetRegister(
            frame::X64RegManager::Reg::RAX)}));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList{reg_manager->GetRegister(
            frame::X64RegManager::Reg::RDX)},
        new temp::TempList{saved_rdx}));
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0",
        new temp::TempList{reg_manager->GetRegister(
            frame::X64RegManager::Reg::RAX)},
        new temp::TempList{saved_rax}));
  }

  return output;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto output = temp::TempFactory::NewTemp();
  auto address = this->exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr("movq (`s0), `d0",
                                         new temp::TempList{output},
                                         new temp::TempList{address}));
  return output;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  if (this->temp_ == reg_manager->FramePointer()) {
    auto frame_ptr = temp::TempFactory::NewTemp();
    std::ostringstream asm_instr;
    asm_instr << "leaq " << fs << "(`s0), `d0";
    instr_list.Append(
        new assem::MoveInstr(asm_instr.str(), new temp::TempList{frame_ptr},
                             new temp::TempList{reg_manager->StackPointer()}));
    return frame_ptr;
  }
  return this->temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  this->stm_->Munch(instr_list, fs);
  return this->exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  std::ostringstream asm_instr;
  auto output = temp::TempFactory::NewTemp();
  asm_instr << "leaq " << this->name_->Name() << "(%rip), `d0";
  instr_list.Append(
      new assem::MoveInstr(asm_instr.str(), new temp::TempList{output}, nullptr));
  return output;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list,
                            std::string_view fs) {
  std::ostringstream asm_instr;
  asm_instr << "movq $" << this->consti_ << ", `d0";
  auto output = temp::TempFactory::NewTemp();
  instr_list.Append(
      new assem::MoveInstr(asm_instr.str(), new temp::TempList{output}, nullptr));
  return output;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  auto argument_temps = this->args_->MunchArgs(instr_list, fs);

  std::ostringstream call_instr;
  call_instr << "callq "
             << static_cast<NameExp *>(this->fun_)->name_->Name();
  instr_list.Append(new assem::OperInstr(call_instr.str(), nullptr,
                                         argument_temps, nullptr));

  auto return_temp = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr(
      "movq `s0, `d0", new temp::TempList{return_temp},
      new temp::TempList{reg_manager->ReturnValue()}));

  if (this->args_->GetList().size() <=
      reg_manager->ArgRegs()->GetList().size()) {
    int stack_bytes =
        (this->args_->GetList().size() -
         reg_manager->ArgRegs()->GetList().size()) *
        reg_manager->WordSize();
    std::ostringstream cleanup_instr;
    cleanup_instr << "addq $" << stack_bytes << ", `d0";
    instr_list.Append(new assem::OperInstr(
        cleanup_instr.str(), new temp::TempList{reg_manager->StackPointer()},
        nullptr, nullptr));
  }

  return return_temp;
}

}

#include "tiger/frame/x64frame.h"

#include <sstream>

extern frame::RegManager *reg_manager;

namespace frame {

X64RegManager::X64RegManager() : RegManager() {
  for (unsigned int idx = 0; idx < REG_COUNT; idx++)
    regs_.push_back(temp::TempFactory::NewTemp());

  std::array<std::string_view, REG_COUNT> register_names{
      "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
      "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
  int register_index = RAX;
  for (auto &reg_name : register_names) {
    temp_map_->Enter(regs_[register_index], new std::string(reg_name));
    register_index++;
  }
}

temp::TempList *X64RegManager::Registers() {
  const std::array register_array{
      RAX, RBX, RCX, RDX, RSI, RDI, RBP, R8, R9, R10, R11, R12, R13, R14, R15,
  };
  auto *register_list = new temp::TempList();
  for (auto &reg_id : register_array)
    register_list->Append(regs_[reg_id]);
  return register_list;
}

temp::TempList *X64RegManager::ArgRegs() {
  const std::array argument_regs{RDI, RSI, RDX, RCX, R8, R9};
  auto *register_list = new temp::TempList();
  for (auto &reg_id : argument_regs)
    register_list->Append(regs_[reg_id]);
  return register_list;
}

temp::TempList *X64RegManager::CallerSaves() {
  std::array caller_saved_regs{RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11};
  auto *register_list = new temp::TempList();
  for (auto &reg_id : caller_saved_regs)
    register_list->Append(regs_[reg_id]);
  return register_list;
}

temp::TempList *X64RegManager::CalleeSaves() {
  std::array callee_saved_regs{RBP, RBX, R12, R13, R14, R15};
  auto *register_list = new temp::TempList();
  for (auto &reg_id : callee_saved_regs)
    register_list->Append(regs_[reg_id]);
  return register_list;
}

temp::TempList *X64RegManager::ReturnSink() {
  temp::TempList *register_list = CalleeSaves();
  register_list->Append(regs_[SP]);
  register_list->Append(regs_[RV]);
  return register_list;
}

int X64RegManager::WordSize() { return 8; }

temp::Temp *X64RegManager::FramePointer() { return regs_[FP]; }

temp::Temp *X64RegManager::StackPointer() { return regs_[SP]; }

temp::Temp *X64RegManager::ReturnValue() { return regs_[RV]; }

class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}

  tree::Exp *ToExp(tree::Exp *frame_ptr) const override {
    return new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, frame_ptr,
                                               new tree::ConstExp(this->offset)));
  }
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}

  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
public:
  tree::Stm *view_shift;

  X64Frame(temp::Label *name, std::list<frame::Access *> *formals)
      : Frame(0, 0, name, formals), view_shift(nullptr) {}

  [[nodiscard]] std::string GetLabel() const override { return name_->Name(); }
  [[nodiscard]] temp::Label *Name() const override { return name_; }

  [[nodiscard]] int Size() {
    return reg_manager->WordSize() *
           (this->local_count_ + this->outgo_count);
  }

  [[nodiscard]] std::list<frame::Access *> *Formals() const override {
    return formals_;
  }

  frame::Access *AllocLocal(bool escape) override {
    if (escape) {
      return new InFrameAccess(
          -reg_manager->WordSize() * (++(this->local_count_)));
    } else {
      return new InRegAccess(temp::TempFactory::NewTemp());
    }
  }

  void AllocOutgoSpace(int size) override {
    if (this->outgo_count < size)
      this->outgo_count = size;
  }

  tree::Exp *ExternalCall(std::string_view s, tree::ExpList *args) override;
};

frame::Frame *NewFrame(temp::Label *name, std::list<bool> formals) {
  std::list<frame::Access *> *formal_accesses =
      new std::list<frame::Access *>{};
  int byte_offset = 0;
  int word_size = reg_manager->WordSize();
  X64Frame *new_frame = new X64Frame(name, formal_accesses);

  for (auto formal_iter = formals.begin(); formal_iter != formals.end();
       formal_iter++) {
    Access *formal_access = new_frame->AllocLocal(*formal_iter);
    formal_accesses->push_back(formal_access);
  }

  int processed_args = 0, stack_arg_count = 0, total_formals = formals.size();
  int max_register_args = reg_manager->ArgRegs()->GetList().size();
  stack_arg_count = total_formals - max_register_args;

  for (auto formal_access : *(new_frame->Formals())) {
    tree::MoveStm *param_move = nullptr;

    if (processed_args < max_register_args) {
      param_move = new tree::MoveStm(
          formal_access->ToExp(
              new tree::TempExp(reg_manager->FramePointer())),
          new tree::TempExp(
              reg_manager->ArgRegs()->NthTemp(processed_args)));
      ++processed_args;
    } else {
      param_move = new tree::MoveStm(
          formal_access->ToExp(
              new tree::TempExp(reg_manager->FramePointer())),
          new tree::MemExp(new tree::BinopExp(
              tree::BinOp::PLUS_OP,
              new tree::TempExp(reg_manager->FramePointer()),
              new tree::ConstExp(reg_manager->WordSize() *
                                 (processed_args - max_register_args + 1)))));
      ++processed_args;
    }

    if (new_frame->view_shift == nullptr) {
      new_frame->view_shift = param_move;
    } else {
      new_frame->view_shift =
          new tree::SeqStm(new_frame->view_shift, param_move);
    }
  }

  return new_frame;
}

tree::Exp *X64Frame::ExternalCall(std::string_view s, tree::ExpList *args) {
  return new tree::CallExp(
      new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
}

tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  auto current_frame = dynamic_cast<frame::X64Frame *>(frame);
  assert(current_frame);

  auto saved_callee_exps = new tree::ExpList();

  tree::Stm *save_sequence = nullptr;
  temp::TempList *callee_save_regs = reg_manager->CalleeSaves();
  for (auto callee_reg : callee_save_regs->GetList()) {
    temp::Temp *backup_temp = temp::TempFactory::NewTemp();
    if (!save_sequence)
      save_sequence = new tree::MoveStm(new tree::TempExp(backup_temp),
                                        new tree::TempExp(callee_reg));
    else
      save_sequence = new tree::SeqStm(
          save_sequence, new tree::MoveStm(new tree::TempExp(backup_temp),
                                           new tree::TempExp(callee_reg)));
    saved_callee_exps->Append(new tree::TempExp(backup_temp));
  }

  tree::Stm *restore_sequence = nullptr;
  callee_save_regs = reg_manager->CalleeSaves();
  auto saved_exp_iter = saved_callee_exps->GetList().begin();
  for (auto callee_reg : callee_save_regs->GetList()) {
    assert(saved_exp_iter != saved_callee_exps->GetList().end());
    if (!restore_sequence)
      restore_sequence =
          new tree::MoveStm(new tree::TempExp(callee_reg), *saved_exp_iter++);
    else
      restore_sequence = new tree::SeqStm(
          restore_sequence,
          new tree::MoveStm(new tree::TempExp(callee_reg), *saved_exp_iter++));
  }

  tree::Stm *final_stm;
  if (current_frame->view_shift == nullptr) {
    final_stm =
        new tree::SeqStm(save_sequence, new tree::SeqStm(stm, restore_sequence));
  } else
    final_stm = new tree::SeqStm(
        save_sequence, new tree::SeqStm(current_frame->view_shift,
                                        new tree::SeqStm(stm, restore_sequence)));
  return final_stm;
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(
      new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr));
  return body;
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  auto current_x64frame = dynamic_cast<frame::X64Frame *>(frame);
  assert(current_x64frame);

  std::ostringstream prologue_stream;
  std::ostringstream epilogue_stream;

  prologue_stream << ".set " << current_x64frame->Name()->Name()
                  << "_framesize, " << current_x64frame->Size() << "\n"
                  << current_x64frame->Name()->Name() << ":\n"
                  << "subq $" << current_x64frame->Size() << ", %rsp\n";

  epilogue_stream << "addq $" << current_x64frame->Size() << ", %rsp\n"
                  << "retq\n"
                  << ".END\n";

  return new assem::Proc(prologue_stream.str(), body, epilogue_stream.str());
}

}

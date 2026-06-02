#include "tiger/frame/x64frame.h"

#include <sstream>

extern frame::RegManager *reg_manager;

namespace frame {

X64RegManager::X64RegManager() : RegManager() {
  for (unsigned int i = 0; i < REG_COUNT; i++)
    regs_.push_back(temp::TempFactory::NewTemp());

  std::array<std::string_view, REG_COUNT> reg_name{
      "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
      "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"};
  int reg = RAX;
  for (auto &name : reg_name) {
    temp_map_->Enter(regs_[reg], new std::string(name));
    reg++;
  }
}

temp::TempList *X64RegManager::Registers() {
  const std::array reg_array{
      RAX, RBX, RCX, RDX, RSI, RDI, RBP, R8, R9, R10, R11, R12, R13, R14, R15,
  };
  auto *temp_list = new temp::TempList();
  for (auto &reg : reg_array)
    temp_list->Append(regs_[reg]);
  return temp_list;
}

temp::TempList *X64RegManager::ArgRegs() {
  const std::array reg_array{RDI, RSI, RDX, RCX, R8, R9};
  auto *temp_list = new temp::TempList();
  ;
  for (auto &reg : reg_array)
    temp_list->Append(regs_[reg]);
  return temp_list;
}

temp::TempList *X64RegManager::CallerSaves() {
  std::array reg_array{RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11};
  auto *temp_list = new temp::TempList();
  ;
  for (auto &reg : reg_array)
    temp_list->Append(regs_[reg]);
  return temp_list;
}

temp::TempList *X64RegManager::CalleeSaves() {
  std::array reg_array{RBP, RBX, R12, R13, R14, R15};
  auto *temp_list = new temp::TempList();
  ;
  for (auto &reg : reg_array)
    temp_list->Append(regs_[reg]);
  return temp_list;
}

temp::TempList *X64RegManager::ReturnSink() {
  temp::TempList *temp_list = CalleeSaves();
  temp_list->Append(regs_[SP]);
  temp_list->Append(regs_[RV]);
  return temp_list;
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
    return new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP,
        frame_ptr, new tree::ConstExp(this->offset)));
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
      : Frame(0, 0, name, formals), view_shift(nullptr){}

  [[nodiscard]] std::string GetLabel() const override { return name_->Name(); }
  [[nodiscard]] temp::Label *Name() const override { return name_; }

  [[nodiscard]] int Size(){return reg_manager->WordSize()*(this->local_count_+this->outgo_count);  }
  [[nodiscard]] std::list<frame::Access *> *Formals() const override {
    return formals_;
  }
  frame::Access *AllocLocal(bool escape) override {
    if (escape) {
      return new InFrameAccess(-reg_manager->WordSize() * (++this->local_count_));
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
  std::list<frame::Access *> *params = new std::list<frame::Access *>{};
  int offset = 0;
  int wordsize = reg_manager->WordSize();
  X64Frame *frame = new X64Frame(name, params);
  for (auto formal_it = formals.begin(); formal_it != formals.end(); formal_it++) {
    Access *param = frame->AllocLocal(*formal_it);
    params->push_back(param);
  }
  int arg_count = 0, mem_count = 0, formals_count = formals.size();
  int max_arg_count = reg_manager->ArgRegs()->GetList().size();
  mem_count = formals_count - max_arg_count;
  for (auto formal : *(frame->Formals())) {
    tree::MoveStm *move_stm = nullptr;
    if (arg_count < max_arg_count) {
      move_stm = new tree::MoveStm(
        formal->ToExp(new tree::TempExp(reg_manager->FramePointer())),
        new tree::TempExp(reg_manager->ArgRegs()->NthTemp(arg_count))
      );
      ++arg_count;
    } else {
      move_stm = new tree::MoveStm(
        formal->ToExp(new tree::TempExp(reg_manager->FramePointer())),
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP,
            new tree::TempExp(reg_manager->FramePointer()),
            new tree::ConstExp(reg_manager->WordSize() * (arg_count - max_arg_count + 1))
          )
        )
      );
      ++arg_count;
    }
    if (frame->view_shift == nullptr) {
      frame->view_shift = move_stm;
    } else {
      frame->view_shift = new tree::SeqStm(
        frame->view_shift,
        move_stm
      );
    }
  }

  return frame;
}

tree::Exp *X64Frame::ExternalCall(std::string_view s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}

tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm) {
  auto x64_frame = dynamic_cast<frame::X64Frame *>(frame);
  assert(x64_frame);

  auto callee_list = new tree::ExpList();

  tree::Stm *save_stm = nullptr;
  temp::TempList *callees = reg_manager->CalleeSaves();
  for (auto callee : callees->GetList()) {
    temp::Temp *r = temp::TempFactory::NewTemp();
    if (!save_stm)
      save_stm =
          new tree::MoveStm(new tree::TempExp(r), new tree::TempExp(callee));
    else
      save_stm = new tree::SeqStm(
          save_stm,
          new tree::MoveStm(new tree::TempExp(r), new tree::TempExp(callee)));
    callee_list->Append(new tree::TempExp(r));
  }

  tree::Stm *restore_stm = nullptr;
  callees = reg_manager->CalleeSaves();
  auto callee_it = callee_list->GetList().begin();
  for (auto callee : callees->GetList()) {
    assert(callee_it != callee_list->GetList().end());
    if (!restore_stm)
      restore_stm = new tree::MoveStm(new tree::TempExp(callee), *callee_it++);
    else
      restore_stm = new tree::SeqStm(
          restore_stm,
          new tree::MoveStm(new tree::TempExp(callee), *callee_it++));
  }

  tree::Stm *exit_stm;
  if (x64_frame->view_shift == nullptr) {
    exit_stm = new tree::SeqStm(save_stm, new tree::SeqStm(stm, restore_stm));
  } else
    exit_stm = new tree::SeqStm(
        save_stm, new tree::SeqStm(x64_frame->view_shift,
                                   new tree::SeqStm(stm, restore_stm)));
  return exit_stm;
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  body->Append(new assem::OperInstr("", nullptr, reg_manager->ReturnSink(),
                                    nullptr));
  return body;
}

assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body) {
  auto x64frame = dynamic_cast<frame::X64Frame *>(frame);
  assert(x64frame);

  std::ostringstream prolog_builder;
  std::ostringstream epilog_builder;

  prolog_builder << ".set " << x64frame->Name()->Name() << "_framesize, "
                 << x64frame->Size() << "\n"
                 << x64frame->Name()->Name() << ":\n"
                 << "subq $" << x64frame->Size() << ", %rsp\n";

  epilog_builder << "addq $" << x64frame->Size() << ", %rsp\n"
                 << "retq\n"
                 << ".END\n";

  return new assem::Proc(prolog_builder.str(), body, epilog_builder.str());
}

} // namespace frame

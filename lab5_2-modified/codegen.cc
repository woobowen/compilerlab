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
  /* TODO: Put your lab5 code here */
  std::string fs_=this->frame_->GetLabel() + "_framesize";
  auto list=new assem::InstrList();

  for(auto stmt:this->traces_->GetStmList()->GetList())
  {
    stmt->Munch(*list, fs_);
  }

  this->assem_instr_=
    std::make_unique<AssemInstr>(frame::ProcEntryExit2(list));
  /* End for lab5 code */
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {

/* TODO: Put your lab5 code here */
/**
 * Generate code for passing arguments
 * @param args argument list
 * @param instr_holder instr holder
 * @return temp list to hold arguments
 */
temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  auto results=new temp::TempList();
  int num_args=this->exp_list_.size();
  int i=0;
  auto arg_it=this->exp_list_.begin();
  for(auto reg:reg_manager->ArgRegs()->GetList())
  {
    if(arg_it==this->exp_list_.end())
      break;
    auto temp=(*arg_it)->Munch(instr_list,fs);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      new temp::TempList{reg},new temp::TempList{temp}));
    results->Append(temp);
    ++arg_it;
    ++i;
  }
  auto fp_exp=new TempExp(reg_manager->FramePointer());
  while(arg_it!=this->exp_list_.end())
  {
    auto temp=(*arg_it)->Munch(instr_list,fs);
    std::ostringstream instr_builder;
    instr_builder<<"movq `s0, -"<<(num_args-i)*reg_manager->WordSize()<<"(`s1)";
    instr_list.Append(new assem::MoveInstr(instr_builder.str(),
      nullptr,new temp::TempList{temp,fp_exp->Munch(instr_list,fs)}));
    results->Append(temp);
    ++arg_it;
    ++i;
  }
  return results;
  /* End for lab5 code */
}

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // SeqStm should not exist in codegen phase
  this->left_->Munch(instr_list, fs);
  this->right_->Munch(instr_list, fs);
  /* End for lab5 code */
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(
      this->label_->Name(), this->label_));
  /* End for lab5 code */
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::ostringstream instr_builder;
  instr_builder << "jmp " << this->jumps_->front()->Name();
  instr_list.Append(new assem::OperInstr(instr_builder.str(), nullptr, nullptr,
                                         new assem::Targets(this->jumps_)));
  /* End for lab5 code */
}

// 目标操作数不可以是立即数
// cmp指令也有目标操作数和源操作数的概念
// 因此我们只可以有cmp $42,rax
// 而不能有cmp rax,$42
//
// 附：deepseek的说明
// x86 的算术逻辑单元（ALU）在执行减法时，需要明确 被减数 和 减数 的输入端口：
// 目标操作数（被减数）必须来自寄存器或内存（可寻址的位置）。
// 源操作数（减数）可以来自立即数（直接嵌入指令流中）。
// 如果允许目标操作数为立即数，硬件需要额外支持将立即数直接输入到 ALU
// 的被减数端口，这会增加电路设计的复杂度。
void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
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
  /* End for lab5 code */
}

// 需要注意目的位置为内存位置的情况，这种情况需要单独处理，而不是递归调用MemExp
void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
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
    auto res=this->dst_->Munch(instr_list, fs);
    instr_builder<<"`d0";
    dst_list->Append(res);
  }
  instr_builder;
  instr_list.Append(new assem::MoveInstr(instr_builder.str(), dst_list, src_list));

  /* End for lab5 code */
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  this->exp_->Munch(instr_list, fs);
  /* End for lab5 code */
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::ostringstream instr_builder;
  auto left = this->left_->Munch(instr_list, fs);
  auto right = this->right_->Munch(instr_list, fs);
  auto src_list = new temp::TempList();
  auto dst_list = new temp::TempList();
  auto result=temp::TempFactory::NewTemp();
  temp::Temp *rax_save=nullptr, *rdx_save=nullptr;

  // 我们需要：
  // 保存RAX
  // 保存RDX
  // 将dst存入RAX
  // RAX，dst恢复（位于函数结尾处操作）
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
    // 对于加减操作
    // 我们首先需要使用movq将某一个操作数移动到结果寄存器之中，这个寄存器可以不是rax
    // 我们默认移动左边的
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      new temp::TempList{result},new temp::TempList{left}));
  }

  switch (this->op_) {
  case tree::BinOp::PLUS_OP:
    instr_builder << "addq `s0, `d0";
    src_list->Append(right);
    dst_list->Append(res);
    break;
  case tree::BinOp::MINUS_OP:
    instr_builder << "subq `s0, `d0";
    src_list->Append(right);
    dst_list->Append(res);
    break;
  case tree::BinOp::MUL_OP:
    // In Intel Manual: RDX:RAX := RAX * r/m64
    // 将dst的值移入rax
    instr_builder << "imulq `s0";
    src_list->Append(right);
    break;
  // In Intel Manual: RDX:RAX := RAX / r/m64
  case tree::BinOp::DIV_OP:
    // 进行符号扩展
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
  // todo:对imulq和idivq需要有数据传送指令将rax的值写回result中，以及恢复保存的rax和rdx
  if(this->op_==tree::BinOp::DIV_OP || this->op_==tree::BinOp::MUL_OP)
  {
    dst_list->Append(res);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      dst_list,new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RAX)}));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
    new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RDX)},new temp::TempList{rdx_save}));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
      new temp::TempList{reg_manager->GetRegister(frame::X64RegManager::Reg::RAX)},new temp::TempList{rax_save}));
  }
  return res;
  /* End for lab5 code */
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // auto dst = temp::TempFactory::NewTemp();
  auto result=temp::TempFactory::NewTemp();
  auto mem_addr=this->exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::MoveInstr("movq (`s0), `d0",
    new temp::TempList{result},new temp::TempList{mem_addr}));
  // instr_builder << "movq ";
  // ConstPos pos=CheckReducible(this->exp_);
  // auto result=temp::TempFactory::NewTemp();
  // auto *src_list=new temp::TempList();
  // auto *dst_list=new temp::TempList({result});
  // if(pos==ConstPos::NA)
  // {
  //   instr_builder << "(`s0)";
  //   src_list->Append(this->exp_->Munch(instr_list, fs));
  // }
  // else if(pos==ConstPos::NONE)
  // {
  //   instr_builder<<"(`s0,s1)";
  //   auto binopexp=static_cast<tree::BinopExp *>(this->exp_);
  //   src_list->Append(binopexp->left_->Munch(instr_list, fs));
  //   src_list->Append(binopexp->right_->Munch(instr_list, fs));
  // }
  // else
  // {
  //   auto arithexp=static_cast<tree::BinopExp *>(this->exp_);
  //   tree::ConstExp *constexp=nullptr;
  //   if(pos==ConstPos::LEFT)
  //   {
  //     constexp=static_cast<tree::ConstExp *>(arithexp->left_);
  //     src_list->Append(arithexp->right_->Munch(instr_list, fs));
  //   }
  //   else
  //   {
  //     constexp=static_cast<tree::ConstExp *>(arithexp->right_);
  //     src_list->Append(arithexp->left_->Munch(instr_list, fs));
  //   }

  //   int offset=constexp->consti_;
  //   // 如果是减号，那么对const取反
  //   if(arithexp->op_==tree::BinOp::MINUS_OP)
  //   {
  //     offset=-offset;
  //   }
  //   instr_builder<<constexp->consti_<<"(`s0)";
  // }
  // instr_builder << ", `d0\n";
  // instr_list.Append(new assem::MoveInstr(instr_builder.str(),dst_list,src_list));
  return res;
  /* End for lab5 code */
}

// 特殊情况：由于fp是一个“虚拟的”寄存器
// 因此当当前临时变量为fp时
// 我们需要生成一个新的临时变量
temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // FP
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
  /* End for lab5 code */
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // EseqExp should not exist in codegen phase
  this->stm_->Munch(instr_list, fs);
  return this->exp_->Munch(instr_list, fs);
  /* End for lab5 code */
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::ostringstream instr_builder;
  auto tmp = temp::TempFactory::NewTemp();
  instr_builder << "leaq " << this->name_->Name() << "(%rip), `d0";
  instr_list.Append(new assem::MoveInstr(instr_builder.str(),
                                         new temp::TempList{tmp}, nullptr));
  return tmp;
  /* End for lab5 code */
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::ostringstream instr_builder;
  // MOVE dst, const interger
  instr_builder << "movq $" << this->consti_ << ", `d0";
  auto tmp = temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr(instr_builder.str(),
                                         new temp::TempList{tmp}, nullptr));
  return tmp;
  /* End for lab5 code */
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto param_list=this->args_->MunchArgs(instr_list, fs);
  // 调用指令（callq）
  std::ostringstream instr_builder_call;
  instr_builder_call<<"callq "<<
    static_cast<NameExp *>(this->fun_)->name_->Name();
  instr_list.Append(new assem::OperInstr(instr_builder_call.str(),nullptr,param_list,nullptr));
  // 返回值获取
  auto ret_val=temp::TempFactory::NewTemp();
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0",
                                         new temp::TempList{ret_val},
                                         new temp::TempList{reg_manager->ReturnValue()}));
  // 清栈
  // 如果没有使用栈空间，则不需要执行
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
  /* End for lab5 code */
}

} // namespace tree

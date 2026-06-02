#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"


namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  virtual tree::Exp *ToExp(tree::Exp *frame_ptr) const = 0;
  virtual ~Access() = default;
};

class Frame {
protected:
  int outgo_count;
  int local_count_;
  std::list<frame::Access *> *formals_;
public:
  temp::Label *name_;
  Frame(int frame_size, int local_count, temp::Label *name, std::list<frame::Access *> *formals)
      : outgo_count(frame_size), local_count_(local_count), name_(name), formals_(formals) {}

  virtual std::string GetLabel() const = 0;
  virtual temp::Label *Name() const = 0;
  virtual std::list<frame::Access *> *Formals() const = 0;
  virtual frame::Access *AllocLocal(bool escape) = 0;
  virtual void AllocOutgoSpace(int size)=0;
  virtual tree::Exp *ExternalCall(std::string_view s, tree::ExpList *args)=0;
  virtual ~Frame() = default;
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

frame::Frame *NewFrame(temp::Label *name, std::list<bool> formals);
tree::Stm *ProcEntryExit1(frame::Frame *frame, tree::Stm *stm);
assem::InstrList *ProcEntryExit2(assem::InstrList *body);
assem::Proc *ProcEntryExit3(frame::Frame *frame, assem::InstrList *body);

} // namespace frame

#endif

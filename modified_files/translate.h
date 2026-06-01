#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

namespace tr {

class Exp;
class ExpAndTy;
class Level;

class PatchList {
public:
  void DoPatch(temp::Label *label) const {
    for (auto &label_patch : patch_list_)
      *label_patch = label;
  }

  static PatchList JoinPatch(const PatchList &first, const PatchList &second) {
    PatchList combined(first.GetList());
    for (auto &label_patch : second.patch_list_) {
      combined.patch_list_.push_back(label_patch);
    }
    return combined;
  }

  explicit PatchList(std::list<temp::Label **> patch_list)
      : patch_list_(patch_list) {}
  PatchList() = default;

  [[nodiscard]] const std::list<temp::Label **> &GetList() const {
    return patch_list_;
  }

private:
  std::list<temp::Label **> patch_list_;
};

class Access {
public:
  Level *level_;
  frame::Access *access_;

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}
  static Access *AllocLocal(Level *level, bool escape);
};

class Level {
public:
  frame::Frame *frame_;
  Level *parent_;

  Level(frame::Frame *frame, Level *parent) : frame_(frame), parent_(parent) {}

  std::list<tr::Access *> *Formals() {
    auto frame_access_list = frame_->Formals();
    auto translate_access_list = new std::list<tr::Access *>();

    for (auto frame_acc : *frame_access_list)
      translate_access_list->push_back(new tr::Access(this, frame_acc));

    return translate_access_list;
  }

  static Level *NewLevel(Level *parent, temp::Label *name,
                         std::list<bool> formals) {
    return new Level(frame::NewFrame(name, formals), parent);
  }
};

class ProgTr {
public:
  ProgTr(std::unique_ptr<absyn::AbsynTree> absyn_tree,
         std::unique_ptr<err::ErrorMsg> errormsg)
      : absyn_tree_(std::move(absyn_tree)), errormsg_(std::move(errormsg)),
        main_level_(std::make_unique<Level>(
            frame::NewFrame(temp::LabelFactory::NamedLabel("tigermain"),
                            std::list<bool>()),
            nullptr)),
        tenv_(std::make_unique<env::TEnv>()),
        venv_(std::make_unique<env::VEnv>()) {}

  void Translate();

  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<err::ErrorMsg> errormsg_;
  std::unique_ptr<Level> main_level_;
  std::unique_ptr<env::TEnv> tenv_;
  std::unique_ptr<env::VEnv> venv_;

  void FillBaseVEnv();
  void FillBaseTEnv();
};

}

#endif

#ifndef TIGER_CANON_CANON_H_
#define TIGER_CANON_CANON_H_

#include <cstdio>
#include <list>
#include <memory>
#include <stdexcept>
#include <vector>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"

namespace tree {
class StmList;
class Exp;
class Stm;
} // namespace tree

namespace frame {
class ProcFrag;
}

namespace canon {

class StmListList {
  friend class Canon;

public:
  StmListList() = default;

  void Append(tree::StmList *stmlist) { stmlist_list_.push_back(stmlist); }
  [[nodiscard]] const std::list<tree::StmList *> &GetList() const {
    return stmlist_list_;
  }

private:
  std::list<tree::StmList *> stmlist_list_;
};

class Block {
public:
  temp::Label *label_;
  StmListList *stm_lists_;

  Block() : stm_lists_(nullptr), label_(nullptr) {}
  Block(temp::Label *label, StmListList *stm_lists)
      : label_(label), stm_lists_(stm_lists) {}
};

struct StmAndExp {
  tree::Stm *s_;
  tree::Exp *e_;

  StmAndExp(const StmAndExp &) = delete;
  StmAndExp &operator=(const StmAndExp &) = delete;
};

class Traces {
public:
  Traces() = delete;
  Traces(nullptr_t) = delete;
  explicit Traces(tree::StmList *stm_list) : stm_list_(stm_list) {
    if (stm_list == nullptr)
      throw std::invalid_argument("NULL pointer is not allowed in Traces");
  }
  Traces(const Traces &traces) = delete;
  Traces(Traces &&traces) = delete;
  Traces &operator=(const Traces &traces) = delete;
  Traces &operator=(Traces &&traces) = delete;
  ~Traces();

  [[nodiscard]] tree::StmList *GetStmList() const { return stm_list_; }

private:
  tree::StmList *stm_list_;
};

class Canon {
  friend class frame::ProcFrag;

public:
  Canon() = delete;
  explicit Canon(tree::Stm *stm_ir)
      : stm_ir_(stm_ir), stm_canon_(nullptr), block_(),
        block_env_(new sym::Table<tree::StmList>()) {}

  tree::StmList *Linearize();

  canon::StmListList *BasicBlocks();

  tree::StmList *TraceSchedule();

  std::unique_ptr<Traces> TransferTraces() { return std::move(traces_); }

private:
  tree::Stm *stm_ir_;
  tree::StmList *stm_canon_;
  Block block_;
  sym::Table<tree::StmList> *block_env_;
  std::unique_ptr<Traces> traces_;

  tree::StmList *GetNext();

  void Trace(std::list<tree::Stm *> &stms);
};

} // namespace canon
#endif

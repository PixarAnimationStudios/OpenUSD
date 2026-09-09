//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "undo/CompoundCommand.h"
#include "undo/LambdaCommand.h"
#include "undo/NoodlesUndoManager.h"

#include "pxr/pxr.h"

// usd24
  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/noncopyable.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

/// Python wrapper for LambdaCommand that holds boost::python callable objects.
class PyLambdaCommand : public Command {
 public:
  PyLambdaCommand(std::string description, object doFunc, object undoFunc)
      : _description(std::move(description)),
        _doFunc(std::move(doFunc)),
        _undoFunc(std::move(undoFunc)) {}

  void execute() override {
    if (_doFunc && !_doFunc.is_none()) {
      _doFunc();
    }
  }

  void undo() override {
    if (_undoFunc && !_undoFunc.is_none()) {
      _undoFunc();
    }
  }

  std::string description() const override {
    return _description;
  }

 private:
  std::string _description;
  object _doFunc;
  object _undoFunc;
};

/// Python wrapper for CompoundCommand that accepts Python Command objects.
class PyCompoundCommand {
 public:
  explicit PyCompoundCommand(const std::string& description)
      : _cmd(std::make_shared<CompoundCommand>(description)) {}

  void addCommand(object pyDoFunc, object pyUndoFunc, const std::string& desc) {
    _cmd->addCommand(
        std::make_unique<PyLambdaCommand>(desc, std::move(pyDoFunc), std::move(pyUndoFunc)));
  }

  void pushToManager() {
    // Swap ownership: Python retains _cmd (now empty), while we transfer the
    // original to the undo manager.  boost::python objects can't transfer
    // unique_ptr ownership, so we wrap the shared_ptr in a thin forwarding
    // Command that the manager owns via unique_ptr.
    auto cmd = std::make_shared<CompoundCommand>(_cmd->description());
    std::swap(cmd, _cmd);

    // Wrap in a CommandPtr via a thin forwarding command
    struct SharedCommand : public Command {
      std::shared_ptr<CompoundCommand> inner;
      explicit SharedCommand(std::shared_ptr<CompoundCommand> c) : inner(std::move(c)) {}
      void execute() override {
        inner->execute();
      }
      void undo() override {
        inner->undo();
      }
      std::string description() const override {
        return inner->description();
      }
    };

    NoodlesUndoManager::instance().pushCommand(std::make_unique<SharedCommand>(std::move(cmd)));
  }

  std::string description() const {
    return _cmd->description();
  }

  bool empty() const {
    return _cmd->empty();
  }

  size_t size() const {
    return _cmd->size();
  }

 private:
  std::shared_ptr<CompoundCommand> _cmd;
};

/// Helper to push a LambdaCommand directly from Python.
void pushLambdaCommand(const std::string& description, object doFunc, object undoFunc) {
  NoodlesUndoManager::instance().pushCommand(
      std::make_unique<PyLambdaCommand>(description, std::move(doFunc), std::move(undoFunc)));
}

} // namespace

void wrapUndo() {
  class_<NoodlesUndoManager, noncopyable>("NoodlesUndoManager", no_init)
      .def(
          "instance",
          &NoodlesUndoManager::instance,
          return_value_policy<reference_existing_object>())
      .staticmethod("instance")
      .def("undo", &NoodlesUndoManager::undo)
      .def("redo", &NoodlesUndoManager::redo)
      .def("canUndo", &NoodlesUndoManager::canUndo)
      .def("canRedo", &NoodlesUndoManager::canRedo)
      .def("clear", &NoodlesUndoManager::clear)
      .def("undoDescription", &NoodlesUndoManager::undoDescription)
      .def("redoDescription", &NoodlesUndoManager::redoDescription)
      .def("setMaxStackDepth", &NoodlesUndoManager::setMaxStackDepth);

  class_<PyCompoundCommand, noncopyable>("CompoundCommand", init<std::string>())
      .def("addCommand", &PyCompoundCommand::addCommand)
      .def("pushToManager", &PyCompoundCommand::pushToManager)
      .def("description", &PyCompoundCommand::description)
      .def("empty", &PyCompoundCommand::empty)
      .def("size", &PyCompoundCommand::size);

  def("pushLambdaCommand", &pushLambdaCommand);
}

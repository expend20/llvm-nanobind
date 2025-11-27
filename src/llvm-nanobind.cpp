#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <memory>

#include <llvm-c/Core.h>

using namespace nanobind::literals;

struct NoMoveCopy {
  NoMoveCopy() = default;
  NoMoveCopy(const NoMoveCopy &) = delete;
  NoMoveCopy &operator=(const NoMoveCopy &) = delete;
};

static std::vector<LLVMContextRef> g_dispose_context;

struct LLVMContext : NoMoveCopy {
  LLVMContextRef m_ref = nullptr;
  bool m_global = false;

  explicit LLVMContext(bool global = false) : m_global(global) {
    printf("%p:LLVMContext(global: %d)\n", this, global);
    if (global) {
      m_ref = LLVMGetGlobalContext();
    } else {
      m_ref = LLVMContextCreate();
    }
    printf("ref: %p\n", m_ref);
  }

  LLVMContext(LLVMContext &&other) noexcept : m_ref(other.m_ref) {
    puts("LLVMContext(LLVMContext&&)");
    other.m_ref = nullptr;
  }

  LLVMContext &operator=(LLVMContext &&other) noexcept {
    puts("LLVMContext::operator=(LLVMContext&&)");
    if (this != &other) {
      LLVMContextDispose(m_ref);
      m_ref = other.m_ref;
      other.m_ref = nullptr;
      other.m_global = false;
    }
    return *this;
  }

  ~LLVMContext() {
    printf("%p:~LLVMContext(%p)\n", this, m_ref);
    if (m_ref != nullptr && !m_global) {
      LLVMContextDispose(m_ref);
    }
  }
};

struct LLVMModule : NoMoveCopy {
  LLVMModuleRef m_ref = nullptr;

  explicit LLVMModule(const std::string &name,
                      const LLVMContext *context = nullptr) {
    printf("%p:LLVMModule(%s, %p)\n", this, name.c_str(), context);
    m_ref = LLVMModuleCreateWithNameInContext(
        name.c_str(), context != nullptr ? context->m_ref : nullptr);
  }

  ~LLVMModule() {
    printf("%p:~LLVMModule(%p)\n", this, m_ref);
    if (m_ref != nullptr) {
      LLVMDisposeModule(m_ref);
    }
  }
};

struct LLVMException : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct LLVMContextManager : NoMoveCopy {
  std::unique_ptr<LLVMContext> m_context;

  LLVMContext &enter() {
    if (m_context)
      throw LLVMException("LLVMContextManager already entered");
    m_context = std::make_unique<LLVMContext>();
    return *m_context;
  }

  void exit(const nanobind::object &, const nanobind::object &,
            const nanobind::object &) {
    if (!m_context)
      throw LLVMException("LLVMModuleContext not entered");
    m_context.reset();
  }

  ~LLVMContextManager() {
    printf("~LLVMContextManager(%p)\n", m_context.get());
  }
};

struct LLVMModuleManager : NoMoveCopy {
  std::string m_name;
  LLVMContext *m_context = nullptr;

  std::unique_ptr<LLVMModule> m_module;

  LLVMModuleManager(std::string name, LLVMContext *context = nullptr)
      : m_name(std::move(name)), m_context(context) {
    printf("%p:LLVMModuleManager\n", this);
  }

  LLVMModule &enter() {
    if (m_module)
      throw LLVMException("LLVMModuleContext already entered");
    m_module = std::make_unique<LLVMModule>(m_name, m_context);
    return *m_module;
  }

  void exit(const nanobind::object &, const nanobind::object &,
            const nanobind::object &) {
    printf("%p:exit\n", this);
    if (!m_module)
      throw LLVMException("LLVMModuleContext not entered");
    m_module.reset();
  }

  ~LLVMModuleManager() {
    printf("%p:~LLVMModuleManager(%p)\n", this, m_module.get());
  }
};

#if 0
struct PauseContext { State &state; };

auto state = nb::class_<State>(m, "State")
   .def(...);

nb::class_<PauseContext>(state, "PauseContext")
    .def("__enter__", [](PauseContext &ctx) { ctx.state.PauseTiming(); })
    .def("__exit__", [](PauseContext &ctx, nb::handle, nb::handle, nb::handle) { ctx.state.ResumeTiming(); },
         nb::arg().none(), nb::arg().none(), nb::arg().none());

state.def("pause", [](State &s) { return PauseContext{s}; }, nb::rv_policy::reference_internal);
#endif

static LLVMContext &global_context() {
  static LLVMContext context(true);
  return context;
}

// Reference: https://nanobind.readthedocs.io/en/latest/basics.html
NB_MODULE(llvm, m) {
  nanobind::class_<LLVMContext>(m, "LLVMContext")
      .def_prop_rw(
          "discard_value_names",
          [](const LLVMContext &ctx) -> bool {
            return LLVMContextShouldDiscardValueNames(ctx.m_ref);
          },
          [](LLVMContext &ctx, bool discard) {
            LLVMContextSetDiscardValueNames(ctx.m_ref, discard);
          },
          R"(Return true if the Context runtime configuration is set to discard all value names.

When true, only GlobalValue names will be available in the IR.)")
      .def(
          "create_module",
          [](LLVMContext *self, const char *name) {
            return new LLVMModuleManager(name, self);
          },
          R"(Create an LLVMModule in this context.)");

  m.def("global_context", &global_context, R"(Get the global LLVM Context.)",
        nanobind::rv_policy::reference);

  nanobind::class_<LLVMContextManager>(m, "LLVMContextManager")
      /*.def(nanobind::init<>(),
           R"(Wrapper class for safely managing an LLVMContext.)")*/
      .def("__enter__", &LLVMContextManager::enter,
           R"(Enter a new LLVM Context.)",
           nanobind::rv_policy::reference_internal)
      .def("__exit__", &LLVMContextManager::exit,
           R"(Exit the current LLVM Context.)", "exc_type"_a.none(),
           "exc_value"_a.none(), "traceback"_a.none());

  m.def(
      "create_context", [] { return new LLVMContextManager(); },
      R"(Create an LLVMModule.)");

  nanobind::class_<LLVMModule>(m, "LLVMModule")
      .def_prop_rw(
          "data_layout",
          [](const LLVMModule &mod) -> const char * {
            return LLVMGetDataLayoutStr(mod.m_ref);
          },
          [](LLVMModule &mod, const char *dl) {
            LLVMSetDataLayout(mod.m_ref, dl);
          },
          R"(Get or set the data layout string for this module.)");

  nanobind::class_<LLVMModuleManager>(m, "LLVMModuleManager")
      /*.def(nanobind::init<const std::string &, LLVMContext *>(), "name"_a,
           "context"_a = nullptr,
           R"(Wrapper class for safely managing an LLVMModule.)")*/
      .def("__enter__", &LLVMModuleManager::enter, R"(Create the module.)",
           nanobind::rv_policy::reference_internal)
      .def("__exit__", &LLVMModuleManager::exit, R"(Destroy the module.)",
           "exc_type"_a.none(), "exc_value"_a.none(), "traceback"_a.none());
}

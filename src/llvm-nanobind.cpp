#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <llvm-c/Core.h>

using namespace nanobind::literals;

static void test() { auto llvm_context = LLVMContextCreate(); }

struct LLVMContext {
  LLVMContextRef m_ref = nullptr;
  bool m_global = false;

  explicit LLVMContext(bool global = false) : m_global(global) {
    printf("LLVMContext(global: %d)\n", global);
    if (global) {
      m_ref = LLVMGetGlobalContext();
    } else {
      m_ref = LLVMContextCreate();
    }
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
    printf("~LLVMContext(%p)\n", m_ref);
    if (m_ref != nullptr && !m_global) {
      LLVMContextDispose(m_ref);
    }
  }

  LLVMContext(const LLVMContext &) = delete;
  LLVMContext &operator=(const LLVMContext &) = delete;
};

struct LLVMModule {
  LLVMModuleRef m_ref = nullptr;

  explicit LLVMModule(const std::string &name,
                      const LLVMContext *context = nullptr) {
    m_ref = LLVMModuleCreateWithNameInContext(
        name.c_str(), context != nullptr ? context->m_ref : nullptr);
  }

  ~LLVMModule() {
    printf("~LLVMModule(%p)\n", m_ref);
    if (m_ref != nullptr) {
      LLVMDisposeModule(m_ref);
    }
  }

  LLVMModule(const LLVMModule &) = delete;
  LLVMModule &operator=(const LLVMModule &) = delete;
};
static_assert(std::is_move_constructible_v<LLVMModule>);

// Reference: https://nanobind.readthedocs.io/en/latest/basics.html
NB_MODULE(llvm, m) {
  nanobind::class_<LLVMContext>(m, "LLVMContext")
      .def(nanobind::init<bool>(), "global_context"_a = false,
           R"(Create a new LLVM Context.)")
      .def_prop_rw(
          "discard_value_names",
          [](const LLVMContext &ctx) -> bool {
            return LLVMContextShouldDiscardValueNames(ctx.m_ref);
          },
          [](LLVMContext &ctx, bool discard) {
            LLVMContextSetDiscardValueNames(ctx.m_ref, discard);
          },
          R"(Return true if the Context runtime configuration is set to discard all value names.

When true, only GlobalValue names will be available in the IR.)");
}

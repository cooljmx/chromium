#ifndef ALGO_BROKER_H__
#define ALGO_BROKER_H__

#ifdef ALGO_BROKER_EXPORTS
#define ALGO_BROKER_API __declspec(dllexport)
#else
#define ALGO_BROKER_API
#endif

namespace algo {

struct TargetOptions {
  const wchar_t* host_path;
  const wchar_t* command_line;
  const wchar_t* current_directory;
  const wchar_t* package_name;
  const wchar_t* fs_rules;
  const wchar_t* reg_rules;
  const wchar_t* np_rules;
  const wchar_t* ev_rules;
};

struct TargetInformation {
  void* process_handle;
  void* thread_handle;
  unsigned process_id;
  unsigned thread_id;
};

struct TargetInitializeOptions {
};

}

extern "C" {
ALGO_BROKER_API bool Initialize();

ALGO_BROKER_API int Spawn(const algo::TargetOptions* options,
                          algo::TargetInformation* target_information);

ALGO_BROKER_API bool Resume(const algo::TargetInformation* target_information);

ALGO_BROKER_API void WaitAll();
}
#endif

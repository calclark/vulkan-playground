project(
  'vulkan-demo',
  'cpp',
  default_options: [
    'cpp_std=c++20',
    'b_ndebug=if-release',
    'cpp_rtti=false',
    'cpp_eh=none',
    'warning_level=3',
    'cpp_args=-Wold-style-cast',
  ]
)

if get_option('debug')
    add_project_arguments('-DUSE_VALIDATION_LAYERS', language: 'cpp')
endif

sources = [
  'src/main.cpp',
]

cmake = import('cmake')
cmake.subproject('glfw')

dependencies = [
  dependency('vulkan'),
  dependency('glfw3', fallback: ['glfw', 'glfw_dep']),
  dependency('glm', fallback: ['glm', 'glm_dep']),
  dependency('fmt', fallback: ['fmt', 'fmt_dep']),
]

out = executable(
  'vulkan-demo',
  sources,
  dependencies: dependencies
)

run_target('run', command: out)


project(
  'demo',
  'cpp',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
    'cpp_args=-Wold-style-cast'
  ]
)

sources = [
  'src/main.cpp',
]

cmake = import('cmake')
cmake.subproject('glfw')

dependencies = [
  dependency('vulkan'),
  dependency('glfw3', fallback: ['glfw', 'glfw_dep']),
  dependency('glm', fallback: ['glm', 'glm_dep']),
]

out = executable(
  'demo',
  sources,
  dependencies: dependencies
)

run_target('run', command: out)


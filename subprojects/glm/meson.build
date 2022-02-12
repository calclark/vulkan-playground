project(
  'glm',
  'cpp',
  default_options: [
    'warning_level=0'
  ]
)

inc = include_directories(
  'include',
  is_system: true
)

glm = static_library(
  'glm',
  include_directories: inc
)

glm_dep = declare_dependency(
  include_directories: inc,
  link_with: glm
)
